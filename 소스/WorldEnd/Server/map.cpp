#include <fstream> 
#include <unordered_map>
#include "map.h"

void GameRoom::SetEvent(COLLISION_EVENT event)
{
	m_collision_events.push_back(event);
}

void GameRoom::SetPlayer(INT player_id, Client* player)
{
	m_players.insert({ player_id, player });
}

GameRoom::GameRoom() : m_state{ GameRoomState::EMPTY }, m_floor{ 1 }, 
	m_type{ EnvironmentType::FOG }
{
}

void GameRoom::Update(FLOAT elapsed_time)
{
	// ���� �̵�
	for (const auto& monster : m_monsters)
		monster->Update(elapsed_time);


	// �浹 �̺�Ʈ�� ���� �˻�
	for (auto c_it = m_collision_events.begin(); c_it != m_collision_events.end();) {
		for (auto m_it = m_monsters.begin(); m_it != m_monsters.end(); ++m_it) {
			if (-1 == (*m_it)->GetData().id) continue;

			// �浹 ��
			if ((*m_it)->GetBoundingBox().Intersects(c_it->bounding_box)) {

				// �浹 �߻� �� ������ ���
				FLOAT damage = m_players[c_it->user_id]->GetDamage();

				switch (c_it->attack_type) {
				case AttackType::SKILL:
					damage *= m_players[c_it->user_id]->GetSkillRatio(AttackType::SKILL);
					break;

				case AttackType::ULTIMATE:
					// �ñر� ����� �߰� �ʿ�
					;
					break;
				}

				(*m_it)->DecreaseHp(damage);

				// 1ȸ�� �浹 �̺�Ʈ�̸� �̺�Ʈ ����Ʈ���� ����
				// ���� �� ���� �̺�Ʈ �˻�� �Ѿ�� ��
				if (CollisionType::ONE_OFF == c_it->collision_type) {
					c_it = m_collision_events.erase(c_it);
					break;
				}
			}
		}
		// Ư�� �̺�Ʈ�� ���� ���� �˻� ��

		// �ش� �̺�Ʈ�� �������� ����Ʈ���� ����
		switch (c_it->collision_type) {
		case CollisionType::MULTIPLE_TIMES:
			c_it = m_collision_events.erase(c_it);
			break;

		case CollisionType::PERSISTENCE:
			// �ð��� �˻��Ͽ� �����ٸ� �̺�Ʈ���� ����
			// �ƴϸ� ���� �̺�Ʈ �˻�
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
	SC_MONSTER_UPDATE_PACKET monster_packet[MAX_MONSTER];

	for (size_t i = 0; i < m_monsters.size(); ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_MONSTER_UPDATE_PACKET));
		monster_packet[i].type = SC_PACKET_UPDATE_MONSTER;
		monster_packet[i].monster_data = m_monsters[i]->GetData();
	}
	for (size_t i = m_monsters.size(); i < MAX_MONSTER; ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_MONSTER_UPDATE_PACKET));
		monster_packet[i].type = SC_PACKET_UPDATE_MONSTER;
		monster_packet[i].monster_data = MONSTER_DATA{ .id = -1 };
	}

	char buf[sizeof(monster_packet)];
	memcpy(buf, reinterpret_cast<char*>(&monster_packet), sizeof(monster_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& data : m_players)
	{
		if (-1 == data.second->GetId()) continue;
		const int retVal = WSASend(data.second->GetSocket(), &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(data.second->GetId()) << " Client] Disconnect" << std::endl;
			else ErrorDisplay("Send(SC_PACKET_UPDATE_MONSTER)");
		}
	}
}

void GameRoom::SendAddMonster(INT player_id)
{
	SC_ADD_MONSTER_PACKET monster_packet[MAX_MONSTER];

	for (size_t i = 0; i < m_monsters.size(); ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		monster_packet[i].type = SC_PACKET_ADD_MONSTER;
		monster_packet[i].monster_data = m_monsters[i]->GetData();
		monster_packet[i].monster_type = m_monsters[i]->GetType();
	}
	for (size_t i = m_monsters.size(); i < MAX_MONSTER; ++i) {
		monster_packet[i].size = static_cast<UCHAR>(sizeof(SC_ADD_MONSTER_PACKET));
		monster_packet[i].type = SC_PACKET_ADD_MONSTER;
		monster_packet[i].monster_data = MONSTER_DATA{ .id = -1 };
	}

	char buf[sizeof(monster_packet)];
	memcpy(buf, reinterpret_cast<char*>(&monster_packet), sizeof(monster_packet));
	WSABUF wsa_buf{ sizeof(buf), buf };
	DWORD sent_byte;

	const int retVal = WSASend(m_players[static_cast<int>(player_id)]->GetSocket(), &wsa_buf, 1, &sent_byte, 0, nullptr, nullptr);
	if (retVal == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAECONNRESET)
			std::cout << "[" << static_cast<int>(m_players[static_cast<int>(player_id)]->GetId()) << " Client] Disconnect" << std::endl;
		else ErrorDisplay("Send(SC_PACKET_ADD_MONSTER)");
	}
}

bool GameRoom::FindPlayer(INT player_id)
{
	for (const auto& data : m_players) {
		if (data.second->GetId() == player_id)
			return true;
	}
	return false;
}

void GameRoom::InitGameRoom()
{
	m_state = GameRoomState::ACCEPT;
	InitMonsters();
	InitEnvironment();
}

void GameRoom::InitMonsters()
{
	// ������ �о ���͸� ������ ����

	for (size_t i = 0; i < 5; ++i) {
		auto monster = make_shared<WarriorMonster>();
		monster->SetTargetId(0);
		monster->SetId(i);
		monster->InitializePosition();
		m_monsters.push_back(monster);
	}
}

void GameRoom::InitEnvironment()
{
	// �����ϰ� ȯ���� ������
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

void GameRoomManager::SetPlayer(INT index, INT player_id, Client* player)
{
	m_game_rooms[index]->SetPlayer(player_id, player);
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
		// ���� ��

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

	auto& members = party->GetMembers();
	for (size_t i = 0; i < MAX_IN_GAME_USER; ++i) {
		if (-1 == members[i]) continue;

		int& client_id = members[i];
		m_game_rooms[room_num]->SetPlayer(client_id, &g_server.m_clients[client_id]);
	}
	m_game_rooms[room_num]->SetState(GameRoomState::ACCEPT);

	return true;
}

void GameRoomManager::Update(float elapsed_time)
{
	for (const auto& game_room : m_game_rooms) {
		if (GameRoomState::EMPTY == game_room->GetState()) continue;
		
		game_room->Update(elapsed_time);
	}
}

void GameRoomManager::SendMonsterData()
{
	for (const auto& game_room : m_game_rooms) {
		if (GameRoomState::EMPTY == game_room->GetState()) continue;

		game_room->SendMonsterData();
	}
}

void GameRoomManager::SendAddMonster(INT player_id)
{
	int index = FindGameRoomInPlayer(player_id);
	m_game_rooms[index]->SendAddMonster(player_id);
}

INT GameRoomManager::FindEmptyRoom()
{
	for (size_t i = 0; const auto & game_room : m_game_rooms) {
		if (GameRoomState::EMPTY == game_room->GetState())
			return i;
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
