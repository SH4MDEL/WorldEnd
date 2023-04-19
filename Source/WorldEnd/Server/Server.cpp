#include "Server.h"
#include "stdafx.h"

Server& Server::GetInstance()
{
	static Server instance;
	return instance;
}

Server::Server()
{	
	// ------- 초기화 ------- //
	for (size_t i = 0; i < MAX_USER; ++i) {
		m_clients[i] = std::make_shared<Client>();
	}

	for (size_t i = WARRIOR_MONSTER_START; i < WARRIOR_MONSTER_END; ++i) {
		m_clients[i] = std::make_shared<WarriorMonster>();
	}

	for (size_t i = ARCHER_MONSTER_START; i < ARCHER_MONSTER_END; ++i) {
		m_clients[i] = std::make_shared<ArcherMonster>();
	}

	for (size_t i = WIZARD_MONSTER_START; i < WIZARD_MONSTER_END; ++i) {
		m_clients[i] = std::make_shared<WizardMonster>();
	}

	for (size_t i = 0; i < MAX_ARROW_RAIN; ++i) {
		m_triggers[i] = std::make_shared<ArrowRain>();
		m_triggers[i]->SetId(i);
	}

	m_game_room_manager = std::make_unique<GameRoomManager>();

	// ----------------------- //
}

Server::~Server()
{
	for (size_t i = 0; i < MAX_USER; ++i) {
		if (State::INGAME == m_clients[i]->GetState())
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

	std::thread thread1{ &Server::Timer,this };
	thread1.detach();

	constexpr int MAX_FAME = 60;
	using frame = std::chrono::duration<int32_t, std::ratio<1, MAX_FAME>>;
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
		if (frame_count.count() >= MAX_FAME) {
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
			std::cout << "GQCS Error on client[" << static_cast<INT>(key) << "]\n";
			Disconnect(static_cast<int>(key));
			if (OP_SEND == exp_over->_comp_type)
				delete exp_over;
			continue;
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
				Disconnect(id);
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

		case OP_COOLDOWN_RESET: {
			// 해당 클라이언트의 쿨타임 초기화
			SC_RESET_COOLDOWN_PACKET packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_RESET_COOLDOWN;
			packet.cooldown_type = static_cast<ActionType>(exp_over->_send_buf[0]);

			m_clients[static_cast<int>(key)]->DoSend(&packet);

			delete exp_over;
			break;
		}
		case OP_MONSTER_REMOVE: {
			int monster_id = static_cast<int>(key);
			auto monster = dynamic_pointer_cast<Monster>(m_clients[monster_id]);
			int room_num = monster->GetRoomNum();

			SC_REMOVE_MONSTER_PACKET packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_REMOVE_MONSTER;
			packet.id = monster_id;

			m_game_room_manager->RemoveMonster(monster_id);
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				break;
			}

			auto& ids = game_room->GetPlayerIds();
			for (int id : ids) {
				if (-1 == id) continue;

				m_clients[id]->DoSend(&packet);
			}
			delete exp_over;
			break;
		}
		case OP_FLOOR_CLEAR: {
			int room_num = static_cast<int>(key);
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				break;
			}

			auto now = std::chrono::system_clock::now();
			auto exec = std::chrono::duration_cast<std::chrono::milliseconds>
				(now - game_room->GetStartTime());
			USHORT reward = static_cast<USHORT>(exec.count() / 100);

			SC_CLEAR_FLOOR_PACKET packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_CLEAR_FLOOR;
			packet.reward = reward;

			auto& ids = game_room->GetPlayerIds();
			for (int id : ids) {
				if (-1 == id) continue;

				m_clients[id]->DoSend(&packet);
			}

			{
				std::lock_guard<std::mutex> l{ game_room->GetStateMutex() };
				game_room->SetState(GameRoomState::CLEAR);
			}

			delete exp_over;
			break;
		}
		case OP_FLOOR_FAIL: {
			int room_num = static_cast<int>(key);
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				break;
			}

			SC_FAIL_FLOOR_PACKET packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_CLEAR_FLOOR;

			auto& ids = game_room->GetPlayerIds();
			for (int id : ids) {
				if (-1 == id) continue;

				m_clients[id]->DoSend(&packet);
			}
			delete exp_over;
			break;
		}
		case OP_BEHAVIOR_CHANGE: {
			auto monster = dynamic_pointer_cast<Monster>(m_clients[static_cast<int>(key)]);
			monster->ChangeBehavior(static_cast<MonsterBehavior>(exp_over->_send_buf[0]));

			delete exp_over;
			break;
		}
		case OP_AGRO_REDUCE: {
			auto monster = dynamic_pointer_cast<Monster>(m_clients[static_cast<int>(key)]);
			monster->DecreaseAggroLevel();

			delete exp_over;
			break;
		}
		case OP_ATTACK_COLLISION: {
			auto client = dynamic_pointer_cast<Client>(m_clients[static_cast<int>(key)]);
			ActionType attack_type = static_cast<ActionType>(exp_over->_send_buf[0]);
			XMFLOAT3* pos = reinterpret_cast<XMFLOAT3*>(&exp_over->_send_buf[1]);

			int room_num = client->GetRoomNum();
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				break;
			}

			auto& monster_ids = game_room->GetMonsterIds();
			auto& player_ids = game_room->GetPlayerIds();

			BoundingOrientedBox obb{};
			
			// 전사 공격은 무기 바운드 박스 따라가도록 하면 됨
			if (PlayerType::WARRIOR == client->GetPlayerType()) {

				// 무기의 바운드 박스는 항상 회전할 필요 없이 공격이 일어날때만 해주면 됨
				float yaw = client->GetYaw();
				float radian = XMConvertToRadians(yaw);
				XMFLOAT4 q{};
				XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(0.f, radian, 0.f));
				client->SetWeaponOrientation(q);

				switch (attack_type) {
				case ActionType::NORMAL_ATTACK:
					obb = client->GetWeaponBoundingBox();
					obb.Center = *pos;
					break;

				case ActionType::SKILL:
					obb = client->GetWeaponBoundingBox();
					obb.Center = *pos;
					break;

				case ActionType::ULTIMATE:
					obb = client->GetWeaponBoundingBox();
					obb.Center = *pos;
					obb.Extents = PlayerSetting::
						ULTIMATE_EXTENT[static_cast<int>(client->GetPlayerType())];
					break;
				}
			}

			// 충돌된 몬스터 빼내는 컨테이너
			std::vector<int> v;
			v.reserve(MAX_INGAME_MONSTER);
			FLOAT damage{ client->GetDamage() };
			damage *= client->GetSkillRatio(attack_type);

			for (int id : monster_ids) {
				if (-1 == id) continue;
				if (State::INGAME != m_clients[id]->GetState()) continue;
				
				if (m_clients[id]->GetBoundingBox().Intersects(obb)) {
					auto monster = dynamic_pointer_cast<Monster>(m_clients[id]);

					monster->DecreaseHp(damage, client->GetId());
					v.push_back(id);

				}
			}

			SendMonsterHit(static_cast<int>(key), player_ids, v);

			delete exp_over;
			break;
		}
		case OP_MONSTER_ATTACK_COLLISION: {
			auto monster = dynamic_pointer_cast<WarriorMonster>(m_clients[static_cast<int>(key)]);
			ActionType attack_type = static_cast<ActionType>(exp_over->_send_buf[0]);
			XMFLOAT3* pos = reinterpret_cast<XMFLOAT3*>(&exp_over->_send_buf[1]);		// 공격 충돌 위치

			int room_num = monster->GetRoomNum();
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				break;
			}

			auto& player_ids = game_room->GetPlayerIds();

			BoundingOrientedBox obb{};
			float damage{ 0.f };	// 스킬이 있다면 계수 더하는 용도

			if (MonsterType::WARRIOR == monster->GetMonsterType()) {
				obb.Center = *pos;
				obb.Extents = XMFLOAT3{ 0.2f, 0.2f, 0.2f };
				damage = monster->GetDamage();
			}

			SendMonsterAttack(static_cast<int>(key), player_ids, obb);

			delete exp_over;
			break;
		}
		case OP_STAMINA_CHANGE: {
			int id = static_cast<int>(key);
			auto client = dynamic_pointer_cast<Client>(m_clients[id]);
			bool is_stamina_increase = static_cast<bool>(exp_over->_send_buf[0]);
			BYTE latest_id = static_cast<BYTE>(exp_over->_send_buf[1]);
			INT* amount = reinterpret_cast<INT*>(&exp_over->_send_buf[2]);

			TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + std::chrono::seconds(1),
				.obj_id = id, .event_type = EventType::STAMINA_CHANGE, .latest_id = latest_id };
			
			SC_CHANGE_STAMINA_PACKET packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_CHANGE_STAMINA;

			if (is_stamina_increase) {
				// 스태미너 증가는 달리는중이면 반복X
				client->ChangeStamina(static_cast<FLOAT>(*amount));

				if (PlayerAnimation::RUN == client->GetCurrentAnimation() ||
					PlayerAnimation::DASH == client->GetCurrentAnimation())
				{
					delete exp_over;
					break;
				}

				// 달리는중이 아니면 최대에 도달했을 때 끝내기
				if (client->GetStamina() >= PlayerSetting::MAX_STAMINA) {
					client->SetStamina(PlayerSetting::MAX_STAMINA);
					packet.stamina = PlayerSetting::MAX_STAMINA;
					client->DoSend(&packet);
					delete exp_over;
					break;
				}
				ev.targat_id = PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;
				ev.is_stamina_increase = true;
			}
			else {
				// 달리는중이 아니면 감소 끝냄
				client->ChangeStamina(static_cast<FLOAT>(*amount));
				if (PlayerAnimation::RUN != client->GetCurrentAnimation() &&
					PlayerAnimation::DASH != client->GetCurrentAnimation() &&
					PlayerAnimation::ROLL != client->GetCurrentAnimation())
				{
					packet.stamina = client->GetStamina();
					client->DoSend(&packet);
					delete exp_over;
					break;
				}
				ev.targat_id = -PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;
				ev.is_stamina_increase = false;
			}
			m_timer_queue.push(ev);

			packet.stamina = client->GetStamina();
			client->DoSend(&packet);

			delete exp_over;
			break;
		}
		case OP_HIT_SCAN: {
			int id = static_cast<int>(key);
			int room_num = m_clients[id]->GetRoomNum();
			int* arrow_id = reinterpret_cast<int*>(&exp_over->_send_buf);
			ActionType* action_type = reinterpret_cast<ActionType*>((exp_over->_send_buf + sizeof(int)));
			XMFLOAT3* position = reinterpret_cast<XMFLOAT3*>((exp_over->_send_buf + sizeof(int) + sizeof(char)));
			XMFLOAT3* direction = reinterpret_cast<XMFLOAT3*>(
				(exp_over->_send_buf + sizeof(int) + sizeof(char) + sizeof(XMFLOAT3)));

			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				break;
			}

			std::span<int> ids{};
			if (IsPlayer(id)) {
				ids = game_room->GetMonsterIds();
				// 플레이어면 충돌대상은 몬스터이므로 ids는 몬스터
			}
			else {
				ids = game_room->GetPlayerIds();
				// 플레이어가 아니면 충돌대상은 플레이어이므로 ids는 플레이어
			}

			// 화살이 날아갈 거리, 임시값 30
			float length{ 30.f };
			float near_dist{std::numeric_limits<float>::max()};
			int near_id{ -1 };
			XMVECTOR pos{ XMLoadFloat3(position) };
			XMVECTOR dir{ XMLoadFloat3(direction) };

			// 광선 충돌 검사 후 가장 가까운 id 구함
			for (int client_id : ids) {
				if (-1 == client_id) continue;
				if (State::DEATH == m_clients[client_id]->GetState()) continue;

				auto& obb = m_clients[client_id]->GetBoundingBox();
				if (obb.Intersects(pos, dir, length)) {
					float dist{ Vector3::Length(Vector3::Sub(m_clients[client_id]->GetPosition(), m_clients[id]->GetPosition())) };
					if (dist < near_dist ) {
						near_dist = dist;
						near_id = client_id;
					}
				}
			}

			// 빗나감
			if (-1 == near_id) {
				delete exp_over;
				break;
			}

			float damage = m_clients[id]->GetDamage();
			if (IsPlayer(id)) {
				SendRemoveArrow(id, *arrow_id);

				damage *= m_clients[id]->GetSkillRatio(*action_type);
				m_clients[near_id]->DecreaseHp(damage, id);
				auto& player_ids = game_room->GetPlayerIds();
				SendMonsterHit(id, player_ids, near_id);
			}
			else {
				SendMonsterAttack(id, near_id);
			}
			
			delete exp_over;
			break;
		}
		case OP_ARROW_SHOOT: {
			int id = static_cast<int>(key);
			ActionType* type = reinterpret_cast<ActionType*>(exp_over->_send_buf);
			
			auto game_room = m_game_room_manager->GetGameRoom(m_clients[id]->GetRoomNum());
			int arrow_id = game_room->GetArrowId();

			switch (*type) {
			case ActionType::NORMAL_ATTACK:
			case ActionType::SKILL: 
			{
				SetRemoveArrowTimerEvent(id, arrow_id);

				int target = GetNearTarget(id, PlayerSetting::AUTO_TARET_RANGE);
				SendPlayerShoot(id, arrow_id, target);
				SetHitScanTimerEvent(id, target, *type, arrow_id);
				break;
			}
			case ActionType::ULTIMATE: {
				int trigger_id = GetNewTriggerId(TriggerType::ARROW_RAIN);
				auto trigger = dynamic_pointer_cast<ArrowRain>(m_triggers[trigger_id]);

				// 위치, 데미지 설정 이후 생성하고, map 에 해당 트리거를 추가 함
				float damage = m_clients[id]->GetDamage() * 
					m_clients[id]->GetSkillRatio(ActionType::ULTIMATE);
				trigger->Create(damage, id);
				game_room->AddTrigger(trigger_id);

				break;
			}
			}

			delete exp_over;
			break;
		}
		case OP_ARROW_REMOVE: {
			int id = static_cast<int>(key);
			int* arrow_id = reinterpret_cast<int*>(exp_over->_send_buf);

			SendRemoveArrow(id, *arrow_id);
			delete exp_over;
			break;
		}
		case OP_GAME_ROOM_RESET: {
			printf("방 초기화!\n");
			int room_num = static_cast<int>(key);
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				return;
			}

			// 방의 모든 몬스터들을 FREE로 만들고 배열을 -1 로 초기화
			auto& ids = game_room->GetMonsterIds();
			for (int& id : ids) {
				if (-1 == id) continue;

				std::lock_guard<std::mutex> lock{ m_clients[id]->GetStateMutex() };
				m_clients[id]->SetState(State::FREE);
				id = -1;
			}
			game_room->SetMonsterCount(0);

			// 방을 비어 있는 상태로 변경
			std::lock_guard<std::mutex> lock{ game_room->GetStateMutex() };
			game_room->SetState(GameRoomState::EMPTY);

			delete exp_over;
			break;
		}

		}
	}
}

void Server::ProcessPacket(int id, char* p) 
{
	unsigned char type = p[1];
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);
	
	switch (type){
	case CS_PACKET_LOGIN: {
		CS_LOGIN_PACKET* packet = reinterpret_cast<CS_LOGIN_PACKET*>(p);
		client->SetPlayerType(packet->player_type);
		client->SetName(packet->name);
		{
			std::lock_guard<std::mutex> lock{ client->GetStateMutex() };
			client->SetState(State::INGAME);
		}
		SendLoginOk(id);

		// 원래는 던전 진입 시 던전에 배치해야하지만
		// 현재 마을이 없이 바로 던전에 진입하므로 던전에 입장시킴
		client->SetRoomNum(0);
		m_game_room_manager->SetPlayer(client->GetRoomNum(), id);
		m_game_room_manager->SendAddMonster(client->GetRoomNum(), id);

		std::cout << client->GetId() << " is connect" << std::endl;
		break;
	}
	case CS_PACKET_PLAYER_MOVE: {
		CS_PLAYER_MOVE_PACKET* packet = reinterpret_cast<CS_PLAYER_MOVE_PACKET*>(p);

		// 위치는 서버에서 저장하므로 굳이 받을 필요는 없을 것
		// 속도만 받아와서 처리해도 됨
		client->SetVelocity(packet->velocity);

		XMFLOAT3 pos = packet->pos;
		pos.x += packet->velocity.x;
		pos.y += packet->velocity.y;
		pos.z += packet->velocity.z;
		client->SetYaw(packet->yaw);

		Move(client, pos);
		break;
	}
	case CS_PACKET_SET_COOLDOWN: {
		CS_COOLDOWN_PACKET* packet = reinterpret_cast<CS_COOLDOWN_PACKET*>(p);

		SetCooldownTimerEvent(id, packet->cooldown_type);
		break;
	}
	case CS_PACKET_ATTACK: {
		CS_ATTACK_PACKET* packet = reinterpret_cast<CS_ATTACK_PACKET*>(p);

		// 충돌 위치, 시간을 서버에서 결정
		switch (client->GetPlayerType()) {
		case PlayerType::WARRIOR:
			SetAttackTimerEvent(id, packet->attack_type, packet->attack_time);
			break;

		case PlayerType::ARCHER:
			SetArrowShootTimerEvent(id, packet->attack_type, packet->attack_time);
			break;
		}


		SetCooldownTimerEvent(id, packet->attack_type);
		break;
	}
	case CS_PACKET_CHANGE_ANIMATION: {
		CS_CHANGE_ANIMATION_PACKET* packet = 
			reinterpret_cast<CS_CHANGE_ANIMATION_PACKET*>(p);

		client->SetCurrentAnimation(packet->animation);

		SendChangeAnimation(id, packet->animation);
		break;
	}
	case CS_PACKET_CHANGE_STAMINA: {
		CS_CHANGE_STAMINA_PACKET* packet =
			reinterpret_cast<CS_CHANGE_STAMINA_PACKET*>(p);

		SetStaminaTimerEvent(id, packet->is_increase);
		break;
	}
	case CS_PACKET_INTERACT_OBJECT: {
		CS_INTERACT_OBJECT_PACKET* packet =
			reinterpret_cast<CS_INTERACT_OBJECT_PACKET*>(p);

		// 해당 아이디에 타입에 맞는 상호작용 처리
		switch (packet->interactable_type) {
		case InteractableType::BATTLE_STARTER:
			client->SetInteractable(false);
			m_game_room_manager->StartBattle(client->GetRoomNum());
			break;
		case InteractableType::PORTAL:
			client->SetInteractable(false);
			m_game_room_manager->WarpNextFloor(client->GetRoomNum());
			break;
		case InteractableType::ENHANCMENT:

			break;
		case InteractableType::RECORD_BOARD:

			break;
		}

		break;
	}

	}
}


void Server::Disconnect(int id)
{
	auto cl = dynamic_pointer_cast<Client>(m_clients[id]);
	if (m_game_room_manager->IsValidRoomNum(m_clients[id]->GetRoomNum())) {
		m_game_room_manager->RemovePlayer(cl->GetId());
	}
	else {
		// 마을 내 플레이어 제거
	}
	cl->SetId(-1);
	cl->SetRoomNum(-1);
	cl->SetPosition(0.f, 0.f, 0.f);
	cl->SetVelocity(0.f, 0.f, 0.f);
	cl->SetYaw(0.f);
	cl->SetStamina(PlayerSetting::MAX_STAMINA);
	cl->SetTriggerFlag();

	cl->SetRemainSize(0);
	ExpOver& ex_over = cl->GetExpOver();
	ex_over._comp_type = OP_RECV;
	ex_over._wsa_buf.buf = ex_over._send_buf;
	ex_over._wsa_buf.len = BUF_SIZE;
	ZeroMemory(&ex_over._wsa_over, sizeof(ex_over._wsa_over));
	
	closesocket(cl->GetSocket());

	std::lock_guard<std::mutex> lock{ cl->GetStateMutex() };
	cl->SetState(State::FREE);
}

// 마을, 던전 구분 필요
void Server::SendLoginOk(int client_id)
{
	SC_LOGIN_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.player_data.id = m_clients[client_id]->GetId();
	packet.player_data.pos = m_clients[client_id]->GetPosition();
	packet.player_data.hp = m_clients[client_id]->GetHp();
	packet.player_type = m_clients[client_id]->GetPlayerType();
	strcpy_s(packet.name, sizeof(packet.name), m_clients[client_id]->GetName().c_str());

	m_clients[client_id]->DoSend(&packet);

	packet.type = SC_PACKET_ADD_OBJECT;

	// 현재 접속해 있는 모든 클라이언트들에게 새로 로그인한 클라이언트들의 정보를 전송
	for (size_t i = 0; i < MAX_USER; ++i) {
		if (-1 == m_clients[i]->GetId()) continue;
		if (m_clients[client_id]->GetId() == m_clients[i]->GetId()) continue;
		
		m_clients[i]->DoSend(&packet);
	}

	// 새로 로그인한 클라이언트에게 현재 접속해 있는 모든 클라이언트들의 정보를 전송
	for (size_t i = 0; i < MAX_USER; ++i){
		if (-1 == m_clients[i]->GetId()) continue;
		if (m_clients[client_id]->GetId() == m_clients[i]->GetId()) continue;

		SC_LOGIN_OK_PACKET sub_packet{};
		sub_packet.size = sizeof(sub_packet);
		sub_packet.type = SC_PACKET_ADD_OBJECT;
		sub_packet.player_data = m_clients[i]->GetPlayerData();
		strcpy_s(sub_packet.name, sizeof(sub_packet.name), m_clients[i]->GetName().c_str());
		sub_packet.player_type = m_clients[i]->GetPlayerType();

		m_clients[client_id]->DoSend(&sub_packet);
	}
}

void Server::SendMoveInGameRoom(int client_id, int room_num)
{
	if (-1 == client_id)
		return;

	SC_UPDATE_CLIENT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_CLIENT;
	packet.data = m_clients[client_id]->GetPlayerData();

	auto game_room = m_game_room_manager->GetGameRoom(room_num);
	if (!game_room)
		return;

	auto& ids = game_room->GetPlayerIds();

	for (int id : ids) {
		if (-1 == id) continue;

		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendPlayerDeath(int client_id)
{
	SC_PLAYER_DEATH_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PLAYER_DEATH;
	packet.id = client_id;

	auto game_room = m_game_room_manager->GetGameRoom(m_clients[client_id]->GetRoomNum());
	if (!game_room)
		return;

	auto& ids = game_room->GetPlayerIds();
	for (int id : ids) {
		if (-1 == id) continue;

		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendChangeAnimation(int client_id, USHORT animation)
{
	SC_CHANGE_ANIMATION_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_ANIMATION;
	packet.id = client_id;
	packet.animation = animation;

	int room_num = m_clients[client_id]->GetRoomNum();
	// 마을, 게임룸 구분하여 보낼 필요 있음
	if (m_game_room_manager->IsValidRoomNum(room_num)) {
		auto game_room = m_game_room_manager->GetGameRoom(room_num);
		if (!game_room)
			return;

		auto& ids = game_room->GetPlayerIds();
		for (int id : ids) {
			if (-1 == id) continue;
			//if (id == client_id) continue;

			m_clients[id]->DoSend(&packet);
		}
	}
	// 마을 처리
	else {
		
	}
}

void Server::SendMonsterHit(int client_id, const std::span<int>& receiver,
	const std::span<int>& creater)
{
	SC_CREATE_MONSTER_HIT packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MONSTER_HIT;

	for (int id : receiver) {
		if (-1 == id) continue;
		if (State::FREE == m_clients[id]->GetState() ||
			State::ACCEPT == m_clients[id]->GetState()) 
			continue;

		for (int client_id : creater) {
			packet.id = client_id;
			packet.hp = m_clients[client_id]->GetHp();
			m_clients[id]->DoSend(&packet);
		}
	}
}

void Server::SendMonsterHit(int client_id, const std::span<int>& receiver, int hit_id)
{
	SC_CREATE_MONSTER_HIT packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MONSTER_HIT;
	packet.id = hit_id;
	packet.hp = m_clients[hit_id]->GetHp();

	for (int id : receiver) {
		if (-1 == id) continue;
		if (State::FREE == m_clients[id]->GetState() ||
			State::ACCEPT == m_clients[id]->GetState())
			continue;

		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendMonsterAttack(int client_id, const std::span<int>& clients,
	const BoundingOrientedBox& obb)
{
	SC_MONSTER_ATTACK_COLLISION_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MONSTER_ATTACK_COLLISION;
	for (int i = 0; i < MAX_INGAME_USER; ++i) {
		packet.ids[i] = -1;
		packet.hps[i] = 0;
	}

	for (size_t i = 0; int id : clients) {
		if (-1 == id) continue;
		if (State::INGAME != m_clients[id]->GetState()) continue;

		// 충돌했을 경우에만 index 증가
		if (obb.Intersects(m_clients[id]->GetBoundingBox())) {
			m_clients[id]->DecreaseHp(m_clients[client_id]->GetDamage(), -1);
			// player의 DecreaseHp의 2번째인자는 함수 내에서 사용 X, 아무값 작성한 것

			// 플레이어가 피격 후 죽었다면
			if (State::DEATH == m_clients[id]->GetState()) {
				SendPlayerDeath(id);
			}
			else {
				packet.ids[i] = id;
				packet.hps[i] = m_clients[id]->GetHp();
			}
			++i;
		}
	}

	if (-1 != packet.ids[0]) {
		for (int id : clients) {
			if (-1 == id) break;

			m_clients[id]->DoSend(&packet);
		}
	}
}

void Server::SendMonsterAttack(int monster_id, int player_id)
{
	SC_MONSTER_ATTACK_COLLISION_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MONSTER_ATTACK_COLLISION;
	for (int i = 0; i < MAX_INGAME_USER; ++i) {
		packet.ids[i] = -1;
		packet.hps[i] = 0;
	}

	m_clients[player_id]->DecreaseHp(m_clients[monster_id]->GetDamage(), -1);
	// player의 DecreaseHp의 2번째인자는 함수 내에서 사용 X, 아무값 작성한 것

	// 플레이어가 피격 후 죽었다면
	if (State::DEATH == m_clients[player_id]->GetState()) {
		SendPlayerDeath(player_id);
	}
	else {
		packet.ids[0] = player_id;
		packet.hps[0] = m_clients[player_id]->GetHp();
	}

	auto game_room = m_game_room_manager->GetGameRoom(m_clients[player_id]->GetRoomNum());
	if (!game_room) {
		return;
	}

	auto& clients = game_room->GetPlayerIds();
	if (-1 != packet.ids[0]) {
		for (int id : clients) {
			if (-1 == id) break;

			m_clients[id]->DoSend(&packet);
		}
	}
}

void Server::SendPlayerShoot(int client_id, int arrow_id, int target_id)
{
	SC_PLAYER_SHOOT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PLAYER_SHOOT;
	packet.id = client_id;
	packet.arrow_id = arrow_id;
	packet.target_id = target_id;

	int room_num = m_clients[client_id]->GetRoomNum();
	if (m_game_room_manager->IsValidRoomNum(room_num)) {
		auto game_room = m_game_room_manager->GetGameRoom(room_num);
		if (!game_room)
			return;

		auto& ids = game_room->GetPlayerIds();

		for (int id : ids) {
			if (-1 == id) continue;

			m_clients[id]->DoSend(&packet);
		}
	}
	else {

	}
}

void Server::SendRemoveArrow(int client_id, int arrow_id)
{
	SC_REMOVE_ARROW_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_ARROW;
	packet.arrow_id = arrow_id;

	int room_num = m_clients[client_id]->GetRoomNum();

	if (m_game_room_manager->IsValidRoomNum(room_num)) {
		auto game_room = m_game_room_manager->GetGameRoom(room_num);
		if (!game_room)
			return;

		auto& ids = game_room->GetPlayerIds();
		for (int id : ids) {
			if (-1 == id) continue;

			m_clients[id]->DoSend(&packet);
		}
	}
	else {

	}
}

bool Server::IsPlayer(int client_id)
{
	if (0 <= client_id && client_id < MAX_USER)
		return true;
	return false;
}

void Server::GameRoomPlayerCollisionCheck(const std::shared_ptr<Client>& player,
	int room_num)
{
	BoundingOrientedBox& player_obb = player->GetBoundingBox();

	auto game_room = m_game_room_manager->GetGameRoom(player->GetRoomNum());
	if (!game_room)
		return;

	auto& player_ids = game_room->GetPlayerIds();
	auto& monster_ids = game_room->GetMonsterIds();

	CollideObject(player, monster_ids, Server::CollideByStatic);
	CollideObject(player, player_ids, Server::CollideByStatic);

	auto& v = m_game_room_manager->GetStructures();

	for (auto& obj : v) {
		auto& obb = obj->GetBoundingBox();
		if (player_obb.Intersects(obb)) {
			CollideByStaticOBB(player, obj);
		}
	}

	// 포탈, 전투 오브젝트와 상호작용
	m_game_room_manager->EventCollisionCheck(player->GetRoomNum(), player->GetId());
}

INT Server::GetNewId()
{
	for (size_t i = 0; i < MAX_USER; ++i) {
		std::lock_guard<std::mutex> lock{ m_clients[i]->GetStateMutex() };
		if (State::FREE == m_clients[i]->GetState()) {
			m_clients[i]->SetState(State::ACCEPT);
			return i;
		}
	}

	return -1;
}

INT Server::GetNewMonsterId(MonsterType type)
{
	INT start_num{}, end_num{};
	switch (type) {
	case MonsterType::WARRIOR:
		start_num = WARRIOR_MONSTER_START;
		end_num = WARRIOR_MONSTER_END;
		break;
	case MonsterType::ARCHER:
		start_num = ARCHER_MONSTER_START;
		end_num = ARCHER_MONSTER_END;
		break;
	case MonsterType::WIZARD:
		start_num = WIZARD_MONSTER_START;
		end_num = WIZARD_MONSTER_END;
		break;
	}

	for (size_t i = start_num; i < end_num; ++i) {
		std::lock_guard<std::mutex> lock{ m_clients[i]->GetStateMutex() };
		if (State::FREE == m_clients[i]->GetState()) {
			m_clients[i]->SetState(State::ACCEPT);
			return i;
		}
	}

	return -1;
}

INT Server::GetNewTriggerId(TriggerType type)
{
	INT start_num{}, end_num{};
	switch (type) {
	case TriggerType::ARROW_RAIN:
		start_num = 0;
		end_num = MAX_ARROW_RAIN;
		break;
	}

	// 트리거 생성은 while 을 통해 반드시 보장을 해주어야 하는지?
	for (size_t i = start_num; i < end_num; ++i) {
		std::lock_guard<std::mutex> lock{ m_triggers[i]->GetStateMutex() };
		if (State::FREE == m_triggers[i]->GetState()) {
			m_triggers[i]->SetState(State::ACCEPT);
			return i;
		}
	}

	return -1;
}

void Server::Move(const std::shared_ptr<Client>& client, XMFLOAT3 position)
{
	// 게임 룸에 진입한채 움직이면 시야처리 X (타워 씬)
	int room_num = client->GetRoomNum();
	if (m_game_room_manager->IsValidRoomNum(room_num)) {
		MoveObject(client, position);
		SetPositionOnStairs(client);
		RotateBoundingBox(client);

		GameRoomPlayerCollisionCheck(client, room_num);

		SendMoveInGameRoom(client->GetId(), room_num);
	}
	// 게임 룸이 아닌채 움직이면 시야처리 (마을 씬)
	else {

	}
}

int Server::GetNearTarget(int client_id, float max_range)
{
	std::span<int> ids{};

	auto game_room = m_game_room_manager->GetGameRoom(m_clients[client_id]->GetRoomNum());
	if (!game_room)
		return -1;

	if (IsPlayer(client_id)) {
		ids = game_room->GetMonsterIds();
	}
	else {
		ids = game_room->GetPlayerIds();
	}

	XMFLOAT3 sub{};
	float length{}, min_length{ std::numeric_limits<float>::max() };
	XMFLOAT3 pos{ m_clients[client_id]->GetPosition() };
	int target{ -1 };
	for (int id : ids) {
		if (-1 == id) continue;
		if (State::INGAME != m_clients[id]->GetState()) continue;

		sub = Vector3::Sub(pos, m_clients[id]->GetPosition());
		length = Vector3::Length(sub);
		if (length < min_length && length < max_range) {
			min_length = length;
			target = id;
		}
	}

	return target;
}

void Server::MoveObject(const std::shared_ptr<GameObject>& object, XMFLOAT3 position)
{
	object->SetPosition(position);

	// 바운드 박스 갱신
	object->SetBoundingBoxCenter(position);
}

void Server::RotateBoundingBox(const std::shared_ptr<GameObject>& object)
{
	// 플레이어 회전 (degree 값)
	float radian = XMConvertToRadians(object->GetYaw());

	XMFLOAT4 q{};
	XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(0.f, radian, 0.f));

	// 바운드 박스 회전
	object->SetBoundingBoxOrientation(q);
}

void Server::SetPositionOnStairs(const std::shared_ptr<GameObject>& object)
{
	XMFLOAT3 pos = object->GetPosition();
	float ratio{};
	if (pos.z >= RoomSetting::DOWNSIDE_STAIRS_BACK &&
		pos.z <= RoomSetting::DOWNSIDE_STAIRS_FRONT) 
	{
		ratio = RoomSetting::DOWNSIDE_STAIRS_FRONT - pos.z;
		pos.y = RoomSetting::DEFAULT_HEIGHT - 
			RoomSetting::DOWNSIDE_STAIRS_HEIGHT * ratio / 10.f;
	}
	else if (pos.z >= RoomSetting::TOPSIDE_STAIRS_BACK &&
			pos.z <= RoomSetting::TOPSIDE_STAIRS_FRONT) 
	{
		ratio = pos.z - RoomSetting::TOPSIDE_STAIRS_BACK;
		pos.y = RoomSetting::DEFAULT_HEIGHT +
			RoomSetting::TOPSIDE_STAIRS_HEIGHT * ratio / 10.f;
	}
	MoveObject(object, pos);
}

void Server::Timer()
{
	using namespace std::chrono;
	TIMER_EVENT ev{};

	// 지역 타이머 큐
	std::priority_queue<TIMER_EVENT> timer_queue;

	while (true) {
		auto current_time = system_clock::now();

		// 지역 타이머 큐의 처리
		if (!timer_queue.empty()) {
			if (timer_queue.top().event_time <= current_time) {
				ev = timer_queue.top();
				timer_queue.pop();
				ProcessEvent(ev);
			}
		}

		// 전역 concurrent 타이머 큐의 처리
		// 이벤트 시간이 안됐으면 지역큐에 넣고 기다린 후 while 문으로 돌아감
		if (m_timer_queue.try_pop(ev)) {
			if (ev.event_time > current_time) {
				timer_queue.push(ev);
				std::this_thread::sleep_for(5ms);
				continue;
			}
			else {
				ProcessEvent(ev);
				continue;
			}
		}

		// 지역 큐가 비었고, 전역 타이머큐 try_pop 실패 시 대기
		std::this_thread::sleep_for(5ms);
	}
}

void Server::ProcessEvent(const TIMER_EVENT& ev)
{
	using namespace std::chrono;

	switch (ev.event_type) {
	case EventType::COOLDOWN_RESET: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_COOLDOWN_RESET;
		memcpy(&over->_send_buf, &ev.action_type, sizeof(ActionType));

		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::BEHAVIOR_CHANGE: {
		auto monster = dynamic_pointer_cast<Monster>(m_clients[ev.obj_id]);
		if (MonsterBehavior::DEATH == ev.next_behavior_type) {
			ExpOver* over = new ExpOver;
			over->_comp_type = OP_MONSTER_REMOVE;

			PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
			return;
		}

		// 가장 최근에 발생한 행동전환 이벤트가 아니면 return
		if (ev.latest_id != monster->GetLastBehaviorId())
			return;

		// 죽었다면 기존에 타이머큐에 있던 동작이 넘어오면 넘겨버림
		if (MonsterBehavior::DEATH == monster->GetBehavior())
			return;

		if (MonsterBehavior::ATTACK == ev.next_behavior_type) {
			TIMER_EVENT attack_ev{ .event_time = system_clock::now() + MonsterSetting::ATK_COLLISION_TIME[static_cast<int>(monster->GetMonsterType())],
				.obj_id = ev.obj_id, .targat_id = monster->GetRoomNum(),
				.position = Vector3::Add(monster->GetPosition(), monster->GetFront()),
				.event_type = EventType::MONSTER_ATTACK_COLLISION, .action_type = ActionType::NORMAL_ATTACK,
			};

			m_timer_queue.push(attack_ev);
		}

		ExpOver* over = new ExpOver;
		over->_comp_type = OP_BEHAVIOR_CHANGE;
		memcpy(&over->_send_buf, &ev.next_behavior_type, sizeof(MonsterBehavior));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::AGRO_LEVEL_DECREASE: {
		auto monster = dynamic_pointer_cast<Monster>(m_clients[ev.obj_id]);
		if (ev.aggro_level < monster->GetAggroLevel())
			return;

		ExpOver* over = new ExpOver;
		over->_comp_type = OP_AGRO_REDUCE;
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::ATTACK_COLLISION: {
		auto client = dynamic_pointer_cast<Client>(m_clients[ev.obj_id]);

		switch (ev.action_type) {
		case ActionType::NORMAL_ATTACK:
			if (ObjectAnimation::ATTACK != client->GetCurrentAnimation())
				return;
			break;
		case ActionType::SKILL:
			if (PlayerAnimation::SKILL != client->GetCurrentAnimation())
				return;
			break;
		case ActionType::ULTIMATE:
			if (PlayerAnimation::ULTIMATE != client->GetCurrentAnimation())
				return;
			break;
		}

		ExpOver* over = new ExpOver;
		over->_comp_type = OP_ATTACK_COLLISION;
		memcpy(&over->_send_buf[0], &ev.action_type, sizeof(ActionType));
		memcpy(&over->_send_buf[1], &ev.position, sizeof(XMFLOAT3));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::MONSTER_ATTACK_COLLISION: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_MONSTER_ATTACK_COLLISION;
		memcpy(&over->_send_buf[0], &ev.action_type, sizeof(ActionType));
		memcpy(&over->_send_buf[1], &ev.position, sizeof(XMFLOAT3));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::STAMINA_CHANGE: {
		auto client = dynamic_pointer_cast<Client>(m_clients[ev.obj_id]);
		if (ev.latest_id != client->GetLatestId())
			return;

		ExpOver* over = new ExpOver;
		over->_comp_type = OP_STAMINA_CHANGE;
		memcpy(&over->_send_buf[0], &ev.is_stamina_increase, sizeof(bool));
		memcpy(&over->_send_buf[1], &ev.latest_id, sizeof(BYTE));
		memcpy(&over->_send_buf[2], &ev.targat_id, sizeof(int));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::HIT_SCAN: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_HIT_SCAN;
		memcpy(over->_send_buf, &ev.targat_id, sizeof(int));
		memcpy(over->_send_buf + sizeof(int), &ev.action_type, sizeof(char));
		memcpy((over->_send_buf + sizeof(int) + sizeof(char)), &ev.position, sizeof(XMFLOAT3));
		memcpy((over->_send_buf + sizeof(int) + sizeof(char) + sizeof(XMFLOAT3)),
			&ev.direction, sizeof(XMFLOAT3));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::ARROW_SHOOT: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_ARROW_SHOOT;
		memcpy(&over->_send_buf, &ev.action_type, sizeof(ActionType));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::ARROW_REMOVE: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_ARROW_REMOVE;
		memcpy(over->_send_buf, &ev.targat_id, sizeof(int));
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::GAME_ROOM_RESET: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_GAME_ROOM_RESET;

		// room_num 을 obj_id 로 넘김
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::TRIGGER_COOLDOWN: {
		// 특정 id의 트리거(obj_id)에 대한 발동시킨 오브젝트(target_id)의 플래그 초기화
		UCHAR trigger = m_triggers[ev.obj_id]->GetType();
		m_clients[ev.targat_id]->SetTriggerFlag(trigger, false);
		break;
	}
	case EventType::TRIGGER_REMOVE: {
		// 특정 id의 트리거(obj_id)를 해당 게임룸(target_id)에서 빼고
		// 해당 트리거를 FREE로 상태 변경
		m_game_room_manager->RemoveTrigger(ev.obj_id, ev.targat_id);
		{
			std::lock_guard<std::mutex> l{ m_triggers[ev.obj_id]->GetStateMutex() };
			m_triggers[ev.obj_id]->SetState(State::FREE);
		}
		break;
	}

	
	}
}

void Server::SetTimerEvent(const TIMER_EVENT& ev)
{
	m_timer_queue.push(ev);
}

void Server::SetAttackTimerEvent(int id, ActionType attack_type,
	std::chrono::system_clock::time_point attack_time)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);
	int player_type = static_cast<int>(client->GetPlayerType());

	TIMER_EVENT ev{ .obj_id = id, .action_type = attack_type };

	switch (client->GetPlayerType()) {
	case PlayerType::WARRIOR:
		ev.event_type = EventType::ATTACK_COLLISION;

		switch (attack_type) {
		case ActionType::NORMAL_ATTACK:
			ev.event_time = attack_time + PlayerSetting::ATTACK_COLLISION_TIME[player_type];
			ev.position = Vector3::Add(client->GetPosition(), Vector3::Mul(client->GetFront(), 0.8f));
			break;
		case ActionType::SKILL:
			ev.event_time = attack_time + PlayerSetting::SKILL_COLLISION_TIME[player_type];
			ev.position = Vector3::Add(client->GetPosition(), Vector3::Mul(client->GetFront(), 0.8f));
			break;
		case ActionType::ULTIMATE:
			ev.event_time = attack_time + PlayerSetting::ULTIMATE_COLLISION_TIME[player_type];
			ev.position = Vector3::Add(client->GetPosition(), Vector3::Mul(client->GetFront(), 0.8f));
			break;
		}
		break;
	}
	m_timer_queue.push(ev);
}

void Server::SetCooldownTimerEvent(int id, ActionType action_type)
{
	int player_type = static_cast<int>(m_clients[id]->GetPlayerType());

	TIMER_EVENT ev{.obj_id = id, .event_type = EventType::COOLDOWN_RESET,
		.action_type = action_type };

	ev.event_time = std::chrono::system_clock::now();

	// 쿨타임 중인지 검사 필요
	// 공격 외 쿨타임 처리
	switch (action_type) {
	case ActionType::NORMAL_ATTACK:
		ev.event_time += PlayerSetting::ATTACK_COOLDOWN[player_type];
		break;
	case ActionType::SKILL:
		ev.event_time += PlayerSetting::SKILL_COOLDOWN[player_type];
		break;
	case ActionType::ULTIMATE:
		ev.event_time += PlayerSetting::ULTIMATE_COOLDOWN[player_type];
		break;
	case ActionType::DASH:
		ev.event_time += PlayerSetting::DASH_COOLDOWN;
		break;
	case ActionType::ROLL:
		ev.event_time += PlayerSetting::ROLL_COOLDOWN;
		break;
	}
	m_timer_queue.push(ev);
}

void Server::SetStaminaTimerEvent(int client_id, bool is_increase)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[client_id]);

	TIMER_EVENT ev{ .obj_id = client_id, .event_type = EventType::STAMINA_CHANGE };

	if (!is_increase) {
		ev.event_time = std::chrono::system_clock::now();
		ev.is_stamina_increase = false;

		if (PlayerAnimation::ROLL == client->GetCurrentAnimation()) {
			ev.targat_id = -PlayerSetting::ROLL_STAMINA_CHANGE_AMOUNT;
		}
		else {
			ev.targat_id = -PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;
		}

		BYTE latest_id{ client->GetLatestId() };
		if (std::numeric_limits<BYTE>::max() == latest_id)
			latest_id = 0;
		ev.latest_id = ++latest_id;
		client->SetLatestId(latest_id);

		m_timer_queue.push(ev);
	}
	else {
		ev.event_time = std::chrono::system_clock::now() + std::chrono::seconds(2);
		ev.is_stamina_increase = true;
		ev.targat_id = PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;

		BYTE latest_id{ client->GetLatestId() };
		if (std::numeric_limits<BYTE>::max() == latest_id)
			latest_id = 0;
		ev.latest_id = ++latest_id;
		client->SetLatestId(latest_id);

		m_timer_queue.push(ev);
	}
}

void Server::SetHitScanTimerEvent(int id, int target_id, ActionType action_type,
	int arrow_id)
{
	if (-1 == target_id)
		return;

	auto now = std::chrono::system_clock::now();

	XMFLOAT3 dir = Vector3::Sub(m_clients[target_id]->GetPosition(),
		m_clients[id]->GetPosition());
	float length = Vector3::Length(dir);
	float speed{};

	if (IsPlayer(id)) {
		speed = PlayerSetting::ARROW_SPEED;
	}
	else {
		speed = MonsterSetting::ARROW_SPEED;
	}
	float time = length * 1000 / speed;		// ms
	std::chrono::milliseconds ms{static_cast<long long>(time)};

	TIMER_EVENT ev{ .event_time = now + ms, .obj_id = id, .targat_id = arrow_id,
		.position = m_clients[id]->GetPosition(), .direction = Vector3::Normalize(dir), 
		.event_type = EventType::HIT_SCAN, .action_type = action_type };
	
	m_timer_queue.push(ev);
}

void Server::SetArrowShootTimerEvent(int id, ActionType attack_type, 
	std::chrono::system_clock::time_point attack_time)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);

	TIMER_EVENT ev{ .obj_id = id, .event_type = EventType::ARROW_SHOOT,
		.action_type = attack_type };

	int type = static_cast<int>(PlayerType::ARCHER);
	switch (attack_type) {
	case ActionType::NORMAL_ATTACK:
		ev.event_time = attack_time + PlayerSetting::ATTACK_COLLISION_TIME[type];
		break;
	case ActionType::SKILL:
		ev.event_time = attack_time + PlayerSetting::SKILL_COLLISION_TIME[type];
		break;
	case ActionType::ULTIMATE:
		ev.event_time = attack_time + PlayerSetting::ULTIMATE_COLLISION_TIME[type];
		break;
	}
	m_timer_queue.push(ev);
}

void Server::SetRemoveArrowTimerEvent(int client_id, int arrow_id)
{
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + RoomSetting::ARROW_REMOVE_TIME,
		.obj_id = client_id, .targat_id = arrow_id,
		.event_type = EventType::ARROW_REMOVE };

	m_timer_queue.push(ev);
}

void Server::CollideObject(const std::shared_ptr<GameObject>& object, const std::span<INT>& ids,
	std::function<void(const std::shared_ptr<GameObject>&, const std::shared_ptr<GameObject>&)> func)
{
	for (INT id : ids) {
		if (-1 == id) continue;
		if (id == object->GetId()) continue;
		if (State::INGAME != m_clients[id]->GetState()) continue;

		if (object->GetBoundingBox().Intersects(m_clients[id]->GetBoundingBox())) {
			func(object, m_clients[id]);
		}
	}
}

void Server::CollideByStatic(const std::shared_ptr<GameObject>& object,
	const std::shared_ptr<GameObject>& static_object)
{
	BoundingOrientedBox& static_obb = static_object->GetBoundingBox();
	BoundingOrientedBox& obb = object->GetBoundingBox();

	FLOAT obb_left = obb.Center.x - obb.Extents.x;
	FLOAT obb_right = obb.Center.x + obb.Extents.x;
	FLOAT obb_front = obb.Center.z + obb.Extents.z;
	FLOAT obb_back = obb.Center.z - obb.Extents.z;

	FLOAT static_obb_left = static_obb.Center.x - static_obb.Extents.x;
	FLOAT static_obb_right = static_obb.Center.x + static_obb.Extents.x;
	FLOAT static_obb_front = static_obb.Center.z + static_obb.Extents.z;
	FLOAT static_obb_back = static_obb.Center.z - static_obb.Extents.z;

	FLOAT x_bias{}, z_bias{};
	bool push_out_x_plus{ false }, push_out_z_plus{ false };

	// 충돌한 물체의 중심이 x가 더 크면
	if (obb.Center.x - static_obb.Center.x <= std::numeric_limits<FLOAT>::epsilon()) {
		x_bias = obb_right - static_obb_left;
	}
	else {
		x_bias = static_obb_right - obb_left;
		push_out_x_plus = true;
	}

	// 충돌한 물체의 중심이 z가 더 크면
	if (obb.Center.z - static_obb.Center.z <= std::numeric_limits<FLOAT>::epsilon()) {
		z_bias = obb_front - static_obb_back;
	}
	else {
		z_bias = static_obb_front - obb_back;
		push_out_z_plus = true;
	}

	XMFLOAT3 pos = object->GetPosition();

	// z 방향으로 밀어내기
	if (x_bias - z_bias >= std::numeric_limits<FLOAT>::epsilon()) {
		// object가 +z 방향으로
		if (push_out_z_plus) {
			pos.z += z_bias;
		}
		// object가 -z 방향으로
		else {
			pos.z -= z_bias;
		}
	}

	// x 방향으로 밀어내기
	else {
		// object가 +x 방향으로
		if (push_out_x_plus) {
			pos.x += x_bias;
		}
		// object가 -x 방향으로
		else {
			pos.x -= x_bias;
		}
	}

	MoveObject(object, pos);
}

void Server::CollideByMoveMent(const std::shared_ptr<GameObject>& object,
	const std::shared_ptr<GameObject>& movement_object)
{
	// 충돌한 플레이어의 속도
	auto obj = dynamic_pointer_cast<MovementObject>(object);
	XMFLOAT3 velocity = obj->GetVelocity();

	// 충돌된 오브젝트의 위치
	XMFLOAT3 pos = movement_object->GetPosition();

	// 충돌된 오브젝트를 충돌한 플레이어의 속도만큼 밀어냄
	// 밀어내는 방향 고려할 필요 있음 ( 아직 X )
	pos.x += velocity.x;
	pos.y += velocity.y;
	pos.z += velocity.z;
	MoveObject(movement_object, pos);
}

void Server::CollideByStaticOBB(const std::shared_ptr<GameObject>& object,
	const std::shared_ptr<GameObject>& static_object)
{
	// 1. 오브젝트에서 충돌면을 구한다
	// 2. 해당 충돌면의 법선 벡터를 구한다
	// 3. 플레이어의 속도 벡터에 2에서 구한 법선 벡터를 투영한다
	// 4. 투영 벡터만큼 플레이어를 이동시킨다

	XMFLOAT3 corners[8]{};

	BoundingOrientedBox static_obb = static_object->GetBoundingBox();
	static_obb.Center.y = 0.f;
	static_obb.GetCorners(corners);
	
	// 꼭짓점 시계방향 0,1,5,4
	XMFLOAT3 o_square[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z} ,
		{corners[5].x, 0.f, corners[5].z} ,
		{corners[4].x, 0.f, corners[4].z} };

	BoundingOrientedBox object_obb = object->GetBoundingBox();
	object_obb.Center.y = 0.f;
	object_obb.GetCorners(corners);

	XMFLOAT3 p_square[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z} ,
		{corners[5].x, 0.f, corners[5].z} ,
		{corners[4].x, 0.f, corners[4].z} };


	
	for (const XMFLOAT3& point : p_square) {
		if (!static_obb.Contains(XMLoadFloat3(&point))) continue;

		std::array<float, 4> dist{};
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
		MoveObject(object, Vector3::Add(object->GetPosition(), v));
		break;
	}

}
 