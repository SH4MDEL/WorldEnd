#include <fstream> 
#include <unordered_map>
#include "Server.h"
#include "map.h"

std::uniform_int_distribution<INT> dist(0, 100);

GameRoom::GameRoom()
{
	Init();

	m_battle_starter = std::make_shared<BattleStarter>();
	m_portal = std::make_shared<WarpPortal>();
}

void GameRoom::Update(FLOAT elapsed_time)
{
	Server& server = Server::GetInstance();

	std::array<INT, MAX_INGAME_MONSTER> ids{};
	{
		std::lock_guard<std::mutex> lock{ m_monster_lock };
		ids = m_monster_ids;
	}

	for (INT id : ids) {
		if (-1 == id) continue;

		server.m_clients[id]->Update(elapsed_time);
	}
}

void GameRoom::InteractObject(InteractionType type)
{
	switch (type) {
	case InteractionType::BATTLE_STARTER:
		{
			std::lock_guard<std::mutex> l{ m_state_lock };
			if (m_state != GameRoomState::INGAME)
				m_state = GameRoomState::INGAME;
		}
		m_battle_starter->SendEvent(m_ingame_player_ids, &type);
		break;
	case InteractionType::PORTAL:
		//
		break;
	}
}

void GameRoom::StartBattle()
{
	Server& server = Server::GetInstance();

	for (INT id : m_monster_ids) {
		if (-1 == id) continue;

		auto monster = dynamic_pointer_cast<Monster>(server.m_clients[id]);

		monster->SetState(State::INGAME);
		monster->ChangeBehavior(MonsterBehavior::CHASE);
		monster->UpdateTarget();
	}
}

void GameRoom::WarpNextFloor(INT room_num)
{
	Server& server = Server::GetInstance();

	m_portal->SendEvent(m_ingame_player_ids, &m_floor);
	m_battle_starter->SetValid(true);

	++m_floor;
	InitGameRoom(room_num);

	// 방 넘어갈 때 생성한 몬스터 정보 다시 넘기기
	for (INT id : m_ingame_player_ids) {
		if (-1 == id) continue;

		SendAddMonster(id);
	}
}

void GameRoom::DungeonClear()
{
	Server& server = Server::GetInstance();

	SC_DUNGEON_CLEAR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_DUNGEON_CLEAR;

	for (INT id : m_ingame_player_ids) {
		if (-1 == id) continue;

		auto client = std::dynamic_pointer_cast<Client>(server.m_clients[id]);
		client->SetHp(server.m_clients[id]->GetMaxHp());
		client->SetRoomNum(-1);
		client->SetPosition(client->GetTownPosition());
		packet.position = client->GetPosition();

		server.m_clients[id]->DoSend(&packet);
	}

	// 방 초기화
	Init();
}

// 빈자리를 찾아 진입
bool GameRoom::SetPlayer(INT room_num, INT player_id)
{
	Server& server = Server::GetInstance();

	bool join{ false };

	// 한번에 한명씩 빈자리에 넣는 것을 보장
	// 빈자리를 찾았을 경우 id를 넣고 락을 해제하지만
	// 빈자리를 못찾으면 락을 함수 종료시점에 해제해줌
	std::unique_lock<std::mutex> l{ m_player_lock };
	for (INT& id : m_ingame_player_ids) {
		if (-1 == id) {
			id = player_id;
			server.m_clients[id]->SetRoomNum(room_num);

			// 플레이어 진입 시 3명이 되었을 경우 INGAME으로 변경
			INT count = GetPlayerCount();
			if (count == MAX_INGAME_USER) {
				std::unique_lock<std::mutex> lock{ m_state_lock };
				if (GameRoomState::INGAME != m_state) {
					m_state = GameRoomState::INGAME;
				}
			}
			l.unlock();
			server.m_clients[id]->SetPosition(RoomSetting::TOWER_START_POSITION);
			join = true;

			if (GameRoomState::ONBATTLE == m_state) {
				// 전투가 이미 시작되었음을 전송하는 것이 필요
			}
			break;
		}
	}

	return join;
}

std::shared_ptr<BattleStarter> GameRoom::GetBattleStarter() const
{
	return m_battle_starter;
}

std::shared_ptr<WarpPortal> GameRoom::GetWarpPortal() const
{
	return m_portal;
}

INT GameRoom::GetArrowId()
{
	std::lock_guard<std::mutex> lock{ m_arrow_lock };
	INT id = m_arrow_id;
	m_arrow_id = (m_arrow_id + 1) % RoomSetting::MAX_ARROWS;

	return id;
}

void GameRoom::SendAddPlayer(INT sender, INT receiver)
{
	Server& server = Server::GetInstance();

	SC_ADD_PLAYER_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ADD_PLAYER;
	packet.id = sender;
	packet.pos = server.m_clients[sender]->GetPosition();
	packet.hp = server.m_clients[sender]->GetHp();
	packet.player_type = server.m_clients[sender]->GetPlayerType();
	//strcpy_s(packet.name, sizeof(packet.name), server.m_clients[sender]->GetName().c_str());

	server.m_clients[receiver]->DoSend(&packet);
}

void GameRoom::SendPlayerData()
{
	Server& server = Server::GetInstance();

	SC_UPDATE_CLIENT_PACKET packet[MAX_INGAME_USER]{};

	for (size_t i = 0; INT id : m_ingame_player_ids) {
		if (-1 == id) {
			packet[i].size = sizeof(SC_UPDATE_CLIENT_PACKET);
			packet[i].type = SC_PACKET_UPDATE_CLIENT;
			packet[i].id = -1;
		}
		else {
			packet[i].size = sizeof(SC_UPDATE_CLIENT_PACKET);
			packet[i].type = SC_PACKET_UPDATE_CLIENT;
			packet[i].id = server.m_clients[id]->GetId();
			packet[i].pos = server.m_clients[id]->GetPosition();
			packet[i].yaw = server.m_clients[id]->GetYaw();
			//packet[player_count].move_time = server.m_clients[id]->GetLastMoveTime();
		}
		++i;
	}

	for (INT id : m_ingame_player_ids) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet, MAX_INGAME_USER);
	}
}

void GameRoom::SendMonsterData()
{
	Server& server = Server::GetInstance();

	SC_UPDATE_MONSTER_PACKET packet[MAX_INGAME_MONSTER]{};

	INT monster_count{};
	std::array<INT, MAX_INGAME_MONSTER> ids{};
	{
		std::lock_guard<std::mutex> lock{ m_monster_lock };
		ids = m_monster_ids;
		monster_count = static_cast<INT>(m_monster_count.load());
	}

	for (size_t i = 0; INT id : ids) {
		packet[i].size = static_cast<UCHAR>(sizeof(SC_UPDATE_MONSTER_PACKET));
		packet[i].type = SC_PACKET_UPDATE_MONSTER;

		if (-1 == id) {
			packet[i].monster_data = MONSTER_DATA{ .id = -1 };
		}
		else {
			packet[i].monster_data = server.m_clients[id]->GetMonsterData();
		}
		++i;
	}

	for (INT id : m_ingame_player_ids){
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet, monster_count);
	}
}

void GameRoom::SendAddMonster(INT player_id)
{
	Server& server = Server::GetInstance();

	SC_ADD_MONSTER_PACKET packet[MAX_INGAME_MONSTER]{};

	INT monster_count{};
	std::array<INT, MAX_INGAME_MONSTER> ids{};
	{
		std::lock_guard<std::mutex> lock{ m_monster_lock };
		ids = m_monster_ids;
		monster_count = static_cast<INT>(m_monster_count.load());
	}

	for (size_t i = 0; INT id : ids) {
		packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		packet[i].type = SC_PACKET_ADD_MONSTER;

		if (-1 == id) {
			packet[i].monster_data = MONSTER_DATA{ .id = -1 };
		}
		else {
			packet[i].monster_data = server.m_clients[id]->GetMonsterData();
			packet[i].hp = server.m_clients[id]->GetHp();
			packet[i].monster_type = server.m_clients[id]->GetMonsterType();
		}
		++i;
	}

	server.m_clients[player_id]->DoSend(&packet, monster_count);
	
	if (GameRoomState::ONBATTLE == m_state) {
		//m_battle_starter->SendEvent(player_id, nullptr);
	}
}

void GameRoom::SendAddMonster()
{
	Server& server = Server::GetInstance();

	SC_ADD_MONSTER_PACKET packet[MAX_INGAME_MONSTER]{};

	INT monster_count{};
	std::array<INT, MAX_INGAME_MONSTER> ids{};
	{
		std::lock_guard<std::mutex> lock{ m_monster_lock };
		ids = m_monster_ids;
		monster_count = static_cast<INT>(m_monster_count.load());
	}

	for (size_t i = 0; INT id : ids) {
		packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		packet[i].type = SC_PACKET_ADD_MONSTER;

		if (-1 == id) {
			packet[i].monster_data = MONSTER_DATA{ .id = -1 };
		}
		else {
			packet[i].monster_data = server.m_clients[id]->GetMonsterData();
			packet[i].hp = server.m_clients[id]->GetHp();
			packet[i].monster_type = server.m_clients[id]->GetMonsterType();
		}
		++i;
	}

	for (INT id : m_ingame_player_ids) {
		if (-1 == id) continue;
		
		server.m_clients[id]->DoSend(&packet, monster_count);
	}


	if (GameRoomState::ONBATTLE == m_state) {
		//m_battle_starter->SendEvent(player_id, nullptr);
	}
}

void GameRoom::RemovePlayer(INT player_id, INT room_num)
{
	if (player_id < 0 || player_id >= MAX_USER)
		return;

	{
		std::lock_guard<std::mutex> lock{ m_player_lock };
		auto it = std::remove(m_ingame_player_ids.begin(), m_ingame_player_ids.end(), player_id);
		if (it != m_ingame_player_ids.end()) {
			*it = -1;
		}
	}

	// 방이 아예 비었다면
	if (-1 == m_ingame_player_ids[0]) {
		Server& server = Server::GetInstance();

		TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() +
			RoomSetting::RESET_TIME, .obj_id = room_num,
			.event_type = EventType::GAME_ROOM_RESET };

		server.SetTimerEvent(ev);
	}
	else {
		Server& server = Server::GetInstance();

		SC_REMOVE_PLAYER_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_REMOVE_PLAYER;
		packet.id = player_id;

		for (INT id : m_ingame_player_ids) {
			if (-1 == id) continue;
			{
				std::lock_guard<std::mutex> lock{ server.m_clients[id]->GetStateMutex() };
				if (State::INGAME != server.m_clients[id]->GetState()) continue;
			}

			server.m_clients[id]->DoSend(&packet);
		}
	}
}

void GameRoom::RemoveMonster(INT monster_id)
{
	if (monster_id < MAX_USER || monster_id >= MAX_OBJECT)
		return;

	Server& server = Server::GetInstance();

	INT monster_count{ -1 }, index{ -1 };
	{
		std::lock_guard<std::mutex> lock{ m_monster_lock };
		auto it = std::remove(m_monster_ids.begin(), m_monster_ids.end(), monster_id);
		if (it != m_monster_ids.end()) {
			*it = -1;
			monster_count = --m_monster_count;
		}
	}

	// 몬스터가 없다면 층 클리어
	if (0 == monster_count) {
		INT room_num = server.m_clients[monster_id]->GetRoomNum();
		ExpOver* over = new ExpOver();
		over->_comp_type = OP_FLOOR_CLEAR;
		PostQueuedCompletionStatus(server.GetIOCPHandle(), 1, room_num, &over->_wsa_over);
	}

	// 해당 몬스터 상태 FREE
	{
		std::lock_guard<std::mutex> lock{ server.m_clients[monster_id]->GetStateMutex() };
		auto monster = dynamic_pointer_cast<Monster>(server.m_clients[monster_id]);
		monster->SetState(State::FREE);
		monster->SetBehaviorId(0);
	}
}

void GameRoom::AddTrigger(INT trigger_id)
{
	std::lock_guard<std::mutex> lock{ m_trigger_lock };
	m_trigger_list.insert(trigger_id);
}

void GameRoom::RemoveTrigger(INT trigger_id)
{
	{
		std::lock_guard<std::mutex> lock{ m_trigger_lock };
		m_trigger_list.erase(trigger_id);
	}
	
	Server& server = Server::GetInstance();

	UCHAR trigger = static_cast<UCHAR>(server.m_triggers[trigger_id]->GetType());
	for (INT id : m_ingame_player_ids) {
		if (-1 == id) continue;
		if (State::INGAME != server.m_clients[id]->GetState()) continue;

		server.m_clients[id]->SetTriggerFlag(trigger, false);
	}
}

void GameRoom::CheckEventCollision(INT player_id)
{
	// 전투 오브젝트와 충돌검사
	if (GameRoomState::INGAME == m_state || 
		GameRoomState::ACCEPT == m_state) 
	{
		CollideWithEventObject(player_id, InteractionType::BATTLE_STARTER);
	}
	// 포탈과 충돌검사
	else if (GameRoomState::CLEAR == m_state) {
		if(m_floor == RoomSetting::BOSS_FLOOR)
			CollideWithEventObject(player_id, InteractionType::DUNGEON_CLEAR);
		else
			CollideWithEventObject(player_id, InteractionType::PORTAL);
	}
}

void GameRoom::CheckTriggerCollision(INT id)
{
	Server& server = Server::GetInstance();

	std::unordered_set<INT> trigger_list{};
	{
		std::lock_guard<std::mutex> l{ m_trigger_lock };
		trigger_list = m_trigger_list;
	}

	BoundingOrientedBox obb{};
	for (INT trigger_id : trigger_list) {
		if (State::INGAME != server.m_triggers[trigger_id]->GetState()) continue;
		
		obb = server.m_triggers[trigger_id]->GetEventBoundingBox();

		// 충돌했을 때 트리거 플래그가 0이면 Activate
		if (obb.Intersects(server.m_clients[id]->GetBoundingBox())) {
			server.m_triggers[trigger_id]->Activate(id);
		}
	}
}

INT GameRoom::GenerateRandomRoom(std::set<INT>& save_room, INT min, INT max)
{
	INT rand_value;

	do {
		rand_value = dist(g_random_engine) % max;
	} while (save_room.find(rand_value) != save_room.end());

	save_room.insert(rand_value);
	if (save_room.size() > (max - min)) {
		save_room.erase(save_room.begin());
	}

	return rand_value;
}

void GameRoom::Init()
{
	{
		std::lock_guard<std::mutex> l{ m_state_lock };
		m_state = GameRoomState::EMPTY;
	}
	//m_floor = RoomSetting::BOSS_FLOOR;
	m_floor = 3;
	m_type = EnvironmentType::FOG;
	m_monster_count = 0;
	m_arrow_id = 0;

	for (INT& id : m_ingame_player_ids)
		id = -1;

	for (INT& id : m_monster_ids)
		id = -1;
}

void GameRoom::InitGameRoom(INT room_num)
{
	InitMonsters(room_num);
	InitEnvironment();
	m_battle_starter->SetValid(true);

	// 게임 시작 시 시작 시간 기록
	m_start_time = std::chrono::system_clock::now();
}

void GameRoom::InitMonsters(INT room_num)
{
	using namespace std;

	Server& server = Server::GetInstance();

	// 파일을 읽어서 몬스터를 생성
	ifstream in{ "MonsterPos.txt" };

	FLOAT mon_pos[MAX_MONSTER_PLACEMENT]{};

	for (int i = 0; i < MAX_MONSTER_PLACEMENT; ++i) {
		in >> mon_pos[i];
	}

    INT random_map = GameRoom::GenerateRandomRoom(m_save_room, 0, 7);

	// Init 은 플레이어 진입 시 불려야함
	INT new_warrior_id{};
	INT new_archer_id{};
	INT new_wizard_id{};
	INT new_boss_id{};

	if ((INT)m_floor == (INT)RoomSetting::BOSS_FLOOR)         // 보스방 진입
	{

		random_map = 10;

		new_boss_id = server.GetNewMonsterId(MonsterType::BOSS);

		m_monster_ids[0] = new_boss_id;

		auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_boss_id]);
		monster->Init();

		monster->SetId(new_boss_id);
		monster->InitializePosition(0, MonsterType::BOSS, random_map, mon_pos);
		monster->SetRoomNum(room_num);
		monster->SetTarget(-1);
		monster->SetState(State::ACCEPT);
		++m_monster_count;
	}

	if (random_map == 0)
	{
		for (size_t i = 0; i < mon_pos[1]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[9]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 3] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 3, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[15]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 5] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 5, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
		
	}
	else if (random_map == 1)
	{
		for (size_t i = 0; i < mon_pos[21]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[27]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 2] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 2, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[35]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 5] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 5, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}
	else if (random_map == 2)
	{
		for (size_t i = 0; i < mon_pos[41]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[47]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 2] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 2, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[53]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 4] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 4, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}
	else if (random_map == 3)
	{
		for (size_t i = 0; i < mon_pos[61]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[69]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 3] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 3, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[75]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 5] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 5, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}
	else if (random_map == 4)
	{
		for (size_t i = 0; i < mon_pos[81]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[87]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 2] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 2, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[97]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 6] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 6, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}
	else if (random_map == 5)
	{
		for (size_t i = 0; i < mon_pos[103]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[111]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 3] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 3, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[117]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 5] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 5, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}
	else if (random_map == 6)
	{
		for (size_t i = 0; i < mon_pos[123]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[131]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 3] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 3, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[137]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 5] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 5, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}
	else if (random_map == 7)
	{
		for (size_t i = 0; i < mon_pos[143]; ++i) {
			new_warrior_id = server.GetNewMonsterId(MonsterType::WARRIOR);

			m_monster_ids[i] = new_warrior_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_warrior_id]);
			monster->Init();

			monster->SetId(new_warrior_id);
			monster->InitializePosition(i, MonsterType::WARRIOR, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}

		for (size_t i = 0; i < mon_pos[151]; ++i) {
			new_archer_id = server.GetNewMonsterId(MonsterType::ARCHER);

			m_monster_ids[i + 3] = new_archer_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_archer_id]);
			monster->Init();

			monster->SetId(new_archer_id);
			monster->InitializePosition(i + 3, MonsterType::ARCHER, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}


		for (size_t i = 0; i < mon_pos[159]; ++i) {
			new_wizard_id = server.GetNewMonsterId(MonsterType::WIZARD);

			m_monster_ids[i + 6] = new_wizard_id;

			auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_wizard_id]);
			monster->Init();

			monster->SetId(new_wizard_id);
			monster->InitializePosition(i + 6, MonsterType::WIZARD, random_map, mon_pos);
			monster->SetRoomNum(room_num);
			monster->SetTarget(-1);
			monster->SetState(State::ACCEPT);
			++m_monster_count;
		}
	}

	//m_floor_cnt++;

	cout << random_map << "번 방" << endl;
}

void GameRoom::InitEnvironment()
{
	// 랜덤하게 환경을 설정함
}

void GameRoom::Reset()
{
	Server& server = Server::GetInstance();

	for (int& id : m_monster_ids) {
		if (-1 == id) continue;

		std::lock_guard<std::mutex> lock{ server.m_clients[id]->GetStateMutex() };
		server.m_clients[id]->SetState(State::FREE);
		id = -1;
	}
	m_monster_count = 0;


	// 방을 비어 있는 상태로 변경
	std::lock_guard<std::mutex> lock{ m_state_lock };
	m_state = GameRoomState::EMPTY;
}

void GameRoom::CollideWithEventObject(INT player_id, InteractionType type)
{
	std::shared_ptr<EventObject> object{};
	switch(type) {
	case InteractionType::BATTLE_STARTER:
		object = m_battle_starter;
		break;
	case InteractionType::PORTAL:
	case InteractionType::DUNGEON_CLEAR:
		object = m_portal;
		break;
	}

	Server& server = Server::GetInstance();
	auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);

	if (!object->GetValid()) {
		client->SetInteractable(false);
		return;
	}

	SC_SET_INTERACTABLE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_SET_INTERACTABLE;
	packet.interactable_type = type;

	if (client->GetBoundingBox().Intersects(object->GetEventBoundingBox())) {
		if (!client->GetInteractable()) {
			client->SetInteractable(true);
			packet.interactable = true;
			client->DoSend(&packet);
		}
	}
	else {
		if (client->GetInteractable()) {
			client->SetInteractable(false);
			packet.interactable = false;
			client->DoSend(&packet);
		}
	}
}

INT GameRoom::GetPlayerCount()
{
	INT count = std::count_if(m_ingame_player_ids.begin(), m_ingame_player_ids.end(), [](INT id) {
		return id != -1;
		});

	return count;
}

GameRoomManager::GameRoomManager()
{
	//LoadMap();
	for (auto& game_room : m_game_rooms) {
		game_room = std::make_shared<GameRoom>();
	}
}

bool GameRoomManager::FillPlayer(INT player_id)
{
	INT room_num{ FindCanAccessRoom() };
	while (room_num != -1) {

		if (SetPlayer(room_num, player_id))
			return true;
		else
			room_num = FindCanAccessRoom();
	}
	return false;
}

// gameroom 의 Setplayer 와 마찬가지로 파티 생성 없이 진입할 때
// 호출되어야 할 함수
bool GameRoomManager::SetPlayer(INT room_num, INT player_id)
{
	{
		std::unique_lock<std::mutex> lock{ m_game_rooms[room_num]->GetStateMutex() };
		if (GameRoomState::EMPTY == m_game_rooms[room_num]->GetState()) {
			m_game_rooms[room_num]->SetState(GameRoomState::ACCEPT);
			lock.unlock();
			m_game_rooms[room_num]->InitGameRoom(room_num);
		}
	}
	
	return m_game_rooms[room_num]->SetPlayer(room_num, player_id);
}

std::shared_ptr<GameRoom> GameRoomManager::GetGameRoom(INT room_num)
{
	if (!IsValidRoomNum(room_num))
		return nullptr;
	return m_game_rooms[room_num]->GetGameRoom();
}

void GameRoomManager::InitGameRoom(INT room_num)
{
	m_game_rooms[room_num]->InitGameRoom(room_num);
}

void GameRoomManager::LoadMap()
{
	std::unordered_map<std::string, BoundingOrientedBox> bounding_box_data;

	std::ifstream in{ "./Resource/GameRoom/GameRoomObject.bin", std::ios::binary };

	BYTE strLength{};
	std::string objectName;
	BoundingOrientedBox bounding_box{};

	while (in.read((char*)(&strLength), sizeof(BYTE))) {
		objectName.resize(strLength, '\0');
		in.read((char*)(&objectName[0]), strLength);

		in.read((char*)(&bounding_box.Extents), sizeof(XMFLOAT3));

		bounding_box_data.insert({ objectName, bounding_box });
	}
	in.close();


	in.open("./Resource/GameRoom/GameRoomMap.bin", std::ios::binary);

	XMFLOAT3 position{};
	FLOAT yaw{};

	while (in.read((char*)(&strLength), sizeof(BYTE))) {
		objectName.resize(strLength, '\0');
		in.read(&objectName[0], strLength);

		in.read((char*)(&position), sizeof(XMFLOAT3));
		in.read((char*)(&yaw), sizeof(FLOAT));
		// 라디안 값

		if (!bounding_box_data.contains(objectName))
			continue;

		auto object = std::make_shared<GameObject>();
		object->SetPosition(position);
		object->SetYaw(XMConvertToDegrees(yaw));

		XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, yaw, 0.f);
		XMFLOAT4 q{};
		XMStoreFloat4(&q, vec);

		bounding_box = bounding_box_data[objectName];
		bounding_box.Center = position;
		bounding_box.Orientation = XMFLOAT4{ q.x, q.y, q.z, q.w };

		object->SetBoundingBox(bounding_box);

		if (objectName == "Invisible_Wall")
			m_invisible_walls.push_back(object);
		else
			m_structures.push_back(object);
	}
}

void GameRoomManager::InteractObject(INT room_num, InteractionType type)
{
	if (!IsValidRoomNum(room_num))
		return;

	m_game_rooms[room_num]->InteractObject(type);
}

void GameRoomManager::StartBattle(INT room_num)
{
	if (!IsValidRoomNum(room_num))
		return;

	auto& room = m_game_rooms[room_num];

	std::unique_lock<std::mutex> l{ room->GetStateMutex() };
	if (GameRoomState::INGAME == room->GetState() || 
		GameRoomState::ACCEPT == room->GetState()) 
	{
		room->SetState(GameRoomState::ONBATTLE);
		l.unlock();

		room->StartBattle();
	}
}

void GameRoomManager::WarpNextFloor(INT room_num)
{
	if (!IsValidRoomNum(room_num))
		return;

	auto& room = m_game_rooms[room_num];

	std::unique_lock<std::mutex> l{ room->GetStateMutex() };
	if (GameRoomState::CLEAR == room->GetState()) {
		room->SetState(GameRoomState::INGAME);
		l.unlock();

		room->WarpNextFloor(room_num);
	}
}

void GameRoomManager::DungeonClear(INT room_num)
{
	if (!IsValidRoomNum(room_num))
		return;

	m_game_rooms[room_num]->DungeonClear();
}


bool GameRoomManager::EnterGameRoom(const std::shared_ptr<Party>& party)
{
	INT room_num = FindEmptyRoom();
	if (-1 == room_num)
		return false;
	m_game_rooms[room_num]->InitGameRoom(room_num);

	Server& server = Server::GetInstance();

	auto& members = party->GetMembers();
	for (INT id : members) {
		if (-1 == id) continue;

		auto client = dynamic_pointer_cast<Client>(server.m_clients[id]);
		m_game_rooms[room_num]->SetPlayer(room_num, id);

		{
			std::lock_guard<std::mutex> lock{ client->GetStateMutex() };
			client->SetState(State::INGAME);
		}

		client->SetPartyNum(-1);	// 파티 번호는 -1 로 초기화
	}
	party->Init();	// 입장한 파티는 초기화

	return true;
}

void GameRoomManager::RemovePlayer(INT player_id)
{
	Server& server = Server::GetInstance();
	INT room_num = server.m_clients[player_id]->GetRoomNum();
	if (!IsValidRoomNum(room_num)) {
		return;
	}
	m_game_rooms[room_num]->RemovePlayer(player_id, room_num);
}

void GameRoomManager::RemoveMonster(INT monster_id)
{
	Server& server = Server::GetInstance();
	INT room_num = server.m_clients[monster_id]->GetRoomNum();
	if (!IsValidRoomNum(room_num)) {
		return;
	}
	m_game_rooms[room_num]->RemoveMonster(monster_id);
}

void GameRoomManager::RemoveTrigger(INT trigger_id, INT room_num)
{
	Server& server = Server::GetInstance();
	if (!IsValidRoomNum(room_num)) {
		return;
	}
	m_game_rooms[room_num]->RemoveTrigger(trigger_id);
}

void GameRoomManager::EventCollisionCheck(INT room_num, INT player_id)
{
	m_game_rooms[room_num]->CheckEventCollision(player_id);
}

void GameRoomManager::Update(float elapsed_time)
{
	for (const auto& game_room : m_game_rooms) {
		std::unique_lock<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::ONBATTLE != game_room->GetState()) 
			continue;
		lock.unlock();

		game_room->Update(elapsed_time);
	}
}

void GameRoomManager::SendPlayerData()
{
	for (const auto& game_room : m_game_rooms) {
		std::unique_lock<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState())
			continue;
		lock.unlock();

		game_room->SendPlayerData();
	}
}

void GameRoomManager::SendMonsterData()
{
	for (const auto& game_room : m_game_rooms) {
		std::unique_lock<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::ONBATTLE != game_room->GetState()) 
			continue;
		lock.unlock();

		game_room->SendMonsterData();
	}
}

void GameRoomManager::SendAddMonster(INT room_num, INT player_id)
{
	m_game_rooms[room_num]->SendAddMonster(player_id);
}

void GameRoomManager::SendAddMonster(INT room_num)
{
	m_game_rooms[room_num]->SendAddMonster();
}

INT GameRoomManager::FindEmptyRoom()
{
	for (size_t i = 0; const auto & game_room : m_game_rooms) {
		std::lock_guard<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState()) {
			game_room->SetState(GameRoomState::INGAME);
			return i;
		}

		++i;
	}
	return -1;
}

INT GameRoomManager::FindCanAccessRoom()
{
	for (size_t i = 0; const auto & game_room : m_game_rooms) {
		std::lock_guard<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState() ||
			GameRoomState::ACCEPT == game_room->GetState())
		{
			return i;
		}

		++i;
	}
	return -1;
}

bool GameRoomManager::IsValidRoomNum(INT room_num)
{
	if(-1 == room_num || room_num >= MAX_GAME_ROOM_NUM)
		return false;
	return true;
}
