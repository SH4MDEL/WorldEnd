#include <fstream> 
#include <unordered_map>
#include "map.h"

vector<shared_ptr<GameObject>> Dungeon::m_structures;


DungeonManager::DungeonManager()
{
	auto dungeon = make_shared<Dungeon>();
	m_dungeons.push_back(dungeon);
	dungeon->LoadMap();
}

int DungeonManager::FindDungeonInPlayer(CHAR player_id)
{
	for (size_t i = 0; const auto & dungeon : m_dungeons) {
		if (dungeon->FindPlayer(player_id))
			return i;
		++i;
	}
	return -1;
}

void DungeonManager::SetEvent(CollisionEvent event, CHAR player_id)
{
	int index = FindDungeonInPlayer(player_id);
	m_dungeons[index]->SetEvent(event);
}

void DungeonManager::SetPlayer(int index, CHAR player_id, Session* player)
{
	m_dungeons[index]->SetPlayer(player_id, player);
}

void DungeonManager::InitDungeon(int index)
{
	m_dungeons[index]->InitDungeon();
}

void DungeonManager::Update(float take_time)
{
	for (const auto& dungeon : m_dungeons)
		dungeon->Update(take_time);
}

void DungeonManager::SendMonsterData()
{
	for (const auto& dungeon : m_dungeons)
		dungeon->SendMonsterData();
}

void DungeonManager::SendMonsterAdd(CHAR player_id)
{
	int index = FindDungeonInPlayer(player_id);
	m_dungeons[index]->SendMonsterAdd(player_id);
}


void Dungeon::LoadMap()
{
	unordered_map<string, BoundingOrientedBox> bounding_box_data;

	ifstream in{ "./Resource/DungeonObject.bin", std::ios::binary };

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

	
	in.open("./Resource/DungeonMap.bin", std::ios::binary);

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
		object->SetRotation(yaw);

		XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, yaw, 0.f);
		XMFLOAT4 q{};
		XMStoreFloat4(&q, vec);

		bounding_box = bounding_box_data[objectName];
		bounding_box.Center = position;
		bounding_box.Orientation = XMFLOAT4{ q.x, q.y, q.x, q.w };
		
		object->SetBoundingBox(bounding_box);

		m_structures.push_back(object);
	}

}

void Dungeon::SetEvent(CollisionEvent event)
{
	m_collision_events.push_back(event);
}

void Dungeon::SetPlayer(CHAR player_id, Session* player)
{
	m_players.insert({ player_id, player });
}

void Dungeon::Update(float take_time)
{
	// 몬스터 이동
	for (const auto& monster : m_monsters)
		monster->Update(take_time);


	// 충돌 이벤트에 대한 검사
	for (auto c_it = m_collision_events.begin(); c_it != m_collision_events.end();) {
		for (auto m_it = m_monsters.begin(); m_it != m_monsters.end(); ++m_it) {
			if (-1 == (*m_it)->GetData().id) continue;

			// 충돌 비교
			if ((*m_it)->GetBoundingBox().Intersects(c_it->bounding_box)) {

				// 충돌 발생 시 데미지 계산
				float damage = m_players[c_it->user_id]->m_damage;

				switch (c_it->attack_type) {
				case AttackType::SKILL:
					damage *= m_players[c_it->user_id]->m_skill_ratio;
					break;

				case AttackType::ULTIMATE:
					// 궁극기 계수는 추가 필요
					;
					break;
				}

				(*m_it)->DecreaseHp(damage);

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

void Dungeon::SendMonsterData()
{
	SC_MONSTER_UPDATE_PACKET monster_packet[MAX_MONSTER];

	for (size_t i = 0; i < m_monsters.size(); ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_MONSTER_UPDATE_PACKET));
		monster_packet[i].type = SC_PACKET_UPDATE_MONSTER;
		monster_packet[i].monster_data = m_monsters[i]->GetData();
	}
	for (size_t i = m_monsters.size(); i < MAX_MONSTER; ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_MONSTER_UPDATE_PACKET));
		monster_packet[i].type = SC_PACKET_UPDATE_MONSTER;
		monster_packet[i].monster_data = MonsterData{ .id = -1 };
	}

	char buf[sizeof(monster_packet)];
	memcpy(buf, reinterpret_cast<char*>(&monster_packet), sizeof(monster_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& data : m_players)
	{
		if (!data.second->m_player_data.active_check) continue;
		const int retVal = WSASend(data.second->m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(data.second->m_player_data.id) << " Session] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_UPDATE_MONSTER)");
		}
	}
}

void Dungeon::SendMonsterAdd(CHAR player_id)
{
	SC_ADD_MONSTER_PACKET monster_packet[MAX_MONSTER];

	//auto m_monsters = g_dungeon_manager->GetDungeons()[0]->GetMonsters();

	for (size_t i = 0; i < m_monsters.size(); ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		monster_packet[i].type = SC_PACKET_ADD_MONSTER;
		monster_packet[i].monster_data = m_monsters[i]->GetData();
		monster_packet[i].monster_type = m_monsters[i]->GetType();
	}
	for (size_t i = m_monsters.size(); i < MAX_MONSTER; ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		monster_packet[i].type = SC_PACKET_ADD_MONSTER;
		monster_packet[i].monster_data = MonsterData{ .id = -1 };
	}

	char buf[sizeof(monster_packet)];
	memcpy(buf, reinterpret_cast<char*>(&monster_packet), sizeof(monster_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;


	/*for (const auto& data : m_players)
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

	const int retVal = WSASend(m_players[static_cast<int>(player_id)]->m_socket, &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
	if (retVal == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAECONNRESET)
			std::cout << "[" << static_cast<int>(m_players[static_cast<int>(player_id)]->m_player_data.id) << " Session] Disconnect" << std::endl;
		else ErrorDisplay("Send(SC_PACKET_ADD_MONSTER)");
	}
}

BOOL Dungeon::FindPlayer(CHAR player_id)
{
	for (const auto& data : m_players) {
		if (data.second->m_player_data.id == player_id)
			return true;
	}
	return false;
}

void Dungeon::InitDungeon()
{
	InitMonsters();
	InitEnvironment();
}

void Dungeon::InitMonsters()
{
	// 파일을 읽어서 몬스터를 생성할 예정

	for (size_t i = 0; i < 5; ++i) {
		auto monster = make_shared<WarriorMonster>();
		monster->SetTargetId(0);
		monster->SetId(i);
		monster->SetMonsterPosition();
		m_monsters.push_back(monster);
	}
}

void Dungeon::InitEnvironment()
{
	// 랜덤하게 환경을 설정함
}

