#include "Server.h"
#include "stdafx.h"
#include "map.h"

unique_ptr<DungeonManager> g_dungeon_manager{};

Server::Server() : m_disconnect_cnt{ 0 }, m_floor{1}, m_next_monster_id {0}, m_accept {false}
{
}

int Server::Network()
{

	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		return 1;
	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (g_socket == INVALID_SOCKET) ErrorDisplay("socket()");

	// bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(g_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR) ErrorDisplay("bind()");

	// listen
	retval = listen(g_socket, SOMAXCONN);
	if (retval == SOCKET_ERROR) ErrorDisplay("listen()");

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	ExpOver accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._send_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_type = OP_ACCEPT;

	bool ret = AcceptEx(g_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &accept_ex._wsa_over);

	for (int i = 0; i < 6; ++i)
		m_worker_threads.emplace_back(&Server::WorkerThreads, this);

	for (auto& th : m_worker_threads)
		th.detach();

	thread thread1{ &Server::Timer,this };
	thread1.detach();


	using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	using ms = std::chrono::duration<float, std::milli>;
	std::chrono::time_point<std::chrono::steady_clock> fps_timer{ std::chrono::steady_clock::now() };

	g_dungeon_manager = make_unique<DungeonManager>();
	g_dungeon_manager->InitDungeon(0);


	frame fps{}, frame_count{};
	while (true) {

		// 아무도 서버에 접속하지 않았으면 패스
		if (m_accept)
		{
			// 이 부분이 없다면 첫 프레임 때 deltaTime이 '클라에서 처음 접속한 시각 - 서버를 켠 시각' 이 된다.
			fps_timer = std::chrono::steady_clock::now();
			continue;
		}

		// 이전 사이클에 얼마나 시간이 걸렸는지 계산
		fps = duration_cast<frame>(std::chrono::steady_clock::now() - fps_timer);

		// 아직 1/60초가 안지났으면 패스
		if (fps.count() < 1) continue;

		if (frame_count.count() & 15) {
			//SendPlayerDataPacket();
		}
		else {
			//SendMonsterDataPacket();
			g_dungeon_manager->SendMonsterData();
		}

		//Update(duration_cast<ms>(fps).count() / 1000.0f);
		g_dungeon_manager->Update(duration_cast<ms>(fps).count() / 1000.0f);

		frame_count = duration_cast<frame>(frame_count + fps);
		if (frame_count.count() >= 60) {
			frame_count = frame::zero();
		}
		else {
		}
		fps_timer = std::chrono::steady_clock::now();
	}

	for (const auto& pl : m_clients)
	{
		if (pl.m_player_data.active_check)
			Disconnect(pl.m_player_data.id);
	}

	for (auto& th : m_worker_threads)
		th.join();

	closesocket(g_socket);
	WSACleanup();
	return 0;
}

void Server::WorkerThreads() 
{
	for (;;) {
		DWORD num_bytes{};
		LONG64 key{};
		WSAOVERLAPPED* over{};
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, reinterpret_cast<PULONG_PTR>(&key), &over, INFINITE);
		ExpOver* exp_over = reinterpret_cast<ExpOver*>(over);
		if (FALSE == ret) {
			if (exp_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				//Disconnect(static_cast<int>(key));
				if (exp_over->_comp_type == OP_SEND) delete exp_over;
				continue;
			}
		}
		switch (exp_over->_comp_type) {
		case OP_ACCEPT: {
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_send_buf));
			CHAR new_id{ GetNewId() };
			if (new_id == -1) {
				std::cout << "Maxmum user overflow. Accept aborted." << std::endl;
			}
			else {
				Session& cl = m_clients[new_id];
				cl.m_lock.lock();
				cl.m_player_data.id = new_id;
				cl.m_player_data.active_check = true;
				cl.m_socket = c_socket;
				cl.m_ready_check = false;
				constexpr char dummy_name[10] = "dummy\0";
				strcpy_s(cl.m_name, sizeof(dummy_name), dummy_name);
				cl.m_player_type = PlayerType::WARRIOR;
				cl.m_prev_size = 0;
				cl.m_recv_over._comp_type = OP_RECV;
				cl.m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl.m_recv_over._send_buf);
				cl.m_recv_over._wsa_buf.len = sizeof(cl.m_recv_over._send_buf);
				ZeroMemory(&cl.m_recv_over._wsa_over, sizeof(cl.m_recv_over._wsa_over));
				cl.m_lock.unlock();

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, new_id, 0);
				cl.DoRecv();
			}
			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_send_buf)) = c_socket;
			AcceptEx(g_socket, c_socket, exp_over->_send_buf + 8, 0,
				sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &exp_over->_wsa_over);
			break;
		}
		case OP_RECV: {
			if (num_bytes == 0) {
				//disconnect(static_cast<int>(key));
				continue;
			}
			Session& cl = m_clients[static_cast<int>(key)];
			int remain_data = num_bytes + cl.m_prev_size;
			char* packet_start = exp_over->_send_buf;
			int packet_size = packet_start[0];
			while (packet_size <= remain_data) {
				ProcessPacket(static_cast<int>(key), packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl.m_prev_size = remain_data;
				memcpy(&exp_over->_send_buf, packet_start, remain_data);
			}
			cl.DoRecv();
			break;
		}
		case OP_SEND: {
			if (num_bytes != exp_over->_wsa_buf.len) {
				//Disconnect(static_cast<int>(key));
			}
			delete exp_over;
			break;
		}

		}
	}
}

void Server::ProcessPacket(const int id, char* p) 
{
	char packet_type = p[1];
	Session& cl = m_clients[id];
	
	switch (packet_type)
	{
	case CS_PACKET_LOGIN:
	{
		CS_LOGIN_PACKET* login_packet = reinterpret_cast<CS_LOGIN_PACKET*>(p);
		cl.m_player_type = login_packet->player_type;
		strcpy_s(cl.m_name, login_packet->name);
		cl.m_lock.lock();
		cl.m_state = STATE::ST_INGAME;
		cl.m_lock.unlock();
		SendLoginOkPacket(cl);
		//SendMonsterAddPacket();

		// 원래는 던전 진입 시 던전에 배치해야하지만
		// 현재 마을이 없이 바로 던전에 진입하므로 던전에 입장시킴
		g_dungeon_manager->SetPlayer(0, id, &cl);
		g_dungeon_manager->SendMonsterAdd(id);

		cout << login_packet->name << " is connect" << endl;


		

		// 재접속 시 disconnectCount를 감소시켜야함
		// disconnect_cnt = max(0, disconnect_cnt - 1);
		//SendPlayerDataPacket();
		break;
	}
	case CS_PACKET_PLAYER_MOVE:
	{
		CS_PLAYER_MOVE_PACKET* move_packet = reinterpret_cast<CS_PLAYER_MOVE_PACKET*>(p);

		cl.m_player_data.velocity = move_packet->velocity;

		XMFLOAT3 pos = move_packet->pos;
		pos.x += move_packet->velocity.x;
		pos.y += move_packet->velocity.y;
		pos.z += move_packet->velocity.z;

		MovePlayer(cl, pos);
		RotatePlayer(cl, move_packet->yaw);

		//cl.m_player_data.pos = move_packet->pos;
		//cl.m_player_data.velocity = move_packet->velocity;
		//cl.m_player_data.yaw = move_packet->yaw;


		//// 바운드 박스 처리
		//cl.m_bounding_box.Center = move_packet->pos;

		//cout << "x: " << cl.m_player_data.pos .x << " y: " << cl.m_player_data.pos.y <<
		//	" z: " << cl.m_player_data.pos.z << endl;

		PlayerCollisionCheck(cl);
		SendPlayerDataPacket();
		break;
	}
	case CS_PACKET_PLAYER_ATTACK:
	{
		CS_ATTACK_PACKET* attack_packet = reinterpret_cast<CS_ATTACK_PACKET*>(p);

		
		switch (attack_packet->key)
		{
			case INPUT_KEY_E:
			{
				cout << "공격!" << endl;
				m_attack_check = true;
				break;
		    }	
	    }
		break;
	}

	case CS_PACKET_WEAPON_COLLISION:
	{
		CS_WEAPON_COLLISION_PACKET* packet = reinterpret_cast<CS_WEAPON_COLLISION_PACKET*>(p);

		// 이벤트를 생성한다
		CollisionEvent event{};
		event.user_id = packet->id;
		event.bounding_box.Center.x = packet->x;
		event.bounding_box.Center.y = packet->y;
		event.bounding_box.Center.z = packet->z;
		event.bounding_box.Extents = cl.m_weopon_bounding_box.Extents;
		event.attack_type = packet->attack_type;
		event.collision_type = packet->collision_type;
		event.end_time = packet->end_time;

		// 이벤트를 해당 플레이어가 속한 던전에 추가한다
		g_dungeon_manager->SetEvent(event, packet->id);


		break;
	}

	case CS_PACKET_CHANGE_ANIMATION:
	{
		CS_CHANGE_ANIMATION_PACKET* animation_packet = reinterpret_cast<CS_CHANGE_ANIMATION_PACKET*>(p);

		SC_CHANGE_ANIMATION_PACKET packet;
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_CHANGE_ANIMATION;
		packet.id = animation_packet->id;
		packet.animation_type = animation_packet->animation_type;

		char buf[sizeof(packet)];
		memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
		WSABUF wsa_buf{ sizeof(buf), buf };
		DWORD sent_byte;

		for (const auto& cl : m_clients) {
			if (!cl.m_player_data.active_check) continue;
			if (cl.m_player_data.id == id) continue;
			const int retval = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_ATTACK_PACKET) Error");
		}

		break;
	}

	}
}


void Server::Disconnect(const int id)
{
	Session& cl = m_clients[id];
	cl.m_lock.lock();
	cl.m_player_data.id = 0;
	cl.m_player_data.active_check = false;
	cl.m_player_data.pos = {};
	cl.m_player_data.velocity = {};
	cl.m_player_data.yaw = 0.0f;
	closesocket(cl.m_socket);
	cl.m_lock.unlock();
}

void Server::SendLoginOkPacket(const Session& player) const
{
	SC_LOGIN_OK_PACKET login_ok_packet{};
	login_ok_packet.size = sizeof(login_ok_packet);
	login_ok_packet.type = SC_PACKET_LOGIN_OK;
	login_ok_packet.player_data.id = player.m_player_data.id;
	login_ok_packet.player_data.active_check = true;
	login_ok_packet.player_data.pos.x = player.m_player_data.pos.x;
	login_ok_packet.player_data.pos.y = player.m_player_data.pos.y;
	login_ok_packet.player_data.pos.z = player.m_player_data.pos.z;
	login_ok_packet.player_data.hp = player.m_player_data.hp;
	login_ok_packet.player_type = player.m_player_type;
	strcpy_s(login_ok_packet.name, sizeof(login_ok_packet.name), player.m_name);

	char buf[sizeof(login_ok_packet)];
	memcpy(buf, reinterpret_cast<char*>(&login_ok_packet), sizeof(login_ok_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	const int retval = WSASend(player.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
	if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_LOGIN_OK_PACKET) Error");

	login_ok_packet.type = SC_PACKET_ADD_PLAYER;
	memcpy(buf, reinterpret_cast<char*>(&login_ok_packet), sizeof(login_ok_packet));

	// 현재 접속해 있는 모든 클라이언트들에게 새로 로그인한 클라이언트들의 정보를 전송
	for (const auto& other : m_clients)
	{
		if (!other.m_player_data.active_check) continue;
		if (player.m_player_data.id == other.m_player_data.id) continue;
		cout << "sc packet1 " << endl;
		const int retval = WSASend(other.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_LOGIN_OK_PACKET) Error");
	}

	// 새로 로그인한 클라이언트에게 현재 접속해 있는 모든 클라이언트들의 정보를 전송
	for (const auto& other : m_clients)
	{
		if (!other.m_player_data.active_check) continue;
		if (static_cast<int>(other.m_player_data.id) == player.m_player_data.id) continue;

		cout << "sc packet2 " << endl;
		SC_LOGIN_OK_PACKET sub_packet{};
		sub_packet.size = sizeof(sub_packet);
		sub_packet.type = SC_PACKET_ADD_PLAYER;
		sub_packet.player_data = other.m_player_data;
		strcpy_s(sub_packet.name, sizeof(sub_packet.name), other.m_name);
		//sub_packet.ready_check = other.m_ready_check;
		sub_packet.player_type = other.m_player_type;

		memcpy(buf, reinterpret_cast<char*>(&sub_packet), sizeof(sub_packet));
		const int retval = WSASend(player.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_LOGIN_OK_PACKET) Error");
		cout << (int)other.m_player_data.id << "add player to " << (int)player.m_player_data.id << endl;
	}

	std::cout << "[ id: " << static_cast<int>(login_ok_packet.player_data.id) << " Login Packet Received ]" << std::endl;
	cout << "HP: " << login_ok_packet.player_data.hp << endl;

}

void Server::SendPlayerDataPacket()
{
	
	SC_UPDATE_CLIENT_PACKET update_packet{};
	update_packet.size = sizeof(update_packet);
	update_packet.type = SC_PACKET_UPDATE_CLIENT;
	for (int i = 0; i < MAX_USER; ++i)
		update_packet.data[i] = m_clients[i].m_player_data;

	for (int i = 1; i < MAX_USER; ++i)
	{
		if (!update_packet.data[i].active_check)
		{
			update_packet.data[i].id = -1;
		}
	}

	char buf[sizeof(update_packet)];
	memcpy(buf, reinterpret_cast<char*>(&update_packet), sizeof(update_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& cl : m_clients)
	{
		if (!cl.m_player_data.active_check) continue; 
		
		const int retval = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(cl.m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_UPDATE_CLIENT)");
		}
	}
	//cout << "작동중인 모든 클라이언트들에게 이동 결과를 알려줌" << endl;
}

void Server::SendPlayerAttackPacket(int pl_id)
{
	SC_ATTACK_PACKET attack_packet;
	attack_packet.size = sizeof(attack_packet);
	attack_packet.type = SC_PACKET_PLAYER_ATTACK;
	attack_packet.id = pl_id;
	attack_packet.key = INPUT_KEY_E;

	char buf[sizeof(attack_packet)];
	memcpy(buf, reinterpret_cast<char*>(&attack_packet), sizeof(attack_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& cl : m_clients) {
		if (!cl.m_player_data.active_check) continue;
		const int retval = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retval == SOCKET_ERROR) ErrorDisplay("Send(SC_ATTACK_PACKET) Error");
	}
}

void Server::PlayerCollisionCheck(Session& player)
{
	for (auto& cl : m_clients) {
		if (!cl.m_player_data.active_check) continue;
		if (cl.m_player_data.id == player.m_player_data.id) continue;

		if (player.m_bounding_box.Intersects(cl.m_bounding_box)) {
			CollideByStatic(player, cl.m_bounding_box);
		}
	}

	auto& v = g_dungeon_manager->GetStructures();

	for (auto& obj : v) {
		if (player.m_bounding_box.Intersects(obj->GetBoundingBox())) {
			CollideByStaticOBB(player, obj->GetBoundingBox());
		}
	}
}

void Server::Update(const float taketime)
{

	if (m_monsters.size() < MAX_MONSTER)
		CreateMonsters(taketime);


	for (auto& m : m_monsters)
		m->CreateMonster(taketime);

}

UCHAR Server::RecognizePlayer(const XMFLOAT3& mon_pos)
{
	for (const auto& pl : m_clients)
	{
		if (!pl.m_exist_check) continue;
		const float  myself{ Vector3::Length(Vector3::Sub(pl.m_player_data.pos, mon_pos)) };
		//cout << "pl - mon: " <<  myself << endl;
		if (myself < m_length)
		{
			m_length = myself;
			m_target_id = pl.m_player_data.id;
		}
	}
	return m_target_id;
}

void Server::CreateMonsters(const float taketime)
{
	//if (m_floor_mob_count[m_floor - 1] <= 0) {
	//	m_spawn_stop = g_spawm_stop;
	//	return;
	//}

	unique_ptr<Monster> monsters;

	//if (m_spawn_stop <= 0.0f) {
		switch (m_floor)
		{
		case 1:
			monsters = make_unique<WarriorMonster>();
			monsters->SetMonsterPosition();
			break;
		default:
			break;
		}
		monsters->SetId(m_next_monster_id++);
		monsters->SetTargetId(RecognizePlayer(monsters->GetPosition()));
		cout << "Monster id - " << (int)monsters->GetId() << " is created" << " / pos (x: " << monsters->GetPosition().x << " y: " << monsters->GetPosition().y
			<< " z: " << monsters->GetPosition().z << ")" << endl;
		cout << "capacity:" << m_monsters.size() << " / " << MAX_MONSTER << std::endl;


		m_monsters.push_back(move(monsters));

	/*	m_spawn_stop = g_spawm_stop;
		--m_floor_mob_count[m_floor - 1];
	}
	m_spawn_stop -= taketime;*/
}

void Server::SendMonsterAddPacket()
{
	//cout << "크기 - " << m_monsters.size() << endl;

	SC_ADD_MONSTER_PACKET monster_packet[MAX_MONSTER];

	//auto m_monsters = g_dungeon_manager->GetDungeons()[0]->GetMonsters();

	for (size_t i = 0; i < m_monsters.size(); ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		monster_packet[i].type = SC_PACKET_ADD_MONSTER;
		monster_packet[i].monster_data = m_monsters[i]->GetData();
		monster_packet[i].monster_type = m_monsters[i]->GetType();
	}
	for (size_t i = m_monsters.size(); i < MAX_MONSTER; ++i)
		monster_packet[i].monster_data = MonsterData{ .id = -1 };

	char buf[sizeof(monster_packet)];
	memcpy(buf, reinterpret_cast<char*>(&monster_packet), sizeof(monster_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;


	for (const auto& cl : m_clients)
	{
		if (!cl.m_player_data.active_check) continue;
		const int retVal = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(cl.m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_ADD_MONSTER)");
		}
	}


	//auto& m_clients = g_dungeon_manager->GetDungeons()[0]->GetPlayers();
	/*for (const auto& data : m_clients)
	{
		if (!data.second->m_player_data.active_check) continue;
		const int retVal = WSASend(data.second->m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(data.second->m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_ADD_MONSTER)");
		}
	}*/
}


void Server::SendMonsterDataPacket()
{
	//cout << "크기 - " << m_monsters.size() << endl;

	//auto m_monsters = g_dungeon_manager->GetDungeons()[0]->GetMonsters();

	SC_MONSTER_UPDATE_PACKET monster_packet[MAX_MONSTER];

	for (size_t i = 0; i < m_monsters.size(); ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_MONSTER_UPDATE_PACKET));
		monster_packet[i].type = SC_PACKET_UPDATE_MONSTER;
		monster_packet[i].monster_data = m_monsters[i]->GetData();
	}
	for (size_t i = m_monsters.size(); i < MAX_MONSTER; ++i)
		monster_packet[i].monster_data = MonsterData{ .id = -1 };

	char buf[sizeof(monster_packet)];
	memcpy(buf, reinterpret_cast<char*>(&monster_packet), sizeof(monster_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	

	for (const auto& cl : m_clients)
	{
		if (!cl.m_player_data.active_check) continue;
		const int retVal = WSASend(cl.m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(cl.m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_UPDATE_MONSTER)");
		}
	}

	/*auto& m_clients = g_dungeon_manager->GetDungeons()[0]->GetPlayers();
	for (const auto& data : m_clients)
	{
		if (!data.second->m_player_data.active_check) continue;
		const int retVal = WSASend(data.second->m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(data.second->m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_UPDATE_MONSTER)");
		}
	}*/
}

CHAR Server::GetNewId() const
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == m_clients[i].m_player_data.active_check)
		{
			return i;
		}
	}
	std::cout << "Maximum Number of Clients" << std::endl;
	return -1;
}

void Server::MovePlayer(Session& player, XMFLOAT3 pos)
{
	player.m_player_data.pos = pos;

	// 바운드 박스 갱신
	player.m_bounding_box.Center = pos;
}

void Server::Timer()
{
	using namespace chrono;
	TimerEvent event;
	event.start_time = std::chrono::system_clock::now() + std::chrono::seconds(3);
	while (m_start_cool_time < 6) {
			auto attack_end_time = std::chrono::system_clock::now();
			auto sec = std::chrono::duration_cast<std::chrono::seconds>(attack_end_time - event.start_time);
			if (m_attack_check == true) {
				if (sec.count() > m_start_cool_time)
				{
					m_start_cool_time++;
					m_remain_cool_time = m_end_cool_time - m_start_cool_time;
					cout << "남은 스킬 쿨타임: " << m_remain_cool_time << "초" << endl;
				}
				else if (m_start_cool_time == 5)
				{
					m_attack_check = false;
					m_start_cool_time = 0;
					SendPlayerAttackPacket(0);
					event.start_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
					attack_end_time = std::chrono::system_clock::now();
				}
			}
		}
}

void Server::RotatePlayer(Session& player, FLOAT yaw)
{
	// 플레이어 회전, degree 값
	player.m_player_data.yaw = yaw;

	float radian = XMConvertToRadians(yaw);
	
	XMFLOAT4 q{};
	XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(0.f, radian, 0.f));

	// 바운드 박스 회전
	player.m_bounding_box.Orientation = XMFLOAT4{q.x, q.y, q.z, q.w};
}

void Server::CollideByStatic(Session& player, const BoundingOrientedBox& obb)
{
	BoundingOrientedBox& player_obb = player.m_bounding_box;

	// length 는 바운드 박스의 길이 합
	// dist 는 바운드 박스간의 거리
	FLOAT x_length = player_obb.Extents.x + obb.Extents.x;
	FLOAT x_dist = abs(player_obb.Center.x - obb.Center.x);

	FLOAT z_length = player_obb.Extents.z + obb.Extents.z;
	FLOAT z_dist = abs(player_obb.Center.z - obb.Center.z);

	// 겹치는 정도
	FLOAT x_bias = x_length - x_dist;
	FLOAT z_bias = z_length - z_dist;

	XMFLOAT3 pos = player.m_player_data.pos;

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

void Server::CollideByMoveMent(Session& player1, Session& player2)
{
	// 충돌한 플레이어의 속도
	XMFLOAT3 velocity = player1.m_player_data.velocity;

	// 충돌된 오브젝트의 위치
	XMFLOAT3 pos = player2.m_player_data.pos;

	// 충돌된 오브젝트를 충돌한 플레이어의 속도만큼 밀어냄
	// 밀어내는 방향 고려할 필요 있음 ( 아직 X )
	pos.x += velocity.x;
	pos.y += velocity.y;
	pos.z += velocity.z;
	MovePlayer(player2, pos);
}

void Server::CollideByStaticOBB(Session& player, const BoundingOrientedBox& obb)
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

	player.m_bounding_box.GetCorners(corners);

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
		MovePlayer(player, Vector3::Add(player.m_player_data.pos, v));
		

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
 
