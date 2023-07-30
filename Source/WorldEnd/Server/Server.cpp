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

	for (size_t i = ARROW_RAIN_START; i < ARROW_RAIN_END; ++i) {
		m_triggers[i] = std::make_shared<ArrowRain>();
		m_triggers[i]->SetId(i);
	}

	for (size_t i = UNDEAD_GRASP_START; i < UNDEAD_GRASP_END; ++i) {
		m_triggers[i] = std::make_shared<UndeadGrasp>();
		m_triggers[i]->SetId(i);
	}

	for (size_t i = BOSS_MONSTER_START; i < BOSS_MONSTER_END; ++i) {
		m_clients[i] = std::make_shared<BossMonster>();
	}


	m_game_room_manager = std::make_unique<GameRoomManager>();
	m_party_manager = std::make_unique <PartyManager>();

	m_database = std::make_unique<DataBase>();
	m_database->InitDataBase();

	printf("Complete Initialize!\n");
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

	int worker_thread_num = std::thread::hardware_concurrency();
	for (int i = 0; i < worker_thread_num; ++i)
		m_worker_threads.emplace_back(&Server::WorkerThread, this);

	for (auto& th : m_worker_threads)
		th.detach();

	std::thread thread1{ &Server::TimerThread, this };
	thread1.detach();
	std::thread thread2{ &Server::TimerThread, this };
	thread2.detach();

	std::thread db_thread { &Server::DBThread, this };
	db_thread.detach();

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
			m_game_room_manager->SendPlayerData();
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
	while (true) {
		DWORD num_bytes{};
		ULONG_PTR key{};
		WSAOVERLAPPED* over{};

		BOOL ret = GetQueuedCompletionStatus(m_handle_iocp, &num_bytes, &key, &over, INFINITE);
		ExpOver* exp_over = reinterpret_cast<ExpOver*>(over);

		if (FALSE == ret) {
			printf("GQCS Error on client[%d]\n", static_cast<int>(key));
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
				printf("Maximum Number of Users Exceeded.\n");
			}
			else {
				auto client = dynamic_pointer_cast<Client>(m_clients[new_id]);
				client->SetId(new_id);

				client->SetRemainSize(0);
				ExpOver& ex_over = client->GetExpOver();
				ex_over._comp_type = OP_RECV;
				ex_over._wsa_buf.buf = ex_over._send_buf;
				ex_over._wsa_buf.len = BUF_SIZE;
				ZeroMemory(&ex_over._wsa_over, sizeof(ex_over._wsa_over));
				client->SetSocket(c_socket);


				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), m_handle_iocp, new_id, 0);
				client->DoRecv();

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

				auto client = std::dynamic_pointer_cast<Client>(m_clients[id]);
				client->IncreaseGold(reward);
				client->DoSend(&packet);
			}

			{
				std::lock_guard<std::mutex> l{ game_room->GetStateMutex() };
				game_room->SetState(GameRoomState::CLEAR);
				game_room->GetWarpPortal()->SetValid(true);
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

					switch (client->GetCurrentAnimation()) {
					case PlayerAnimation::ULTIMATE: {
						obb.Center = *pos;
						obb.Extents = PlayerSetting::
							ULTIMATE_EXTENT[(int)(client->GetPlayerType())][(int)client->GetUltimateSkillType()];
						break;
					}

					case PlayerAnimation::ULTIMATE2: {
						obb = client->GetWeaponBoundingBox();
						obb.Center = *pos;
						break; 
					}
					}
					break;
				}
			}

			AttackCollisionWithMonster(static_cast<int>(key), obb, attack_type);

			delete exp_over;
			break;
		}
		case OP_MONSTER_ATTACK_COLLISION: {
			int monster_id = static_cast<int>(key);
			auto monster = dynamic_pointer_cast<Monster>(m_clients[monster_id]);
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
			float damage{ monster->GetDamage() };	// 스킬이 있다면 계수 더하는 용도
			
			// 공격 충돌 검사시 행동이 공격 중이 아니면
			/*if (monster->GetBehavior() != MonsterBehavior::ATTACK) {
				delete exp_over;
				break;
			}*/

			if (MonsterType::WARRIOR == monster->GetMonsterType()) {
				obb.Center = *pos;
				obb.Extents = XMFLOAT3{ 0.2f, 0.2f, 0.2f };
			}
			else if (MonsterType::WIZARD == monster->GetMonsterType()) {
				XMFLOAT3 temp = Vector3::Normalize(Vector3::Sub(*pos, monster->GetPosition()));
				obb.Center = Vector3::Add(*pos, Vector3::Mul(temp, 0.5f));
				obb.Extents = XMFLOAT3{ 0.2f, 0.2f, 0.2f };
			}
			else if (MonsterType::BOSS == monster->GetMonsterType()) {
				if (monster->GetBehavior() == MonsterBehavior::ATTACK) {
					XMFLOAT3 temp = Vector3::Normalize(Vector3::Sub(*pos, monster->GetPosition()));
					obb.Center = Vector3::Add(*pos, Vector3::Mul(temp, 1.5f));
					obb.Extents = XMFLOAT3{ 1.1f, 1.1f, 1.1f };
				}
				else if (monster->GetBehavior() == MonsterBehavior::WIDE_SKILL) {
					XMFLOAT3 temp = Vector3::Normalize(Vector3::Sub(*pos, monster->GetPosition()));
					obb.Center = Vector3::Add(*pos, Vector3::Mul(temp, 2.0f));
					obb.Extents = XMFLOAT3{ 1.6f, 1.6f, 1.6f };
				}
				else if (monster->GetBehavior() == MonsterBehavior::NORMAL_ATTACK) {
					XMFLOAT3 temp = Vector3::Normalize(Vector3::Sub(*pos, monster->GetPosition()));
					obb.Center = Vector3::Add(*pos, Vector3::Mul(temp, 1.5f));
					obb.Extents = XMFLOAT3{ 1.2f, 1.2f, 1.2f };
				}
				else if (monster->GetBehavior() == MonsterBehavior::ENHANCE_WIDE_SKILL) {
					XMFLOAT3 temp = Vector3::Normalize(Vector3::Sub(*pos, monster->GetPosition()));
					obb.Center = Vector3::Add(*pos, Vector3::Mul(temp, 2.0f));
					obb.Extents = XMFLOAT3{ 1.7f, 1.7f, 1.7f };
				}
				else if (monster->GetBehavior() == MonsterBehavior::ULTIMATE_SKILL) {
					XMFLOAT3 temp = Vector3::Normalize(Vector3::Sub(*pos, monster->GetPosition()));
					obb.Center = Vector3::Add(*pos, Vector3::Mul(temp, 1.5f));
					obb.Extents = XMFLOAT3{ 1.1f, 1.1f, 1.1f };
				}
			}

			for (int id : player_ids) {
				if (-1 == id) continue;
				if (State::INGAME != m_clients[id]->GetState()) continue;

				if (obb.Intersects(m_clients[id]->GetBoundingBox())) {
					if (DecreaseState::DECREASE == m_clients[id]->DecreaseHp(m_clients[monster_id]->GetDamage(), -1)) {		// 감소한 경우에만 체력 변경
						SendChangeHp(id);
					}
				}
			}

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
				ev.target_id = PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;
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
				ev.target_id = -PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;
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
			float length{};
			if (IsPlayer(id)) {
				// 플레이어면 충돌대상은 몬스터이므로 ids는 몬스터
				ids = game_room->GetMonsterIds();
				length = PlayerSetting::ARROW_RANGE;
			}
			else {
				// 플레이어가 아니면 충돌대상은 플레이어이므로 ids는 플레이어
				ids = game_room->GetPlayerIds();
				length = MonsterSetting::ARROW_RANGE;
			}

			float near_dist{ std::numeric_limits<float>::max() };
			int near_id{ -1 };
			XMVECTOR pos{ XMLoadFloat3(position) };
			XMVECTOR dir{ XMLoadFloat3(direction) };

			// 광선 충돌 검사 후 가장 가까운 id 구함
			for (int client_id : ids) {
				if (-1 == client_id) continue;
				if (State::DEATH == m_clients[client_id]->GetState()) continue;

				auto& obb = m_clients[client_id]->GetBoundingBox();
				if (obb.Intersects(pos, dir, length)) {
					float dist{ Vector3::Length(Vector3::Sub(m_clients[client_id]->GetPosition(),
						m_clients[id]->GetPosition())) };

					if (dist < near_dist) {
						near_dist = dist;
						near_id = client_id;
					}
				}
			}

			// 빗나감
			if (-1 == near_id) {
				if (ActionType::ULTIMATE == *action_type) {		// 궁극기의 경우 빗나가도 주변 폭발 충돌되도록
					BoundingOrientedBox obb{};

					obb.Center = Vector3::Add(*position, Vector3::Mul(*direction, PlayerSetting::ARROW_RANGE));
					// position + dir * 거리 에서 폭발 충돌 처리
					obb.Extents = PlayerSetting::ULTIMATE_EXTENT
						[(int)PlayerType::ARCHER][std::dynamic_pointer_cast<Client>(m_clients[id])->GetUltimateSkillType()];
					AttackCollisionWithMonster(id, obb, *action_type);
					AttackCollisionWithMonster(id, obb, *action_type);
				}

				delete exp_over;
				break;
			}

			SendRemoveArrow(id, *arrow_id);

			float damage = m_clients[id]->GetDamage();
			if (IsPlayer(id)) {
				if (ActionType::SKILL == *action_type || 
					ActionType::NORMAL_ATTACK == *action_type) {
					damage *= m_clients[id]->GetSkillRatio(*action_type);
					if (DecreaseState::DECREASE == m_clients[near_id]->DecreaseHp(damage, id)) {
						SendChangeHp(near_id);
					}
				}
				else if (ActionType::ULTIMATE == *action_type) {
					// 주변 폭발 충돌
					BoundingOrientedBox obb{};

					obb.Center = m_clients[near_id]->GetPosition();
					// position + dir * 거리 에서 폭발 충돌 처리
					obb.Extents = PlayerSetting::ULTIMATE_EXTENT
						[(int)PlayerType::ARCHER][std::dynamic_pointer_cast<Client>(m_clients[id])->GetUltimateSkillType()];
					AttackCollisionWithMonster(id, obb, *action_type);
				}
			}
			else {
				if (DecreaseState::DECREASE == m_clients[near_id]->DecreaseHp(damage, id)) {
					SendChangeHp(near_id);
				}
			}

			delete exp_over;
			break;
		}
		case OP_ARROW_SHOOT: {
			int id = static_cast<int>(key);
			ActionType* type = reinterpret_cast<ActionType*>(exp_over->_send_buf);
			int* target_id = reinterpret_cast<int*>((exp_over->_send_buf + sizeof(ActionType)));

			switch (*type) {
			case ActionType::NORMAL_ATTACK:
				if (IsPlayer(id)) {
					if (m_clients[id]->GetCurrentAnimation() != ObjectAnimation::ATTACK)
						break;
				}
				ProcessArrow(id, *target_id, *type);
				break;
			case ActionType::SKILL:
				if (IsPlayer(id)) {
					if (m_clients[id]->GetCurrentAnimation() != PlayerAnimation::SKILL &&
						m_clients[id]->GetCurrentAnimation() != PlayerAnimation::SKILL2)
						break;
				}
				ProcessArrow(id, *target_id, *type);
			case ActionType::ULTIMATE: {
				XMFLOAT3* pos = reinterpret_cast<XMFLOAT3*>((exp_over->_send_buf +
					sizeof(ActionType) + sizeof(int)));
				
				switch(m_clients[id]->GetCurrentAnimation()) {
				case PlayerAnimation::ULTIMATE:
					SetTrigger(id, TriggerType::ARROW_RAIN, *pos);
					break;

				case PlayerAnimation::ULTIMATE2: {
					ProcessArrow(id, *target_id, *type);
					break; 
				}
				}
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
			int room_num = static_cast<int>(key);
			printf("%d 번 방 초기화!\n", room_num);
			auto game_room = m_game_room_manager->GetGameRoom(room_num);
			if (!game_room) {
				delete exp_over;
				return;
			}

			game_room->Reset();

			delete exp_over;
			break;
		}
		case OP_BATTLE_START: {
			int id = static_cast<int>(key);
			m_game_room_manager->StartBattle(m_clients[id]->GetRoomNum());

			delete exp_over;
			break;
		}
		case OP_PORTAL_WARP: {
			int id = static_cast<int>(key);
			auto game_room = m_game_room_manager->GetGameRoom(m_clients[id]->GetRoomNum());
			if (!game_room) {
				delete exp_over;
				break;
			}

			auto& ids = game_room->GetPlayerIds();

			for (int id : ids) {
				if (-1 == id) continue;

				Move(id, RoomSetting::TOWER_START_POSITION);
			}

			delete exp_over;
			break;
		}
		case OP_TRIGGER_COOLDOWN: {
			int trigger_id = static_cast<int>(key);
			int* obj_id = reinterpret_cast<int*>(exp_over->_send_buf);

			// 특정 id의 트리거(obj_id)에 대한 발동시킨 오브젝트(target_id)의 플래그 초기화
			UCHAR trigger = static_cast<UCHAR>(m_triggers[trigger_id]->GetType());
			m_clients[*obj_id]->SetTriggerFlag(trigger, false);

			auto game_room = m_game_room_manager->GetGameRoom(m_clients[*obj_id]->GetRoomNum());
			if (!game_room)
				break;

			game_room->CheckTriggerCollision(*obj_id);

			delete exp_over;
			break;
		}
		case OP_MULTIPLE_TRIGGER_SET: {
			int id = static_cast<int>(key);
			int* target_id = reinterpret_cast<int*>(&exp_over->_send_buf);
			TriggerType* type = reinterpret_cast<TriggerType*>((exp_over->_send_buf + sizeof(int)));
			BYTE* current_count = reinterpret_cast<BYTE*>((exp_over->_send_buf + sizeof(int) + sizeof(TriggerType)));
			BYTE* max_count = reinterpret_cast<BYTE*>((exp_over->_send_buf +
				sizeof(int) + sizeof(TriggerType) + sizeof(BYTE)));

			SendMagicCircle(m_clients[id]->GetRoomNum(), m_clients[*target_id]->GetPosition(),
				TriggerSetting::EXTENT[static_cast<int>(*type)]);

			TIMER_EVENT trigger_ev{ .event_time = std::chrono::system_clock::now()
			+ TriggerSetting::GENTIME[static_cast<int>(*type)],
			.obj_id = id, .target_id = *target_id, .position = m_clients[*target_id]->GetPosition(),
			.event_type = EventType::MULTIPLE_TRIGGER_SET, .trigger_type = *type,
			.latest_id = static_cast<BYTE>((*current_count + 1)), .aggro_level = *max_count,
			.is_valid = true };

			m_timer_queue.push(trigger_ev);

			delete exp_over;
			break;
		}
		case OP_LOGIN_OK:
			SendLoginOk(static_cast<int>(key));

			delete exp_over;
			break;
		case OP_LOGIN_FAIL:
			SendLoginFail(static_cast<int>(key));

			delete exp_over;
			break;
		case OP_SIGNIN_OK:
			SendSigninOK(static_cast<int>(key));

			delete exp_over;
			break;
		case OP_SIGNIN_FAIL:
			SendSigninFail(static_cast<int>(key));

			delete exp_over;
			break;

		}
	}
}

void Server::ProcessPacket(int id, char* p) 
{
	unsigned char type = p[1];
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);

	switch (type){
	case CS_PACKET_SIGNIN: {
		CS_SIGNIN_PACKET* packet = reinterpret_cast<CS_SIGNIN_PACKET*>(p);

		std::wstring user_id{ packet->id, packet->id + strlen(packet->id) };
		std::wstring password { packet->password, packet->password + strlen(packet->password) };

		DB_EVENT ev{};
		ev.client_id = id;
		ev.data.user_id = user_id;
		ev.data.password = password;
		ev.event_type = DBEventType::CREATE_ACCOUNT;
		ev.event_time = std::chrono::system_clock::now();
		SetDatabaseEvent(ev);

		break;
	}
	case CS_PACKET_LOGIN: {
		CS_LOGIN_PACKET* packet = reinterpret_cast<CS_LOGIN_PACKET*>(p);
		
		std::wstring user_id{ packet->id, packet->id + strlen(packet->id) };
		std::wstring password { packet->password, packet->password + strlen(packet->password) };

		DB_EVENT ev{};
		ev.client_id = id;
		ev.data.user_id = user_id;
		ev.data.password = password;
		ev.event_type = DBEventType::TRY_LOGIN;
		ev.event_time = std::chrono::system_clock::now();
		SetDatabaseEvent(ev);
		break;
	}
	case CS_PACKET_PLAYER_MOVE: {
		CS_PLAYER_MOVE_PACKET* packet = reinterpret_cast<CS_PLAYER_MOVE_PACKET*>(p);

		client->SetYaw(packet->yaw);
		RotateBoundingBox(client);
		Move(id, packet->pos);
		
#ifdef USER_NUM_TEST
		client->SetLastMoveTime(packet->move_time);
#endif	
		break;
	}
	case CS_PACKET_SET_COOLDOWN: {
		CS_COOLDOWN_PACKET* packet = reinterpret_cast<CS_COOLDOWN_PACKET*>(p);

		SetCooldownTimerEvent(id, packet->cooldown_type);
		break;
	}
	case CS_PACKET_ATTACK: {
		CS_ATTACK_PACKET* packet = reinterpret_cast<CS_ATTACK_PACKET*>(p);

		auto time = std::chrono::system_clock::now() - packet->attack_time;
		packet->attack_time += time;

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

		if (!client->GetInteractable()) {
			break;
		}

		// 해당 아이디에 타입에 맞는 상호작용 처리
		switch (packet->interaction_type) {
		case InteractionType::BATTLE_STARTER:
			client->SetInteractable(false);
			m_game_room_manager->InteractObject(client->GetRoomNum(), packet->interaction_type);
			SetBattleStartTimerEvent(id);
			break;
		case InteractionType::PORTAL:
			client->SetInteractable(false);
			m_game_room_manager->WarpNextFloor(client->GetRoomNum());
			SetWarpTimerEvent(id);
			break;
		case InteractionType::ENHANCMENT:

			break;
		case InteractionType::RECORD_BOARD:

			break;
		case InteractionType::DUNGEON_CLEAR:
			client->SetInteractable(false);
			m_game_room_manager->DungeonClear(client->GetRoomNum());
			break;
		}

		break;
	}
	case CS_PACKET_JOIN_PARTY: {
		CS_JOIN_PARTY_PACKET* packet = reinterpret_cast<CS_JOIN_PARTY_PACKET*>(p);

		m_party_manager->JoinParty(packet->party_num, id);
		break;
	}
	case CS_PACKET_CREATE_PARTY: {
		CS_CREATE_PARTY_PACKET* packet = reinterpret_cast<CS_CREATE_PARTY_PACKET*>(p);

		m_party_manager->CreateParty(packet->party_num, id);
		break;
	}
	case CS_PACKET_EXIT_PARTY: {
		CS_EXIT_PARTY_PACKET* packet = reinterpret_cast<CS_EXIT_PARTY_PACKET*>(p);

		m_party_manager->ExitParty(client->GetPartyNum(), id);
		break;
	}
	case CS_PACKET_CHANGE_CHARACTER: {
		CS_CHANGE_CHARACTER_PACKET* packet =
			reinterpret_cast<CS_CHANGE_CHARACTER_PACKET*>(p);

		client->SetPlayerType(packet->player_type);
		
		SendChangeCharacter(id, packet->player_type);
		break;
	}
	case CS_PACKET_EXIT_DUNGEON: {
		CS_EXIT_DUNGEON_PACKET* packet = reinterpret_cast<CS_EXIT_DUNGEON_PACKET*>(p);

		
		m_game_room_manager->RemovePlayer(id);
		SendExitDungeonOk(id);
		client->SetHp(m_clients[id]->GetMaxHp());
		client->SetPosition(client->GetTownPosition());

		break;
	}
	case CS_PACKET_ENTER_DUNGEON: {
		CS_ENTER_DUNGEON_PACKET* packet = reinterpret_cast<CS_ENTER_DUNGEON_PACKET*>(p);

		auto& party = m_party_manager->GetParty(client->GetPartyNum());

		if (party->GetHostId() != id)
			break;

		party->SendEnterDungeon();

		// 파티원 마을 명단에서 삭제
		std::unordered_set<int> removed_players{};
		{
			std::lock_guard<std::mutex> l{ m_village_lock };
			for (INT party_member : party->GetMembers()) {
				if (0 != m_village_ids.erase(party_member)) {	// 0 반환은 key 가 없을 경우 -> 존재하는 key 면 삭제하도록
					removed_players.insert(party_member);
				}
			}
		}

		for (INT id : removed_players) {
			SendRemoveInVillage(id);
		}

		m_game_room_manager->EnterGameRoom(party);


		break;
	}
	case CS_PACKET_CHANGE_PARTY_PAGE: {
		CS_CHANGE_PARTY_PAGE_PACKET* packet = reinterpret_cast<CS_CHANGE_PARTY_PAGE_PACKET*>(p);
		
		m_party_manager->ChangePage(id, packet->page);
		break;
	}
	case CS_PACKET_OPEN_PARTY_UI: {
		CS_OPEN_PARTY_UI_PACKET* packet = reinterpret_cast<CS_OPEN_PARTY_UI_PACKET*>(p);


		m_party_manager->OpenPartyUI(id);
		break;
	}
	case CS_PACKET_ENTER_VILLAGE: {
		CS_ENTER_VILLAGE_PACKET* packet = reinterpret_cast<CS_ENTER_VILLAGE_PACKET*>(p);
		
		// 마을 인원에 추가
		std::unordered_set<int> ids{};
		{
			std::lock_guard<std::mutex> l{ m_village_lock };
			m_village_ids.insert(id);
			ids = m_village_ids;
		}

		// 추가한 플레이어 add
		for (auto& pl_id : ids) {
			if (-1 == pl_id) continue;
			if (pl_id == id) continue;

			// 기존 플레이어 정보를 나에게 전송
			SendAddPlayer(pl_id, id);

			// 내 정보를 기존에 방에 있던 플레이어에게 전송
			SendAddPlayer(id, pl_id);
		}
		break;
	}
	case CS_PACKET_CLOSE_PARTY_UI: {
		CS_CLOSE_PARTY_UI_PACKET* packet = reinterpret_cast<CS_CLOSE_PARTY_UI_PACKET*>(p);

		m_party_manager->ClosePartyUI(id);
		break;
	}
	case CS_PACKET_ENHANCE: {
		CS_ENHANCE_PACKET* packet = reinterpret_cast<CS_ENHANCE_PACKET*>(p);

		int cost{ client->GetCost(packet->enhancement_type) };

		if (cost > client->GetGold()) {
			break;
		}

		if (client->GetLevel(packet->enhancement_type) >= PlayerSetting::MAX_ENHANCEMENT_LEVEL) {
			break;
		}

		client->LevelUpEnhancement(packet->enhancement_type);
		client->IncreaseGold(-cost);

		SendEnhanceOk(id, packet->enhancement_type);

		break;
	}
	case CS_PACKET_CHNAGE_SKILL: {
		CS_CHANGE_SKILL_PACKET* packet = reinterpret_cast<CS_CHANGE_SKILL_PACKET*>(p);

		int cost{};

		switch (packet->skill_type) {
		case static_cast<UCHAR>(SkillType::NORMAL):
			cost = PlayerSetting::DEFAULT_NORMAL_SKILL_COST;
			break;
		case static_cast<UCHAR>(SkillType::ULTIMATE):
			cost = PlayerSetting::DEFAULT_ULTIMATE_COST;
			break;
		}

		if (cost > client->GetGold()) {
			break;
		}

		client->ChangeSkill(packet->player_type, packet->skill_type, packet->changed_type);
		client->IncreaseGold(-cost);

		break;
	}
	case CS_PACKET_TOWER_SCENE: {		// 타워 씬으로 전환했음을 받고 플레이어, 몬스터를 ADD
		CS_TOWER_SCENE_PACKET* packet = reinterpret_cast<CS_TOWER_SCENE_PACKET*>(p);

		int room_num = client->GetRoomNum();
		if (-1 == room_num) {
			break;
		}

		auto& ids = m_game_room_manager->GetGameRoom(room_num)->GetPlayerIds();

		// Add Player
		for (INT player_id : ids) {
			if (-1 == player_id) continue;
			if (id == player_id) continue;

			// 다른 플레이어 정보를 씬 전환이 된 플레이어에게 전송
			SendAddPlayer(player_id, id);
		}

		// Add Monster
		m_game_room_manager->SendAddMonster(client->GetRoomNum(), id);


		break;
	}

	case CS_PACKET_TELEPORT_GATE: {
		Move(id, PlayerSetting::GATE_POSITION);
		break;
	}
	case CS_PACKET_TELEPORT_NPC: {
		Move(id, PlayerSetting::NPC_POSITION);
		break;
	}
	case CS_PACKET_INVINCIBLE: {
		client->ToggleInvinsible();	// 무적이 false면 true 로, true면 false 로 전환
		break;
	}


	}
}


void Server::Disconnect(int id)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);
	if (client->GetPartyNum() != -1) {
		m_party_manager->ExitParty(client->GetPartyNum(), id);
	}

	if (m_game_room_manager->IsValidRoomNum(client->GetRoomNum())) {
		m_game_room_manager->RemovePlayer(id);
	}
	else {
		SendRemoveInVillage(id);

		// 파티 UI 보고 있었다면 함수 내부에서 지움
		m_party_manager->ClosePartyUI(id);
	}

	DB_EVENT ev{};
	ev.client_id = id;
	ev.data.user_id = client->GetUserId();
	ev.data.player_type = (char)client->GetPlayerType();
	ev.data.gold = client->GetGold();

	XMFLOAT3 pos{ client->GetTownPosition() };
	ev.data.x = pos.x;
	ev.data.y = pos.y;
	ev.data.z = pos.z;

	ev.data.hp_level = client->GetHpLevel();
	ev.data.atk_level = client->GetAtkLevel();
	ev.data.def_level = client->GetDefLevel();
	ev.data.crit_rate_level = client->GetCritRateLevel();
	ev.data.crit_damage_level = client->GetCritDamageLevel();

	
	for (size_t i = 0; i < (INT)PlayerType::COUNT; ++i) {
		ev.data.normal_skill_type[i] = client->GetNormalSkillType((PlayerType)i);
		ev.data.ultimate_type[i] = client->GetUltimateSkillType((PlayerType)i);
	}

	ev.event_type = DBEventType::SAVE_USER_DATA;
	ev.event_time = std::chrono::system_clock::now();
	SetDatabaseEvent(ev);

	client->Init();

	client->SetRemainSize(0);
	ExpOver& ex_over = client->GetExpOver();
	ex_over._comp_type = OP_RECV;
	ex_over._wsa_buf.buf = ex_over._send_buf;
	ex_over._wsa_buf.len = BUF_SIZE;
	ZeroMemory(&ex_over._wsa_over, sizeof(ex_over._wsa_over));
	
	closesocket(client->GetSocket());

	std::lock_guard<std::mutex> lock{ client->GetStateMutex() };
	client->SetState(State::FREE);
}


void Server::SendSigninOK(int client_id)
{
	SC_SIGNIN_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_SIGNIN_OK;

	m_clients[client_id]->DoSend(&packet);
}

void Server::SendSigninFail(int client_id)
{
	SC_SIGNIN_FAIL_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_SIGNIN_FAIL;

	m_clients[client_id]->DoSend(&packet);
}

void Server::SendLoginOk(int client_id)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[client_id]);

	SC_LOGIN_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.id = client_id;
	packet.pos = client->GetPosition();
	packet.player_type = client->GetPlayerType();
	
	packet.gold = client->GetGold();
	packet.hp_level = client->GetHpLevel();
	packet.atk_level = client->GetAtkLevel();
	packet.def_level = client->GetDefLevel();
	packet.crit_rate_level = client->GetCritRateLevel();
	packet.crit_damage_level = client->GetCritDamageLevel();

	for (size_t i = 0; i < (INT)PlayerType::COUNT; ++i) {
		packet.skills[i].first = client->GetNormalSkillType((PlayerType)i);
		packet.skills[i].second = client->GetUltimateSkillType((PlayerType)i);
	}

	m_clients[client_id]->DoSend(&packet);
}

void Server::SendLoginFail(int client_id)
{
	SC_LOGIN_FAIL_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_FAIL;
	
	m_clients[client_id]->DoSend(&packet);
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

			m_clients[id]->DoSend(&packet);
		}
	}
	// 마을 처리
	else {
		for (auto id : m_village_ids) {
			if (-1 == id) continue;

			m_clients[id]->DoSend(&packet);
		}
	}
}

void Server::SendArrowShoot(int client_id, int arrow_id, ActionType type)
{
	SC_ARROW_SHOOT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ARROW_SHOOT;
	packet.id = client_id;
	packet.arrow_id = arrow_id;
	packet.action_type = type;

	int room_num = m_clients[client_id]->GetRoomNum();
	auto game_room = m_game_room_manager->GetGameRoom(room_num);

	if (game_room) {
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
	auto game_room = m_game_room_manager->GetGameRoom(room_num);

	if (game_room) {
		auto& ids = game_room->GetPlayerIds();

		for (int id : ids) {
			if (-1 == id) continue;

			m_clients[id]->DoSend(&packet);
		}
	}
	else {

	}
}

void Server::SendChangeHp(int client_id)
{
	int room_num = m_clients[client_id]->GetRoomNum();
	auto game_room = m_game_room_manager->GetGameRoom(room_num);
	if (!game_room)
		return;

	SC_CHANGE_HP_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_HP;
	packet.id = client_id;
	packet.hp = m_clients[client_id]->GetHp();

	// 플레이어가 피격 후 죽었다면
	if (IsPlayer(client_id)) {
		if (State::DEATH == m_clients[client_id]->GetState()) {
			SendPlayerDeath(client_id);
			return;
		}
	}

	auto& ids = game_room->GetPlayerIds();

	for (int id : ids) {
		if (-1 == id) continue;

		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendTrigger(int client_id, TriggerType type, const XMFLOAT3& pos)
{
	int room_num = m_clients[client_id]->GetRoomNum();
	auto game_room = m_game_room_manager->GetGameRoom(room_num);
	if (!game_room)
		return;

	SC_ADD_TRIGGER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ADD_TRIGGER;
	packet.trigger_type = type;
	packet.pos = pos;

	for (int id : game_room->GetPlayerIds()) {
		if (-1 == id) continue;
		
		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendMagicCircle(int room_num, const XMFLOAT3& pos, const XMFLOAT3& extent)
{
	auto game_room = m_game_room_manager->GetGameRoom(room_num);
	if (!game_room)
		return;

	SC_ADD_MAGIC_CIRCLE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ADD_MAGIC_CIRCLE;
	packet.pos = pos;
	packet.extent = extent;

	for (int id : game_room->GetPlayerIds()) {
		if (-1 == id) continue;

		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendAddPlayer(int sender, int receiver)
{
	SC_ADD_PLAYER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ADD_PLAYER;
	packet.id = sender;
	packet.pos = m_clients[sender]->GetPosition();
	packet.hp = m_clients[sender]->GetHp();
	packet.player_type = m_clients[sender]->GetPlayerType();

	std::wstring ws = std::dynamic_pointer_cast<Client>(m_clients[sender])->GetUserId();
	std::string s{};
	s.assign(ws.begin(), ws.end());
	strcpy_s(packet.name, s.c_str());

	m_clients[receiver]->DoSend(&packet);
}

void Server::SendMovePlayer(int client_id, int move_object)
{
	SC_UPDATE_CLIENT_PACKET packet{};

	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_CLIENT;
	packet.id = move_object;
	packet.pos = m_clients[move_object]->GetPosition();
	packet.yaw = m_clients[move_object]->GetYaw();

	m_clients[client_id]->DoSend(&packet);
}

void Server::SendEnhanceOk(int client_id, EnhancementType type)
{
	auto client = std::dynamic_pointer_cast<Client>(m_clients[client_id]);

	SC_ENHANCE_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ENHANCE_OK;
	packet.enhancement_type = type;

	int level{};
	switch (type) {
	case EnhancementType::HP:
		level = client->GetHpLevel();
		break;
	case EnhancementType::ATK:
		level = client->GetAtkLevel();
		break;
	case EnhancementType::DEF:
		level = client->GetDefLevel();
		break;
	case EnhancementType::CRIT_RATE:
		level = client->GetCritRateLevel();
		break;
	case EnhancementType::CRIT_DAMAGE:
		level = client->GetCritDamageLevel();
		break;
	}
	packet.level = level;

	m_clients[client_id]->DoSend(&packet);
}

void Server::SendChangeCharacter(int client_id, PlayerType type)
{
	SC_CHANGE_CHARACTER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_CHARACTER;
	packet.id = client_id;
	packet.player_type = type;

	for (INT id : m_village_ids) {
		if (-1 == id) continue;
		if (client_id == id) continue;

		m_clients[id]->DoSend(&packet);
	}
}

void Server::SendUpdateVillage()
{
	// 마을 위치 업데이트
}

void Server::SendRemoveInVillage(int client_id)
{
	// 마을 내 플레이어 제거
	SC_REMOVE_PLAYER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_PLAYER;
	packet.id = client_id;

	std::unordered_set<int> ids{};
	{
		std::lock_guard<std::mutex> l{ m_village_lock };
		m_village_ids.erase(client_id);
		ids = m_village_ids;
	}

	// 나머지 플레이어들에게 remove
	for (INT player_id : ids) {
		m_clients[player_id]->DoSend(&packet);
	}
}

void Server::SendExitDungeonOk(int client_id)
{
	SC_EXIT_DUNGEON_OK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_EXIT_DUNGEON_OK;

	m_clients[client_id]->DoSend(&packet);
}

bool Server::IsPlayer(int client_id)
{
	if (0 <= client_id && client_id < MAX_USER)
		return true;
	return false;
}

void Server::GameRoomObjectCollisionCheck(const std::shared_ptr<MovementObject>& object,
	int room_num)
{
	BoundingOrientedBox& object_obb = object->GetBoundingBox();

	auto game_room = m_game_room_manager->GetGameRoom(object->GetRoomNum());
	if (!game_room)
		return;

	auto& monster_ids = game_room->GetMonsterIds();

	CollideObject(object, monster_ids, Server::CollideByStatic);

	auto& v = m_game_room_manager->GetStructures();

	for (const auto& obj : v) {
		auto& obb = obj->GetBoundingBox();
		if (object_obb.Intersects(obb)) {
			CollideByStaticOBB(object, obj);
		}
	}
}

int Server::MakeArrow(int client_id, ActionType type)
{
	auto game_room = m_game_room_manager->GetGameRoom(m_clients[client_id]->GetRoomNum());
	if (!game_room) {
		return -1;
	}

	int arrow_id = game_room->GetArrowId();

	SetRemoveArrowTimerEvent(client_id, arrow_id);

	SendArrowShoot(client_id, arrow_id, type);

	return arrow_id;
}

void Server::ProcessArrow(int client_id, int target_id, ActionType type)
{
	int arrow_id = MakeArrow(client_id, type);
	if (-1 == arrow_id)
		return;

	SetHitScanTimerEvent(client_id, type, arrow_id, target_id);
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
	case MonsterType::BOSS:
		start_num = BOSS_MONSTER_START;
		end_num = BOSS_MONSTER_END;
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
		start_num = ARROW_RAIN_START;
		end_num = ARROW_RAIN_END;
		break;
	case TriggerType::UNDEAD_GRASP:
		start_num = UNDEAD_GRASP_START;
		end_num = UNDEAD_GRASP_END;
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

void Server::Move(int client_id, XMFLOAT3 position)
{
	auto client = std::dynamic_pointer_cast<Client>(m_clients[client_id]);
	client->SetPosition(position);

	// 게임 룸에 진입한채 움직이면 시야처리 X (타워 씬)
	int room_num = client->GetRoomNum();

	if (m_game_room_manager->IsValidRoomNum(room_num)) {
		m_game_room_manager->EventCollisionCheck(room_num, client->GetId());
		m_game_room_manager->GetGameRoom(room_num)->CheckTriggerCollision(client->GetId());
	}
	// 게임 룸이 아닌채 움직이면 시야처리 (마을 씬)
	else {
		// 마을 위치 갱신
		client->SetTownPosition(position);

		for (auto id : m_village_ids) {
			if (-1 == id) continue;
			if (id == client_id) continue;
			
			// 기존 플레이어를 나에게 전송
			//SendMovePlayer(client_id, id);

			// 나를 기존 플레이어들에게 전송
			SendMovePlayer(id, client_id);
		}



		//std::unordered_set<INT> near_list;

		//for (auto other_player : m_clients) {
		//	if (other_player->GetId() == id)
		//		continue;
		//	if (false == CanSee(id, other_player->GetId()))
		//		continue;

		//	near_list.insert(other_player->GetId());
		//}

		////lock시간을 줄이기 위해 자료를 복사해서 사용
		//client->m_vl.lock();
		//std::unordered_set<int> my_vl{ client->GetViewList() };
		//client->m_vl.lock();

		//// 움직일때 시야에 들어온 플레이어 추가
		//for (INT other : near_list) {
		//	
		//	if (my_vl.count(other) == 0) {
		//		
		//		client->m_vl.lock();
		//		client->SetViewList(other);
		//		client->m_vl.lock();

		//		// 보였으니 그리라고 패킷을 보냄.
		//		SendAddPlayer(client->GetId(), other);

		//		continue;
		//	}
		//	auto other_player = dynamic_pointer_cast<Client>(m_clients[other]);

		//	other_player->m_vl.lock();

		//	if (other_player->GetViewList().count(client->GetId())) {
		//		other_player->SetViewList(client->GetId());
		//		other_player->m_vl.unlock();
		//		SendAddPlayer(other, client->GetId());
		//	}
		//	else {
		//		// 상대 뷰리스트에 있으면 이동 패킷 전송
		//		other_player->m_vl.unlock();
		//		SendMovePlayer(other, client->GetId());
		//	}
		//}

		
	}
}

bool Server::CanSee(int from, int to)
{
	if (VIEW_RANGE - 1 < abs(m_clients[from]->GetPosition().x - m_clients[to]->GetPosition().x)) return false;
	if (VIEW_RANGE - 1 < abs(m_clients[from]->GetPosition().z - m_clients[to]->GetPosition().z)) return false;

	return true;
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
	else if(pos.z >= RoomSetting::DOWNSIDE_STAIRS_FRONT &&
		pos.z <= RoomSetting::TOPSIDE_STAIRS_BACK)
	{
		if (std::fabs(pos.y - RoomSetting::DEFAULT_HEIGHT) <= std::numeric_limits<float>::epsilon()) {
			return;
		}
		pos.y = RoomSetting::DEFAULT_HEIGHT;
	}
	MoveObject(object, pos);
}

void Server::TimerThread()
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
				ProcessTimerEvent(ev);
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
				ProcessTimerEvent(ev);
				continue;
			}
		}

		// 지역 큐가 비었고, 전역 타이머큐 try_pop 실패 시 대기
		std::this_thread::sleep_for(5ms);
	}
}

void Server::ProcessTimerEvent(const TIMER_EVENT& ev)
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
		if (MonsterBehavior::REMOVE == ev.next_behavior_type) {
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
			switch (monster->GetMonsterType()) {
			case MonsterType::WARRIOR: 
			case MonsterType::WIZARD:
			{
				TIMER_EVENT attack_ev{ .event_time = system_clock::now() + MonsterSetting::ATK_COLLISION_TIME[static_cast<int>(monster->GetMonsterType())],
					.obj_id = ev.obj_id, .target_id = monster->GetRoomNum(),
					.position = Vector3::Add(monster->GetPosition(), monster->GetFront()),
					.event_type = EventType::MONSTER_ATTACK_COLLISION, .action_type = ActionType::NORMAL_ATTACK,
				};

				m_timer_queue.push(attack_ev);
				break;
			}
			case MonsterType::ARCHER: {
				TIMER_EVENT attack_ev{ .event_time = system_clock::now(), .obj_id = ev.obj_id,
					.target_id = monster->GetTargetId(), .event_type = EventType::ARROW_SHOOT,
					.action_type = ActionType::NORMAL_ATTACK };

				m_timer_queue.push(attack_ev);
				break;
			}
			case MonsterType::BOSS: {
				TIMER_EVENT attack_ev{ .event_time = system_clock::now() + MonsterSetting::ATK_COLLISION_TIME[static_cast<int>(monster->GetMonsterType())],
					.obj_id = ev.obj_id, .target_id = monster->GetRoomNum(),
					.position = Vector3::Add(monster->GetPosition(), monster->GetFront()),
					.event_type = EventType::MONSTER_ATTACK_COLLISION, .action_type = ActionType::SKILL,
				};

				m_timer_queue.push(attack_ev);
				break;
			}
			}
		}
		else if (MonsterBehavior::CAST == ev.next_behavior_type) {
			int target = monster->GetTargetId();
			if (target < 0 || target >= MAX_USER) {
				break;
			}

			TIMER_EVENT trigger_ev{ .event_time = std::chrono::system_clock::now()
			+ TriggerSetting::GENTIME[static_cast<int>(TriggerType::UNDEAD_GRASP)],
			.obj_id = ev.obj_id, .target_id = target,
			.position = m_clients[target]->GetPosition(),
			.event_type = EventType::MULTIPLE_TRIGGER_SET, .trigger_type = TriggerType::UNDEAD_GRASP,
			.latest_id = 0, .aggro_level = 3, .is_valid = false };

			m_timer_queue.push(trigger_ev);
		}
		else if (MonsterBehavior::WIDE_SKILL == ev.next_behavior_type || MonsterBehavior::ENHANCE_WIDE_SKILL == ev.next_behavior_type ||
			     MonsterBehavior::ENHANCE_RUN == ev.next_behavior_type) {
			TIMER_EVENT attack_ev{ .event_time = system_clock::now() + MonsterSetting::ATK_COLLISION_TIME[static_cast<int>(monster->GetMonsterType())],
					.obj_id = ev.obj_id, .target_id = monster->GetRoomNum(),
					.position = Vector3::Add(monster->GetPosition(), monster->GetFront()),
					.event_type = EventType::MONSTER_ATTACK_COLLISION, .action_type = ActionType::SKILL,
			};

			m_timer_queue.push(attack_ev);
		}
		else if (MonsterBehavior::NORMAL_ATTACK == ev.next_behavior_type) {
			TIMER_EVENT attack_ev{ .event_time = system_clock::now() + MonsterSetting::ATK_COLLISION_TIME[static_cast<int>(monster->GetMonsterType())],
					.obj_id = ev.obj_id, .target_id = monster->GetRoomNum(),
					.position = Vector3::Add(monster->GetPosition(), monster->GetFront()),
					.event_type = EventType::MONSTER_ATTACK_COLLISION, .action_type = ActionType::NORMAL_ATTACK,
			};

			m_timer_queue.push(attack_ev);
		}
		else if (MonsterBehavior::ULTIMATE_SKILL == ev.next_behavior_type) {
			TIMER_EVENT attack_ev{ .event_time = system_clock::now() + MonsterSetting::ATK_COLLISION_TIME[static_cast<int>(monster->GetMonsterType())],
					.obj_id = ev.obj_id, .target_id = monster->GetRoomNum(),
					.position = Vector3::Add(monster->GetPosition(), monster->GetFront()),
					.event_type = EventType::MONSTER_ATTACK_COLLISION, .action_type = ActionType::ULTIMATE,
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

		if (PlayerType::WARRIOR == client->GetPlayerType()) {	// 전사의 경우 모션중에 타격이 일어나야 함
			switch (ev.action_type) {
			case ActionType::NORMAL_ATTACK:
				if (ObjectAnimation::ATTACK != client->GetCurrentAnimation())
					return;
				break;
			case ActionType::SKILL:
				if (PlayerAnimation::SKILL != client->GetCurrentAnimation() &&
					PlayerAnimation::SKILL2 != client->GetCurrentAnimation())
					return;
				break;
			case ActionType::ULTIMATE:
				if (PlayerAnimation::ULTIMATE != client->GetCurrentAnimation() &&
					PlayerAnimation::ULTIMATE2 != client->GetCurrentAnimation())
					return;
				break;
			}
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
		memcpy(&over->_send_buf[2], &ev.target_id, sizeof(int));

		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::HIT_SCAN: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_HIT_SCAN;
		memcpy(over->_send_buf, &ev.target_id, sizeof(int));
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
		memcpy(over->_send_buf, &ev.action_type, sizeof(ActionType));
		memcpy((over->_send_buf + sizeof(ActionType)), &ev.target_id, sizeof(int));
		memcpy((over->_send_buf + sizeof(ActionType) + sizeof(int)), &ev.position, sizeof(XMFLOAT3));

		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::ARROW_REMOVE: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_ARROW_REMOVE;
		memcpy(over->_send_buf, &ev.target_id, sizeof(int));

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
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_TRIGGER_COOLDOWN;
		memcpy(over->_send_buf, &ev.target_id, sizeof(int));

		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::TRIGGER_REMOVE: {
		// 특정 id의 트리거(obj_id)를 해당 게임룸(target_id)에서 빼고
		// 해당 트리거를 FREE로 상태 변경
		m_game_room_manager->RemoveTrigger(ev.obj_id, ev.target_id);
		{
			std::lock_guard<std::mutex> l{ m_triggers[ev.obj_id]->GetStateMutex() };
			m_triggers[ev.obj_id]->SetState(State::FREE);
		}
		break;
	}
	case EventType::MULTIPLE_TRIGGER_SET: {
		// 트리거 생성 반복
		// latest_id 를 현재 생성횟수, aggro_level 을 최대 생성 횟수로 체크
		if (ev.latest_id < ev.aggro_level) {
			ExpOver* over = new ExpOver;
			over->_comp_type = OP_MULTIPLE_TRIGGER_SET;
			memcpy(&over->_send_buf, &ev.target_id, sizeof(int));
			memcpy((over->_send_buf + sizeof(int)), &ev.trigger_type, sizeof(TriggerType));
			memcpy((over->_send_buf + sizeof(int) + sizeof(TriggerType)), &ev.latest_id, sizeof(BYTE));
			memcpy((over->_send_buf + sizeof(int) + sizeof(TriggerType) + sizeof(BYTE)),
				&ev.aggro_level, sizeof(BYTE));

			PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		}

		if(ev.is_valid)
			SetTrigger(ev.obj_id, ev.trigger_type, ev.position);

		break;
	}
	case EventType::BATTLE_START: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_BATTLE_START;
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}
	case EventType::PORTAL_WARP: {
		ExpOver* over = new ExpOver;
		over->_comp_type = OP_PORTAL_WARP;
		PostQueuedCompletionStatus(m_handle_iocp, 1, ev.obj_id, &over->_wsa_over);
		break;
	}

	
	}
}

void Server::DBThread()
{
	using namespace std::chrono;
	DB_EVENT ev{};

	while (true) {
		auto current_time = system_clock::now();

		if (m_db_queue.try_pop(ev)) {
			if (ev.event_time > current_time) {
				m_db_queue.push(ev);
				std::this_thread::sleep_for(5ms);
				continue;
			}
			else{
				switch (ev.event_type) {
				case DBEventType::TRY_LOGIN: {
					ExpOver* over = new ExpOver;

					USER_INFO user_info{};
					user_info.user_id = ev.data.user_id;
					user_info.password = ev.data.password;

					PLAYER_DATA data{};

					if (m_database->TryLogin(user_info, data)) {
						auto client = dynamic_pointer_cast<Client>(m_clients[ev.client_id]);

						client->SetUserId(ev.data.user_id);
						std::string name{};
						client->SetName(data.name);
						client->SetPlayerType(static_cast<PlayerType>(data.player_type));
						client->SetPosition(data.x, data.y, data.z);
						client->SetTownPosition(data.x, data.y, data.z);

						// 골드정보 Set
						client->SetGold(data.gold);

						// 강화정보 Set
						client->SetHpLevel(data.hp_level);
						client->SetAtkLevel(data.atk_level);
						client->SetDefLevel(data.def_level);
						client->SetCritRateLevel(data.crit_rate_level);
						client->SetCritDamageLevel(data.crit_damage_level);

						// 스킬 정보 Set
						for (size_t i = 0; i < (INT)PlayerType::COUNT; ++i) {
							client->SetNormalSkillType((PlayerType)i, data.normal_skill_type[i]);
							client->SetUltimateSkillType((PlayerType)i, data.ultimate_type[i]);
						}

						over->_comp_type = OP_LOGIN_OK;
						PostQueuedCompletionStatus(m_handle_iocp, 1, ev.client_id, &over->_wsa_over);
					}
					else {
						over->_comp_type = OP_LOGIN_FAIL;
						PostQueuedCompletionStatus(m_handle_iocp, 1, ev.client_id, &over->_wsa_over);
					}
					break;
				}
				case DBEventType::SAVE_USER_DATA:
					m_database->SaveUserData(ev.data);
					break;
				case DBEventType::CREATE_ACCOUNT: {
					ExpOver* over = new ExpOver;

					USER_INFO user_info{};
					user_info.user_id = ev.data.user_id;
					user_info.password = ev.data.password;

					if (m_database->CreateAccount(user_info)) {		// 회원가입 성공
						over->_comp_type = OP_SIGNIN_OK;
						PostQueuedCompletionStatus(m_handle_iocp, 1, ev.client_id, &over->_wsa_over);
					}
					else {											// 회원가입 실패
						over->_comp_type = OP_SIGNIN_FAIL;
						PostQueuedCompletionStatus(m_handle_iocp, 1, ev.client_id, &over->_wsa_over);
					}
					break;
				}
				case DBEventType::DELETE_ACCOUNT: {
					USER_INFO user_info{};
					user_info.user_id = ev.data.user_id;
					user_info.password = ev.data.password;

					if (!m_database->DeleteAccount(user_info)) {

					}
					break; 
				}
				case DBEventType::UPDATE_PLAYER_INFO: {
					PLAYER_INFO player_info{};

					if (!m_database->UpdatePlayer(player_info)) {

					}
					break;
				}
				case DBEventType::UPDATE_SKILL_INFO: {
					SKILL_INFO skill_info{};

					if (!m_database->UpdateSkill(skill_info)) {

					}
					break;
				}
				case DBEventType::UPDATE_UPGRADE_INFO: {
					UPGRADE_INFO upgrade_info{};

					if (!m_database->UpdateUpgrade(upgrade_info)) {

					}
					break;
				}
				case DBEventType::UPDATE_GOLD: {
					auto client = dynamic_pointer_cast<Client>(m_clients[ev.client_id]);
					if (!m_database->UpdateGold(ev.data.user_id, client->GetGold())) {

					}
					break;
				}
				case DBEventType::UPDATE_POSITION: {
					auto client = dynamic_pointer_cast<Client>(m_clients[ev.client_id]);
					XMFLOAT3 pos = client->GetPosition();

					if (!m_database->UpdatePosition(ev.data.user_id, pos.x, pos.y, pos.z)) {

					}
					break;
				}
				}

				continue;
			}
		}
		
		std::this_thread::sleep_for(5ms);
	}
}

void Server::SetTimerEvent(const TIMER_EVENT& ev)
{
	m_timer_queue.push(ev);
}

void Server::SetDatabaseEvent(const DB_EVENT& ev)
{
	m_db_queue.push(ev);
}

void Server::SetAttackTimerEvent(int id, ActionType attack_type,
	std::chrono::system_clock::time_point attack_time)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);
	int player_type = static_cast<int>(client->GetPlayerType());

	TIMER_EVENT ev{ .obj_id = id, .action_type = attack_type };
	ev.event_type = EventType::ATTACK_COLLISION;

	switch (client->GetPlayerType()) {
	case PlayerType::WARRIOR:

		switch (attack_type) {
		case ActionType::NORMAL_ATTACK:
			ev.event_time = attack_time + PlayerSetting::ATTACK_COLLISION_TIME[player_type];
			ev.position = Vector3::Add(client->GetPosition(), Vector3::Mul(client->GetFront(), 0.8f));
			break;
		case ActionType::SKILL:
			ev.event_time = attack_time + 
				PlayerSetting::SKILL_COLLISION_TIME[player_type][client->GetNormalSkillType()];
			ev.position = Vector3::Add(client->GetPosition(), Vector3::Mul(client->GetFront(), 0.8f));
			break;
		case ActionType::ULTIMATE:
			ev.event_time = attack_time + 
				PlayerSetting::ULTIMATE_COLLISION_TIME[player_type][client->GetUltimateSkillType()];
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

	auto client = dynamic_pointer_cast<Client>(m_clients[id]);

	// 쿨타임 중인지 검사 필요
	// 공격 외 쿨타임 처리
	switch (action_type) {
	case ActionType::NORMAL_ATTACK:
		client->SetInvincibleRoll(false);
		ev.event_time += PlayerSetting::ATTACK_COOLDOWN[player_type];
		break;
	case ActionType::SKILL:
		client->SetInvincibleRoll(false);
		ev.event_time += PlayerSetting::SKILL_COOLDOWN[player_type];
		break;
	case ActionType::ULTIMATE:
		client->SetInvincibleRoll(false);
		ev.event_time += PlayerSetting::ULTIMATE_COOLDOWN[player_type];
		break;
	case ActionType::DASH:
		client->SetInvincibleRoll(false);
		ev.event_time += PlayerSetting::DASH_COOLDOWN;
		break;
	case ActionType::ROLL:
		client->SetInvincibleRoll(true);
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
			ev.target_id = -PlayerSetting::ROLL_STAMINA_CHANGE_AMOUNT;
		}
		else {
			ev.target_id = -PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;
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
		ev.target_id = PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT;

		BYTE latest_id{ client->GetLatestId() };
		if (std::numeric_limits<BYTE>::max() == latest_id)
			latest_id = 0;
		ev.latest_id = ++latest_id;
		client->SetLatestId(latest_id);

		m_timer_queue.push(ev);
	}
}

// 타겟을 향해 날림
void Server::SetHitScanTimerEvent(int id, ActionType action_type, int arrow_id, int target_id)
{
	if(-1 == target_id)
		return;

	XMFLOAT3 dir = Vector3::Sub(m_clients[target_id]->GetPosition(),
		m_clients[id]->GetPosition());

	SetHitScanTimerEvent(id, action_type, arrow_id, dir);
}

// 방향을 향해 날림 
void Server::SetHitScanTimerEvent(int id, ActionType action_type, int arrow_id, const XMFLOAT3& dir)
{
	auto now = std::chrono::system_clock::now();

	float length = Vector3::Length(dir);
	float speed{};

	if (IsPlayer(id)) {
		speed = PlayerSetting::ARROW_SPEED;
	}
	else {
		speed = MonsterSetting::ARROW_SPEED;
	}
	float time = length * 1'000 / speed;		// ms
	std::chrono::milliseconds ms{static_cast<long long>(time)};

	TIMER_EVENT ev{ .event_time = now + ms, .obj_id = id, .target_id = arrow_id,
		.position = m_clients[id]->GetPosition(), .direction = Vector3::Normalize(dir),
		.event_type = EventType::HIT_SCAN, .action_type = action_type };

	m_timer_queue.push(ev);
}

void Server::SetArrowShootTimerEvent(int id, ActionType attack_type, 
	std::chrono::system_clock::time_point attack_time)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[id]);
	
	float target_range{};
	if (IsPlayer(id)) {
		target_range = PlayerSetting::ARROW_RANGE;
	}
	else {
		target_range = MonsterSetting::ARROW_RANGE;
	}

	int target = GetNearTarget(id, target_range);

	// 타겟방향 회전
	if (-1 != target) {									
		XMVECTOR z_axis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };

		XMFLOAT3 sub = Vector3::Sub(m_clients[target]->GetPosition(), m_clients[id]->GetPosition());
		sub.y = 0.f;
		sub = Vector3::Normalize(sub);
		XMVECTOR radian{ XMVector3AngleBetweenNormals(z_axis, XMLoadFloat3(&sub)) };
		FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };

		XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, XMVectorGetX(radian), 0.f);
		XMFLOAT4 q{};
		XMStoreFloat4(&q, vec);
		m_clients[id]->SetBoundingBoxOrientation(XMFLOAT4{ q.x, q.y, q.z, q.w });
		m_clients[id]->SetYaw(sub.x < 0 ? -angle : angle);
	}	

	TIMER_EVENT ev{ .obj_id = id, .target_id = target,
		.event_type = EventType::ARROW_SHOOT, .action_type = attack_type };

	int type = static_cast<int>(PlayerType::ARCHER);
	switch (attack_type) {
	case ActionType::NORMAL_ATTACK:
		ev.event_time = attack_time + PlayerSetting::ATTACK_COLLISION_TIME[type];
		break;
	case ActionType::SKILL:
		switch (client->GetNormalSkillType()) {
		case 0:
			ev.event_time = attack_time +
				PlayerSetting::SKILL_COLLISION_TIME[type][client->GetNormalSkillType()];
			break;

		case 1:
			using namespace std::literals;

			for (size_t i = 0; i < 3; ++i) {
				ev.event_time = attack_time +
					PlayerSetting::SKILL_COLLISION_TIME[type][client->GetNormalSkillType()] +
					(100ms * i);
				m_timer_queue.push(ev);
			}
			return;
		}
		break;
	case ActionType::ULTIMATE:
		ev.event_time = attack_time + 
			PlayerSetting::ULTIMATE_COLLISION_TIME[type][client->GetUltimateSkillType()];
		ev.position = Vector3::Add(m_clients[id]->GetPosition(),
			Vector3::Mul(m_clients[id]->GetFront(), TriggerSetting::ARROWRAIN_DIST));
		break;
	}
	m_timer_queue.push(ev);
}

void Server::SetRemoveArrowTimerEvent(int client_id, int arrow_id)
{
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + RoomSetting::ARROW_REMOVE_TIME,
		.obj_id = client_id, .target_id = arrow_id,
		.event_type = EventType::ARROW_REMOVE };

	m_timer_queue.push(ev);
}

void Server::SetBattleStartTimerEvent(int client_id)
{
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + RoomSetting::BATTLE_DELAY_TIME,
		 .obj_id = client_id, .event_type = EventType::BATTLE_START };

	m_timer_queue.push(ev);
}

void Server::SetWarpTimerEvent(int client_id)
{
	using namespace std::literals;
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + RoomSetting::WARP_DELAY_TIME,
		.obj_id = client_id, .event_type = EventType::PORTAL_WARP };

	m_timer_queue.push(ev);
}

void Server::SetTrigger(int client_id, TriggerType type, const XMFLOAT3& pos)
{
	int room_num = m_clients[client_id]->GetRoomNum();
	auto game_room = m_game_room_manager->GetGameRoom(room_num);
	if (!game_room)
		return;

	int trigger_id = GetNewTriggerId(type);
	float damage{};

	game_room->AddTrigger(trigger_id);

	switch (type) {
	case TriggerType::ARROW_RAIN:
		damage = m_clients[client_id]->GetDamage() *
			m_clients[client_id]->GetSkillRatio(ActionType::ULTIMATE);
		m_triggers[trigger_id]->Create(damage, client_id, pos, room_num);
		break;

	case TriggerType::UNDEAD_GRASP:
		auto monster = dynamic_pointer_cast<Monster>(m_clients[client_id]);
		damage = m_clients[client_id]->GetDamage();
		m_triggers[trigger_id]->Create(damage, client_id, pos, room_num);
		break;
	}

	SendTrigger(client_id, type, pos);
}

void Server::SetTrigger(int client_id, TriggerType type, int target_id)
{
	SetTrigger(client_id, type, m_clients[target_id]->GetPosition());
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

void Server::AttackCollisionWithMonster(int client_id, const BoundingOrientedBox& obb, ActionType type)
{
	auto client = dynamic_pointer_cast<Client>(m_clients[client_id]);

	int room_num = client->GetRoomNum();

	auto game_room = m_game_room_manager->GetGameRoom(room_num);
	if (!game_room) {
		return;
	}

	// 충돌된 몬스터 빼내는 컨테이너
	std::vector<int> v;
	v.reserve(MAX_INGAME_MONSTER);
	FLOAT damage{ client->GetDamage() };

	damage *= client->GetSkillRatio(type);

	auto& monster_ids = game_room->GetMonsterIds();

	for (int id : monster_ids) {
		if (-1 == id) continue;
		if (State::INGAME != m_clients[id]->GetState()) continue;

		if (m_clients[id]->GetBoundingBox().Intersects(obb)) {
			auto monster = dynamic_pointer_cast<Monster>(m_clients[id]);
			
			if (DecreaseState::DECREASE == monster->DecreaseHp(damage, client->GetId())) {
				v.push_back(id);
			}
		}
	}

	for (int monster_id : v) {
		SendChangeHp(monster_id);
	}
}
 

XMFLOAT3 Server::GetRotateDirection(int client_id, FLOAT value) const
{
	XMFLOAT3 pos = m_clients[client_id]->GetPosition();
	XMFLOAT3 front = m_clients[client_id]->GetFront();
	XMFLOAT3 dir = Vector3::Normalize(Vector3::Sub(front, pos));

	XMFLOAT4 q{};
	XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(0.f, value, 0.f));
	XMVECTOR rotation = XMVector3Rotate(XMLoadFloat3(&dir), XMLoadFloat4(&q));
	
	XMFLOAT3 result{};
	XMStoreFloat3(&result, rotation);
	return result;
}