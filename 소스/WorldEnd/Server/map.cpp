#include <fstream> 
#include <unordered_map>
#include "Server.h"
#include "map.h"

void GameRoom::SetEvent(COLLISION_EVENT event)
{
	m_collision_events.push_back(event);
}

// 파티 생성하지 않고 진입 시 사용하는 함수
// 혼자서만 들어가므로 나중에 for문 검사 필요없으니 삭제 필요함
void GameRoom::SetPlayer(INT player_id)
{
	for (int& id : m_player_ids) {
		if (-1 == id) {
			id = player_id;
			break;
		}
	}
}

GameRoom::GameRoom() : m_state{ GameRoomState::EMPTY }, m_floor{ 1 }, 
	m_type{ EnvironmentType::FOG }
{
	for (INT& id : m_player_ids)
		id = -1;

	for (INT& id : m_monster_ids)
		id = -1;
}

void GameRoom::Update(FLOAT elapsed_time)
{
	Server& server = Server::GetInstance();

	// 몬스터 이동
	for (INT id : m_monster_ids) {
		if (-1 == id) continue;

		server.m_clients[id]->Update(elapsed_time);
	}


	// 충돌 이벤트에 대한 검사
	for (auto c_it = m_collision_events.begin(); c_it != m_collision_events.end();) {
		auto& player = server.m_clients[c_it->user_id];

		for (INT id : m_monster_ids) {
			if (-1 == id) continue;

			auto& monster = server.m_clients[id];

			if (-1 == monster->GetId()) continue;

			// 충돌 비교
			if (monster->GetBoundingBox().Intersects(c_it->bounding_box)) {

				// 충돌 발생 시 데미지 계산
				FLOAT damage = player->GetDamage();

				switch (c_it->attack_type) {
				case AttackType::SKILL:
					damage *= player->GetSkillRatio(AttackType::SKILL);
					break;

				case AttackType::ULTIMATE:
					// 궁극기 계수는 추가 필요
					;
					break;
				}

				monster->SetHp(static_cast<FLOAT>(monster->GetHp()) - damage);

				// 1회성 충돌 이벤트이면 이벤트 리스트에서 제거
				// 제거 후 다음 이벤트 검사로 넘어가야 함
				if (CollisionType::ONE_OFF == c_it->collision_type) {
					c_it = m_collision_events.erase(c_it);
					break;
				}
			}
		}
		// 특정 이벤트에 대한 몬스터 검사 끝

		// 해당 이벤트가 끝났으면 리스트에서 제거
		switch (c_it->collision_type) {
		case CollisionType::MULTIPLE_TIMES:
			c_it = m_collision_events.erase(c_it);
			break;

		case CollisionType::PERSISTENCE:
			// 시간을 검사하여 지났다면 이벤트에서 제거
			// 아니면 다음 이벤트 검사
			if (c_it->end_time < chrono::system_clock::now()) {
				c_it = m_collision_events.erase(c_it);
			}
			else {
				++c_it;
			}
			break;
		}

	}
}

void GameRoom::SendMonsterData()
{
	Server& server = Server::GetInstance();

	SC_UPDATE_MONSTER_PACKET packet[MAX_MONSTER]{};

	for (size_t i = 0; INT id : m_monster_ids) {
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

		server.m_clients[id]->DoSend(&packet, MAX_MONSTER);
	}
}

void GameRoom::SendAddMonster(INT player_id)
{
	Server& server = Server::GetInstance();

	SC_ADD_MONSTER_PACKET packet[MAX_MONSTER]{};

	for (size_t i = 0; INT id : m_monster_ids) {
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

	server.m_clients[player_id]->DoSend(&packet, MAX_MONSTER);
}

bool GameRoom::FindPlayer(INT player_id)
{
	for (INT id : m_player_ids) {
		if (id == player_id)
			return true;
	}
	return false;
}

void GameRoom::DeletePlayer(INT player_id)
{
	auto it = find(m_player_ids.begin(), m_player_ids.end(), player_id);
	*it = -1;
	
	INT num = count(m_player_ids.begin(), m_player_ids.end(), -1);
	if (MAX_INGAME_USER == num) {
		lock_guard<mutex> lock{ m_state_lock };
		m_state = GameRoomState::EMPTY;

		for (INT& id : m_player_ids)
			id = -1;

		for (INT& id : m_monster_ids)
			id = -1;
	}
	else {
		Server& server = Server::GetInstance();

		SC_REMOVE_OBJECT_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_REMOVE_OBJECT;
		packet.id = player_id;

		for (INT id : m_player_ids) {
			if (-1 == id) continue;

			server.m_clients[id]->DoSend(&packet);
		}
	}
}

void GameRoom::InitGameRoom()
{
	{
		lock_guard<mutex> lock{ m_state_lock };
		m_state = GameRoomState::ACCEPT; 
	}
	InitMonsters();
	InitEnvironment();
}

void GameRoom::InitMonsters()
{
	Server& server = Server::GetInstance();
	// 파일을 읽어서 몬스터를 생성할 예정
	
	INT new_id{};
	for (size_t i = 0; i < 5; ++i) {
		new_id = server.GetNewMonsterId(MonsterType::WARRIOR);
		
		m_monster_ids[i] = new_id;
		
		auto monster = dynamic_pointer_cast<Monster>(server.m_clients[new_id]);
		monster->SetId(new_id);
		monster->SetTargetId(0);
		monster->InitializePosition();
	}
}

void GameRoom::InitEnvironment()
{
	// 랜덤하게 환경을 설정함
}

GameRoomManager::GameRoomManager()
{
	LoadMap();
	for (auto& game_room : m_game_rooms) {
		game_room = make_shared<GameRoom>();
	}
}

void GameRoomManager::SetEvent(COLLISION_EVENT event, INT player_id)
{
	int index = FindGameRoomInPlayer(player_id);
	m_game_rooms[index]->SetEvent(event);
}

// gameroom 의 Setplayer 와 마찬가지로 파티 생성 없이 진입할 때
// 호출되어야 할 함수
void GameRoomManager::SetPlayer(INT room_num, INT player_id)
{
	Server& server = Server::GetInstance();
	server.m_clients[player_id]->SetRoomNum(room_num);
	{
		lock_guard<mutex> lock{ m_game_rooms[room_num]->GetStateMutex() };
		m_game_rooms[room_num]->SetState(GameRoomState::ACCEPT);
	}
	m_game_rooms[room_num]->SetPlayer(player_id);
}

void GameRoomManager::InitGameRoom(INT room_num)
{
	m_game_rooms[room_num]->InitGameRoom();
}

void GameRoomManager::LoadMap()
{
	unordered_map<string, BoundingOrientedBox> bounding_box_data;

	ifstream in{ "./Resource/GameRoomObject.bin", std::ios::binary };

	BYTE strLength{};
	string objectName;
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

		auto object = make_shared<GameObject>();
		object->SetPosition(position);
		object->SetYaw(yaw);

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

bool GameRoomManager::EnterGameRoom(const shared_ptr<Party>& party)
{
	INT room_num = FindEmptyRoom();
	if (-1 == room_num)
		return false;

	Server& server = Server::GetInstance();

	auto& members = party->GetMembers();
	for (size_t i = 0; i < MAX_INGAME_USER; ++i) {
		if (-1 == members[i]) continue;

		auto client = dynamic_pointer_cast<Client>(server.m_clients[members[i]]);
		m_game_rooms[room_num]->SetPlayer(members[i]);
	}
	
	{
		lock_guard<mutex> lock{ m_game_rooms[room_num]->GetStateMutex() };
		m_game_rooms[room_num]->SetState(GameRoomState::ACCEPT);
	}
	return true;
}

void GameRoomManager::DeletePlayer(INT room_num, INT player_id)
{
	m_game_rooms[room_num]->DeletePlayer(player_id);
}

void GameRoomManager::Update(float elapsed_time)
{
	for (const auto& game_room : m_game_rooms) {
		unique_lock<mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState()) 
			continue;
		lock.unlock();

		game_room->Update(elapsed_time);
	}
}

void GameRoomManager::SendMonsterData()
{
	for (const auto& game_room : m_game_rooms) {
		unique_lock<mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState()) 
			continue;
		lock.unlock();

		game_room->SendMonsterData();
	}
}

void GameRoomManager::SendAddMonster(INT player_id)
{
	int room_num = FindGameRoomInPlayer(player_id);
	m_game_rooms[room_num]->SendAddMonster(player_id);
}

INT GameRoomManager::FindEmptyRoom()
{
	for (size_t i = 0; const auto & game_room : m_game_rooms) {
		unique_lock<mutex> lock{ game_room->GetStateMutex() };
		if (GameRoomState::EMPTY == game_room->GetState())
			return i;
		lock.unlock();

		++i;
	}
	return -1;
}

INT GameRoomManager::FindGameRoomInPlayer(INT player_id)
{
	for (size_t i = 0; const auto & game_room : m_game_rooms) {
		if (game_room->FindPlayer(player_id))
			return i;
		++i;
	}
	return -1;
}
