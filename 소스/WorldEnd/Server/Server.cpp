#include "Server.h"
#include "stdafx.h"

Server& Server::GetInstance()
{
	static Server instance{};
	return instance;
}

Server::Server()
{	
	// ------- 초기화 ------- //
	for (size_t i = 0; i < MAX_USER; ++i) {
		m_clients[i] = make_shared<Client>();
	}

	for (size_t i = WARRIOR_MONSTER_START; i < WIZARD_MONSTER_END; ++i) {
		m_clients[i] = make_shared<WarriorMonster>();
	}

	for (size_t i = WARRIOR_MONSTER_START; i < WIZARD_MONSTER_END; ++i) {
		m_clients[i] = make_shared<ArcherMonster>();
	}

	for (size_t i = WARRIOR_MONSTER_START; i < WIZARD_MONSTER_END; ++i) {
		m_clients[i] = make_shared<WizardMonster>();
	}

	m_game_room_manager = make_unique<GameRoomManager>();
	m_game_room_manager->InitGameRoom(0);	

	// ----------------------- //

}

Server::~Server()
{
	for (size_t i = 0; i < MAX_USER; ++i) {
		if (State::ST_INGAME == m_clients[i]->GetState())
			Disconnect(m_clients[i]->GetId());
	}

	closesocket(m_server_socket);
	WSACleanup();
}

void Server::Network()
{
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		return;

	m_server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_server_socket) 
		ErrorDisplay("socket()");

	// bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(m_server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (SOCKET_ERROR == retval)
		ErrorDisplay("bind()");

	// listen
	retval = listen(m_server_socket, SOMAXCONN);
	if (SOCKET_ERROR == retval)
		ErrorDisplay("listen()");

	m_handle_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_server_socket), m_handle_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	
	ExpOver accept_ex;
	memcpy(&accept_ex._send_buf, &c_socket, sizeof(SOCKET));
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_type = OP_ACCEPT;

	BOOL ret = AcceptEx(m_server_socket, c_socket, accept_ex._send_buf + sizeof(SOCKET), 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &accept_ex._wsa_over);

	if (FALSE == ret) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			ErrorDisplay("Accept Error");
		}
	}

	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(&Server::WorkerThread, this);

	for (auto& th : m_worker_threads)
		th.detach();

	thread thread1{ &Server::Timer,this };
	thread1.detach();


	using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	using ms = std::chrono::duration<float, std::milli>;
	std::chrono::time_point<std::chrono::steady_clock> fps_timer{ std::chrono::steady_clock::now() };

	frame fps{}, frame_count{};
	while (true) {

		// 아무도 서버에 접속하지 않았으면 패스
		//if (m_accept)
		//{
		//	// 이 부분이 없다면 첫 프레임 때 deltaTime이 '클라에서 처음 접속한 시각 - 서버를 켠 시각' 이 된다.
		//	fps_timer = std::chrono::steady_clock::now();
		//	continue;
		//}

		// 이전 사이클에 얼마나 시간이 걸렸는지 계산
		fps = duration_cast<frame>(std::chrono::steady_clock::now() - fps_timer);

		// 아직 1/60초가 안지났으면 패스
		if (fps.count() < 1) continue;

		if (frame_count.count() & 1) {
			//SendPlayerDataPacket();
		}
		else {
			m_game_room_manager->SendMonsterData();
		}

		m_game_room_manager->Update(duration_cast<ms>(fps).count() / 1000.0f);

		frame_count = duration_cast<frame>(frame_count + fps);
		if (frame_count.count() >= 60) {
			frame_count = frame::zero();
		}
		else {
		}
		fps_timer = std::chrono::steady_clock::now();
	}

}

void Server::WorkerThread() 
{
	while(true) {
		DWORD num_bytes{};
		ULONG_PTR key{};
		WSAOVERLAPPED* over{};

		BOOL ret = GetQueuedCompletionStatus(m_handle_iocp, &num_bytes, &key, &over, INFINITE);
		ExpOver* exp_over = reinterpret_cast<ExpOver*>(over);
		
		if (FALSE == ret) {
			if (exp_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << static_cast<INT>(key) << "]\n";
				//Disconnect(static_cast<int>(key));
				if (exp_over->_comp_type == OP_SEND) delete exp_over;
				continue;
			}
		}

		switch (exp_over->_comp_type) {
		case OP_ACCEPT: {
			SOCKET c_socket{};
			memcpy(&c_socket, &exp_over->_send_buf, sizeof(SOCKET));
			int new_id{ GetNewId() };

			if (-1 == new_id) {
				std::cout << "Maximum Number of Users Exceeded." << std::endl;
			}
			else {
				auto cl = dynamic_pointer_cast<Client>(m_clients[new_id]);

				cl->SetId(new_id);
				cl->SetSocket(c_socket);

				// client 클래스의 생성자에서 remain_size, EXP_OVER를 초기화함
				// 이후 Disconnect에서 나간 클라이언트를 다시 초기값으로 해주면
				// 여기서 다시 할 필요는 없을 것

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), m_handle_iocp, new_id, 0);
				cl->DoRecv();

				// c_socket 이 사용되었을 때만 WSASocket을 호출하면 됨
				c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			}

			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			memcpy(&exp_over->_send_buf, &c_socket, sizeof(SOCKET));
			AcceptEx(m_server_socket, c_socket, exp_over->_send_buf + sizeof(SOCKET), 0,
				sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &exp_over->_wsa_over);
			break;
		}
		case OP_RECV: {
			int id = static_cast<int>(key);

			if (num_bytes == 0) {
				Disconnect(static_cast<int>(key));
				continue;
			}
			auto cl = dynamic_pointer_cast<Client>(m_clients[id]);

			int remain_data = num_bytes + cl->GetRemainSize();
			char* packet = exp_over->_send_buf;
			int packet_size = packet[0];
			while (packet_size <= remain_data) {
				ProcessPacket(id, packet);
				remain_data -= packet_size;
				packet += packet_size;

				if (remain_data > 0)
					packet_size = packet[0];
				else
					break;
			}

			if (remain_data > 0) {
				cl->SetRemainSize(remain_data);
				memcpy(&exp_over->_send_buf, packet, remain_data);
			}
			cl->DoRecv();
			break;
		}
		case OP_SEND: {
			if (num_bytes != exp_over->_wsa_buf.len) {
				Disconnect(static_cast<int>(key));
			}
			delete exp_over;
			break;
		}

		}
	}
}

void Server::ProcessPacket(int id, char* p) 
{
	unsigned char type = p[1];
	auto cl = dynamic_pointer_cast<Client>(m_clients[id]);
	
	switch (type){
	case CS_PACKET_LOGIN:
	{
		CS_LOGIN_PACKET* packet = reinterpret_cast<CS_LOGIN_PACKET*>(p);
		cl->SetPlayerType(packet->player_type);
		cl->SetName(packet->name);
		{
			lock_guard<mutex> lock{ cl->GetStateMutex() };
			cl->SetState(State::ST_INGAME);
		}
		SendLoginOkPacket(cl);

		// 원래는 던전 진입 시 던전에 배치해야하지만
		// 현재 마을이 없이 바로 던전에 진입하므로 던전에 입장시킴
		m_game_room_manager->SetPlayer(0, id, cl);
		m_game_room_manager->SendAddMonster(id);

		cout << packet->name << " is connect" << endl;
		break;
	}
	case CS_PACKET_PLAYER_MOVE:
	{
		CS_PLAYER_MOVE_PACKET* packet = reinterpret_cast<CS_PLAYER_MOVE_PACKET*>(p);

		// 위치는 서버에서 저장하므로 굳이 받을 필요는 없을 것
		// 속도만 받아와서 처리해도 됨
		cl->SetVelocity(packet->velocity);

		XMFLOAT3 pos = packet->pos;
		pos.x += packet->velocity.x;
		pos.y += packet->velocity.y;
		pos.z += packet->velocity.z;

		MovePlayer(cl, pos);
		RotatePlayer(cl, packet->yaw);

		PlayerCollisionCheck(cl);
		SendPlayerDataPacket();
		break;
	}
	case CS_PACKET_PLAYER_ATTACK:
	{
		CS_ATTACK_PACKET* packet = reinterpret_cast<CS_ATTACK_PACKET*>(p);

		
		switch (packet->attack_type)
		{
			case AttackType::NORMAL:
			{
				cout << "공격!" << endl;
				break;
		    }	
	    }
		break;
	}

	case CS_PACKET_WEAPON_COLLISION:
	{
		CS_WEAPON_COLLISION_PACKET* packet = reinterpret_cast<CS_WEAPON_COLLISION_PACKET*>(p);

		// 이벤트를 생성한다
		COLLISION_EVENT event{};
		event.user_id = cl->GetId();
		event.bounding_box.Center.x = packet->x;
		event.bounding_box.Center.y = packet->y;
		event.bounding_box.Center.z = packet->z;
		event.bounding_box.Extents = cl->GetWeaponBoundingBox().Extents;
		event.attack_type = packet->attack_type;
		event.collision_type = packet->collision_type;
		event.end_time = packet->end_time;

		// 이벤트를 해당 플레이어가 속한 던전에 추가한다
		m_game_room_manager->SetEvent(event, cl->GetId());

		break;
	}

	case CS_PACKET_CHANGE_ANIMATION:
	{
		CS_CHANGE_ANIMATION_PACKET* animation_packet = reinterpret_cast<CS_CHANGE_ANIMATION_PACKET*>(p);

		SC_CHANGE_ANIMATION_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_CHANGE_ANIMATION;
		packet.id = cl->GetId();
		packet.animation_type = animation_packet->animation_type;

		// 마을, 게임룸 구분하여 보낼 필요 있음
		for (const auto& cl : m_clients) {
			if (-1 == cl->GetId()) continue;
			if (cl->GetId() == id) continue;
			
			cl->DoSend(&packet);
		}

		break;
	}

	}
}


void Server::Disconnect(int id)
{
	// remain_size 를 0으로, ExpOver의 _comp_type을 OP_RECV 로 해야함
	auto cl = dynamic_pointer_cast<Client>(m_clients[id]);
	cl->SetId(-1);
	cl->SetPosition(0.f, 0.f, 0.f);
	cl->SetVelocity(0.f, 0.f, 0.f);
	cl->SetYaw(0.f);
	closesocket(cl->GetSocket());

	lock_guard<mutex> lock{ cl->GetStateMutex() };
	cl->SetState(State::ST_FREE);
}

void Server::SendLoginOkPacket(const shared_ptr<Client>& player) const
{
	SC_LOGIN_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.player_data.id = player->GetId();
	packet.player_data.pos = player->GetPosition();
	packet.player_data.hp = player->GetHp();
	packet.player_type = player->GetPlayerType();
	strcpy_s(packet.name, sizeof(packet.name), player->GetName().c_str());

	player->DoSend(&packet);

	packet.type = SC_PACKET_ADD_CHARACTER;

	// 현재 접속해 있는 모든 클라이언트들에게 새로 로그인한 클라이언트들의 정보를 전송
	for (const auto& other : m_clients){
		if (-1 == other->GetId()) continue;
		if (player->GetId() == other->GetId()) continue;
		
		other->DoSend(&packet);
	}

	// 새로 로그인한 클라이언트에게 현재 접속해 있는 모든 클라이언트들의 정보를 전송
	for (const auto& other : m_clients){
		if (-1 == other->GetId()) continue;
		if (player->GetId() == other->GetId()) continue;

		SC_LOGIN_OK_PACKET sub_packet{};
		sub_packet.size = sizeof(sub_packet);
		sub_packet.type = SC_PACKET_ADD_CHARACTER;
		sub_packet.player_data = other->GetPlayerData();
		strcpy_s(sub_packet.name, sizeof(sub_packet.name), other->GetName().c_str());
		sub_packet.player_type = other->GetPlayerType();

		player->DoSend(&sub_packet);
	}
}

void Server::SendPlayerDataPacket()
{
	SC_UPDATE_CLIENT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_CLIENT;
	
	for (int i = 0; i < MAX_INGAME_USER; ++i)
		packet.data[i] = m_clients[i]->GetPlayerData();

	for (const auto& cl : m_clients){
		if (State::ST_INGAME != cl->GetState()) continue; 
		
		cl->DoSend(&packet);
	}
}

void Server::PlayerCollisionCheck(shared_ptr<Client>& player)
{
	BoundingOrientedBox& player_obb = player->GetBoundingBox();
	for (auto& cl : m_clients) {
		if (-1 == cl->GetId()) continue;
		if (cl->GetId() == player->GetId()) continue;

		BoundingOrientedBox& obb = cl->GetBoundingBox();
		if (player_obb.Intersects(obb)) {
			CollideByStatic(player, obb);
		}
	}

	auto& v = m_game_room_manager->GetStructures();

	for (auto& obj : v) {
		BoundingOrientedBox& obb = obj->GetBoundingBox();
		if (player_obb.Intersects(obb)) {
			CollideByStaticOBB(player, obb);
		}
	}
}

INT Server::GetNewId()
{
	for (size_t i = 0; i < MAX_USER; ++i) {
		lock_guard<mutex> lock{ m_clients[i]->GetStateMutex() };
		if (State::ST_FREE == m_clients[i]->GetState()) {
			m_clients[i]->SetState(State::ST_ACCEPT);
			return i;
		}
	}

	return -1;
}

void Server::MovePlayer(shared_ptr<Client>& player, XMFLOAT3 pos)
{
	player->SetPosition(pos);

	// 바운드 박스 갱신
	player->SetBoundingBoxCenter(pos);
}

void Server::Timer()
{
	using namespace chrono;
	TIMER_EVENT event;

	while (true) {
		if (!m_timer_queue.try_pop(event)) {
			std::this_thread::sleep_for(1ms);
			continue;
		}

		if (event.event_time <= system_clock::now()) {
			
			switch (event.event_type)
			{
			case EVENT_PLAYER_ATTACK:
				cout << "공격 이벤트 중" << endl;
				break;
			default:
				break;
			}
		}

	}
	/*while (true) {
		auto now = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - event.start_time);
		if (m_attack_check == true) {
			if (elapsed.count() < 5.0f) {
			}
			else {
				m_attack_check = false;
				m_start_cool_time = 0;
				for (int i = 0; i < MAX_USER; ++i)
					SendPlayerAttackPacket(m_clients[i].m_player_data.id);
				event.start_time = now;
			}
		}
	}*/


}

void Server::RotatePlayer(shared_ptr<Client>& player, FLOAT yaw)
{
	// 플레이어 회전, degree 값
	player->SetYaw(yaw);

	float radian = XMConvertToRadians(yaw);
	
	XMFLOAT4 q{};
	XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(0.f, radian, 0.f));

	// 바운드 박스 회전
	player->SetBoundingBoxOrientation(q);
}

void Server::CollideByStatic(shared_ptr<Client>& player, const BoundingOrientedBox& obb)
{
	BoundingOrientedBox& player_obb = player->GetBoundingBox();

	// length 는 바운드 박스의 길이 합
	// dist 는 바운드 박스간의 거리
	FLOAT x_length = player_obb.Extents.x + obb.Extents.x;
	FLOAT x_dist = abs(player_obb.Center.x - obb.Center.x);

	FLOAT z_length = player_obb.Extents.z + obb.Extents.z;
	FLOAT z_dist = abs(player_obb.Center.z - obb.Center.z);

	// 겹치는 정도
	FLOAT x_bias = x_length - x_dist;
	FLOAT z_bias = z_length - z_dist;

	XMFLOAT3 pos = player->GetPosition();

	// z 방향으로 밀어내기
	if (x_bias - z_bias >= numeric_limits<FLOAT>::epsilon()) {
		
		// player_obb 가 앞쪽으로 밀려남
		if (player_obb.Center.z - obb.Center.z >= numeric_limits<FLOAT>::epsilon()) {
			pos.z += z_bias;
		}
		// player_obb 가 뒤쪽으로 밀려남
		else {
			pos.z -= z_bias;
		}
	}

	// x 방향으로 밀어내기
	else {

		// player_obb 가 앞쪽으로 밀려남
		if (player_obb.Center.x - obb.Center.x >= numeric_limits<FLOAT>::epsilon()) {
			pos.x += x_bias;
		}
		// player_obb 가 뒤쪽으로 밀려남
		else {
			pos.x -= x_bias;
		}
	}

	MovePlayer(player, pos);
}

void Server::CollideByMoveMent(shared_ptr<Client>& player1, shared_ptr<Client>& player2)
{
	// 충돌한 플레이어의 속도
	XMFLOAT3 velocity = player1->GetVelocity();

	// 충돌된 오브젝트의 위치
	XMFLOAT3 pos = player2->GetPosition();

	// 충돌된 오브젝트를 충돌한 플레이어의 속도만큼 밀어냄
	// 밀어내는 방향 고려할 필요 있음 ( 아직 X )
	pos.x += velocity.x;
	pos.y += velocity.y;
	pos.z += velocity.z;
	MovePlayer(player2, pos);
}

void Server::CollideByStaticOBB(shared_ptr<Client>& player, const BoundingOrientedBox& obb)
{
	// 1. 오브젝트에서 충돌면을 구한다
	// 2. 해당 충돌면의 법선 벡터를 구한다
	// 3. 플레이어의 속도 벡터에 2에서 구한 법선 벡터를 투영한다
	// 4. 투영 벡터만큼 플레이어를 이동시킨다

	XMFLOAT3 corners[8]{};

	obb.GetCorners(corners);
	
	// 꼭짓점 시계방향 0,1,5,4
	XMFLOAT3 o_square[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z} ,
		{corners[5].x, 0.f, corners[5].z} ,
		{corners[4].x, 0.f, corners[4].z} };

	player->GetBoundingBox().GetCorners(corners);

	XMFLOAT3 p_square[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z} ,
		{corners[5].x, 0.f, corners[5].z} ,
		{corners[4].x, 0.f, corners[4].z} };


	
	for (const XMFLOAT3& point : p_square) {
		if (!obb.Contains(XMLoadFloat3(&point))) continue;

		array<float, 4> dist{};
		dist[0] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[0]), XMLoadFloat3(&o_square[1]), XMLoadFloat3(&point)));
		dist[1] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[1]), XMLoadFloat3(&o_square[2]), XMLoadFloat3(&point)));
		dist[2] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[2]), XMLoadFloat3(&o_square[3]), XMLoadFloat3(&point)));
		dist[3] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[3]), XMLoadFloat3(&o_square[0]), XMLoadFloat3(&point)));

		auto min = min_element(dist.begin(), dist.end());
		
		XMFLOAT3 v{};
		if (*min == dist[0])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[1], o_square[2]));
		}
		else if (*min == dist[1])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[1], o_square[0]));
		}
		else if (*min == dist[2])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[2], o_square[1]));
		}
		else if (*min == dist[3])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[0], o_square[1]));
		}
		v = Vector3::Mul(v, *min);
		MovePlayer(player, Vector3::Add(player->GetPosition(), v));
		

		//// 충돌한 오브젝트의 꼭짓점을 이용해 사각형의 테두리를 따라가는 벡터 4개를 구함
		//XMFLOAT3 vec[4]{
		//	Vector3::Sub(o_square[0], o_square[1]),
		//	Vector3::Sub(o_square[1], o_square[2]),
		//	Vector3::Sub(o_square[2], o_square[3]),
		//	Vector3::Sub(o_square[3], o_square[0])
		//};

		//// 플레이어의 충돌점과 위에서 구한 벡터를 더함
		//XMFLOAT3 position[4]{};
		//for (size_t i = 0; XMFLOAT3& pos : position) {
		//	pos = Vector3::Add(point, vec[i]);
		//	++i;
		//}

		//// 구한 위치와 player obb의 Center 와의 거리를 구함
		//// 거리가 가장 짧은 위치에 더한 벡터가 밀어내는 방향의 벡터
		//array<FLOAT, 4>dist{};
		//XMFLOAT3 player_center{ player.m_bounding_box.Center.x, 0.f, player.m_bounding_box.Center.z };
		//for (size_t i = 0; FLOAT & d : dist) {
		//	d = Vector3::Length(Vector3::Sub(player_center, position[i]));
		//	++i;
		//}

		//// dist에서 최솟값의 위치를 알아내고 이를 이용해 index를 구함
		//// vec[index] 가 밀어내야 할 벡터가 됨
		//auto p = min_element(dist.begin(), dist.end());
		//int index = distance(dist.begin(), p);

		//// 충돌점과 충돌한 직선의 거리를 구함
		//// 위에서 구한 벡터의 index - 1 이 충돌한 직선이 됨
		//int line_index = index - 1;
		//if (-1 == line_index)
		//	line_index = 3;

		//float scale = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[line_index]), XMLoadFloat3(&o_square[(line_index + 1) % 4]), XMLoadFloat3(&point)));

		//
		//// vec[index] 를 정규화 하고
		//XMFLOAT3 direction = Vector3::Normalize(vec[index]);

		//XMFLOAT3 move_vector = Vector3::Mul(direction, scale);

		//MovePlayer(player, Vector3::Add(player.m_player_data.pos, move_vector));
		break;
	}

}
 
