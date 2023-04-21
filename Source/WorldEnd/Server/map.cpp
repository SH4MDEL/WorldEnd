#include <fstream> 
#include <unordered_map>
#include "Server.h"
#include "map.h"

GameRoom::GameRoom() : m_state{ GameRoomState::EMPTY }, m_floor{ 1 },
m_type{ EnvironmentType::FOG }, m_monster_count{ 0 }, m_arrow_id{ 0 }
{
	for (INT& id : m_player_ids)
		id = -1;

	for (INT& id : m_monster_ids)
		id = -1;

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

void GameRoom::StartBattle()
{
	Server& server = Server::GetInstance();

	m_battle_starter->SendEvent(m_player_ids, &m_monster_ids);

	for (INT id : m_monster_ids) {
		if (-1 == id) continue;

		auto monster = dynamic_pointer_cast<Monster>(server.m_clients[id]);

		monster->SetState(State::INGAME);
		monster->ChangeBehavior(MonsterBehavior::CHASE);
	}
}

void GameRoom::WarpNextFloor(INT room_num)
{
	Server& server = Server::GetInstance();

	m_portal->SendEvent(m_player_ids, &m_floor);

	++m_floor;
	if (RoomSetting::BOSS_FLOOR == m_floor) {
		// 보스 룸 진입
	}
	InitGameRoom(room_num);

	// 방 넘어갈 때 생성한 몬스터 정보 다시 넘기기
	for (INT id : m_player_ids) {
		if (-1 == id) continue;

		SendAddMonster(id);
	}
}

// 파티 생성하지 않고 진입 시 사용하는 함수
// 혼자서만 들어가므로 나중에 for문 검사 필요없으니 삭제 필요함
void GameRoom::SetPlayer(INT player_id)
{
	for (int& id : m_player_ids) {
		if (-1 == id) {
			id = player_id;
			if (GameRoomState::INGAME == m_state) {
				m_battle_starter->SendEvent(player_id, nullptr);
			}
			break;
		}
	}
}

std::shared_ptr<BattleStarter> GameRoom::GetBattleStarter() const
{
	return m_battle_starter;
}

INT GameRoom::GetArrowId()
{
	std::lock_guard<std::mutex> lock{ m_arrow_lock };
	int id = m_arrow_id;
	m_arrow_id = (m_arrow_id + 1) % RoomSetting::MAX_ARROWS;

	return id;
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

	for (INT id : m_player_ids){
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
			packet[i].monster_type = server.m_clients[id]->GetMonsterType();
		}
		++i;
	}
	

	server.m_clients[player_id]->DoSend(&packet, monster_count);
	
	if (GameRoomState::INGAME == m_state) {
		m_battle_starter->SendEvent(player_id, nullptr);
	}
}

void GameRoom::RemovePlayer(INT player_id, INT room_num)
{
	if (player_id < 0 || player_id >= MAX_USER)
		return;

	for (INT& id : m_player_ids) {
		if (id == player_id) {
			std::lock_guard<std::mutex> lock{ m_player_lock };
			id = -1;
		}
	}

	// 방이 아예 비었다면
	INT num = count(m_player_ids.begin(), m_player_ids.end(), -1);
	if (MAX_INGAME_USER == num) {
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

		for (INT id : m_player_ids) {
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
		for (size_t i = 0; INT & id : m_monster_ids) {
			if (id == monster_id) {
				id = -1;
				monster_count = --m_monster_count;
				index = i;
			}
			++i;
		}
	}

	// 마지막 몬스터가 아니라면 삭제한 위치에 마지막 몬스터 id 끌고오기
	if (index != monster_count) {
		m_monster_ids[index] = m_monster_ids[monster_count];
		m_monster_ids[monster_count] = -1;
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
	std::lock_guard<std::mutex> lock{ m_trigger_lock };
	m_trigger_list.erase(trigger_id);
}

void GameRoom::CheckEventCollision(INT player_id)
{
	// 전투 오브젝트와 충돌검사
	if (GameRoomState::ACCEPT == m_state) {
		CollideWithEventObject(player_id, InteractableType::BATTLE_STARTER);
	}
	// 포탈과 충돌검사
	else if (GameRoomState::CLEAR == m_state) {
		CollideWithEventObject(player_id, InteractableType::PORTAL);
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
		obb = server.m_triggers[trigger_id]->GetEventBoundingBox();

		// 충돌했을 때 트리거 플래그가 0이면 Activate
		if (obb.Intersects(server.m_clients[id]->GetBoundingBox())) {
			server.m_triggers[trigger_id]->Activate(id);
		}
	}
}

void GameRoom::InitGameRoom(INT room_num)
{
	InitMonsters(room_num);
	InitEnvironment();
	m_battle_starter->SetIsValid(true);

	// 게임 시작 시 시작 시간 기록
	m_start_time = std::chrono::system_clock::now();
}

void GameRoom::InitMonsters(INT room_num)
{
	Server& server = Server::GetInstance();
	// 파일을 읽어서 몬스터를 생성할 예정
	
	// Init 은 플레이어 진입 시 불려야함
	INT new_id{};
	for (size_t i = 0; i < 2; ++i) {
		new_id = server.GetNewMonsterId(MonsterType::ARCHER);
		
		m_monster_ids[i] = new_id;
		
		auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_id]);
		monster->Init();

		monster->SetId(new_id);
		monster->InitializePosition();
		monster->SetRoomNum(room_num);
		monster->SetTarget(0);
		monster->SetState(State::ACCEPT);
		++m_monster_count;
	}
}

void GameRoom::InitEnvironment()
{
	// 랜덤하게 환경을 설정함
}

void GameRoom::CollideWithEventObject(INT player_id, InteractableType type)
{
	std::shared_ptr<EventObject> object{};
	switch(type) {
	case InteractableType::BATTLE_STARTER:
		object = m_battle_starter;
		break;
	case InteractableType::PORTAL:
		object = m_portal;
		break;
	}

	Server& server = Server::GetInstance();
	auto client = dynamic_pointer_cast<Client>(server.m_clients[player_id]);

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

GameRoomManager::GameRoomManager()
{
	LoadMap();
	for (auto& game_room : m_game_rooms) {
		game_room = std::make_shared<GameRoom>();
	}
}

// gameroom 의 Setplayer 와 마찬가지로 파티 생성 없이 진입할 때
// 호출되어야 할 함수
void GameRoomManager::SetPlayer(INT room_num, INT player_id)
{
	{
		std::unique_lock<std::mutex> lock{ m_game_rooms[room_num]->GetStateMutex() };
		if (GameRoomState::EMPTY == m_game_rooms[room_num]->GetState()) {
			m_game_rooms[room_num]->SetState(GameRoomState::ACCEPT);
			lock.unlock();
			m_game_rooms[room_num]->InitGameRoom(room_num);
		}
	}

	Server& server = Server::GetInstance();
	server.m_clients[player_id]->SetRoomNum(room_num);
	
	m_game_rooms[room_num]->SetPlayer(player_id);
}

std::shared_ptr<GameRoom> GameRoomManager::GetGameRoom(INT room_num)
{
	if (!IsValidRoomNum(room_num))
		return nullptr;
	return m_game_rooms[room_num]->GetGameRoom();
}

//std::chrono::system_clock::time_point GameRoomManager::GetStartTime(INT room_num)
//{
//	if (!IsValidRoomNum(room_num)) {
//		return std::chrono::system_clock::now();
//	}
//	return m_game_rooms[room_num]->GetStartTime();
//}
//
//EnvironmentType GameRoomManager::GetEnvironment(INT room_num)
//{
//	if (!IsValidRoomNum(room_num)) {
//		return EnvironmentType::COUNT;
//	}
//	return m_game_rooms[room_num]->GetType();
//}
//
//GameRoomState GameRoomManager::GetRoomState(INT room_num)
//{
//	if (!IsValidRoomNum(room_num)) {
//		return GameRoomState::COUNT;
//	}
//	return m_game_rooms[room_num]->GetState();
//}
//
//INT GameRoomManager::GetArrowId(INT room_num)
//{
//	if (!IsValidRoomNum(room_num)) {
//		return -1;
//	}
//	return m_game_rooms[room_num]->GetArrowId();
//}

void GameRoomManager::InitGameRoom(INT room_num)
{
	m_game_rooms[room_num]->InitGameRoom(room_num);
}

void GameRoomManager::LoadMap()
{
	std::unordered_map<std::string, BoundingOrientedBox> bounding_box_data;

	std::ifstream in{ "./Resource/GameRoomObject.bin", std::ios::binary };

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


	in.open("./Resource/GameRoomMap.bin", std::ios::binary);

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

		m_structures.push_back(object);
	}
}

void GameRoomManager::StartBattle(INT room_num)
{
	auto& room = m_game_rooms[room_num];

	std::unique_lock<std::mutex> l{ room->GetStateMutex() };
	if (GameRoomState::ACCEPT == room->GetState()) {
		room->SetState(GameRoomState::INGAME);
		l.unlock();

		m_game_rooms[room_num]->StartBattle();
	}
}

void GameRoomManager::WarpNextFloor(INT room_num)
{
	auto& room = m_game_rooms[room_num];

	std::unique_lock<std::mutex> l{ room->GetStateMutex() };
	if (GameRoomState::CLEAR == room->GetState()) {
		room->SetState(GameRoomState::ACCEPT);
		l.unlock();

		m_game_rooms[room_num]->WarpNextFloor(room_num);
	}
}

bool GameRoomManager::EnterGameRoom(const std::shared_ptr<Party>& party)
{
	INT room_num = FindEmptyRoom();
	if (-1 == room_num)
		return false;
	m_game_rooms[room_num]->InitGameRoom(room_num);

	Server& server = Server::GetInstance();

	auto& members = party->GetMembers();
	for (size_t i = 0; i < MAX_INGAME_USER; ++i) {
		if (-1 == members[i]) continue;

		auto client = dynamic_pointer_cast<Client>(server.m_clients[members[i]]);
		client->SetRoomNum(room_num);
		m_game_rooms[room_num]->SetPlayer(members[i]);
	}
	
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
		if (GameRoomState::INGAME != game_room->GetState()) 
			continue;
		lock.unlock();

		game_room->Update(elapsed_time);
	}
}

void GameRoomManager::SendMonsterData()
{
	for (const auto& game_room : m_game_rooms) {
		std::unique_lock<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::INGAME != game_room->GetState()) 
			continue;
		lock.unlock();

		game_room->SendMonsterData();
	}
}

void GameRoomManager::SendAddMonster(INT room_num, INT player_id)
{
	m_game_rooms[room_num]->SendAddMonster(player_id);
}

INT GameRoomManager::FindEmptyRoom()
{
	for (size_t i = 0; const auto & game_room : m_game_rooms) {
		std::unique_lock<std::mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState()) {
			game_room->SetState(GameRoomState::ACCEPT);
			return i;
		}
		lock.unlock();

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
