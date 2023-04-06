#include "object.h"
#include "Server.h"

GameObject::GameObject() :m_position{ 0.f, 0.f,0.f }, m_yaw{ 0.f }, m_id{ -1 },
m_bounding_box{ XMFLOAT3{0.f, 0.f, 0.f}, XMFLOAT3{0.f, 0.f, 0.f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f} }
{
}

void GameObject::SetPosition(const XMFLOAT3& pos)
{
	m_position = pos;
	m_bounding_box.Center = pos;
}

void GameObject::SetPosition(FLOAT x, FLOAT y, FLOAT z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
	m_bounding_box.Center = m_position;
}

RecordBoard::RecordBoard()
{
	m_bounding_box.Center = XMFLOAT3(0.f, 0.f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.f, 0.f, 0.f);

	m_yaw = 0.f;
	XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, m_yaw, 0.f);
	XMFLOAT4 q{};
	XMStoreFloat4(&q, vec);
	m_bounding_box.Orientation = XMFLOAT4(q.x, q.y, q.z, q.w);
}

void RecordBoard::SendEvent(INT player_id, void* c)
{
	// 해당 플레이어에게 기록판 정보 보내는 이벤트
}

void RecordBoard::SetRecord(const SCORE_DATA& record, INT player_num)
{
	std::array<SCORE_DATA, MAX_RECORD_NUM>* records{};
	switch (player_num) {
	case 1:
		records = &m_solo_squad_records;
		break;
	case 2:
		records = &m_duo_squad_records;
		break;
	case 3:
		records = &m_trio_squad_records;
		break;
	}

	if (records->back().score < record.score)
		return;

	*(records->end() - 1) = record;
	sort(records->begin(), records->end());

	// DB 처리
}

std::array<SCORE_DATA, MAX_RECORD_NUM>& RecordBoard::GetRecord(INT player_num)
{
	switch (player_num) {
	case 1:
		return m_solo_squad_records;
	case 2:
		return m_duo_squad_records;
	case 3:
		return m_trio_squad_records;
	default:
		throw std::exception{};
	}
}

Enhancment::Enhancment()
{
	m_bounding_box.Center = XMFLOAT3(0.f, 0.f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.f, 0.f, 0.f);

	m_yaw = 0.f;
	XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, m_yaw, 0.f);
	XMFLOAT4 q{};
	XMStoreFloat4(&q, vec);
	m_bounding_box.Orientation = XMFLOAT4(q.x, q.y, q.z, q.w);
}

void Enhancment::SendEvent(INT player_id, void* c)
{
	// 해당 플레이어에게 강화 정보 보내는 이벤트
}

BattleStarter::BattleStarter() : m_is_valid{ true }
{
	m_event_bounding_box.Center = XMFLOAT3(0.f, RoomSetting::EVENT_OBJECT_HEIGHT, 24.f);
	m_event_bounding_box.Extents =
		XMFLOAT3(RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS);
}

void BattleStarter::SetIsValid(bool is_valid)
{
	m_is_valid = is_valid;
}

void BattleStarter::SendEvent(INT player_id, void* c)
{
	SC_START_BATTLE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_START_BATTLE;

	Server& server = Server::GetInstance();
	server.m_clients[player_id]->DoSend(&packet);
}

void BattleStarter::SendEvent(const std::span<INT>& ids, void* c)
{
	std::unique_lock<std::mutex> l{ m_valid_lock };
	if (m_is_valid) {
		m_is_valid = false;
		l.unlock();

		SC_START_BATTLE_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_START_BATTLE;

		Server& server = Server::GetInstance();
		for (INT id : ids) {
			if (-1 == id) continue;
			{
				std::lock_guard<std::mutex> lock{ server.m_clients[id]->GetStateMutex() };
				if (State::ST_INGAME != server.m_clients[id]->GetState()) continue;
			}

			server.m_clients[id]->DoSend(&packet);
		}
	}
}

WarpPortal::WarpPortal() : m_is_valid{ false }
{
	m_event_bounding_box.Center =
		XMFLOAT3(-1.f, RoomSetting::TOPSIDE_STAIRS_HEIGHT + RoomSetting::EVENT_OBJECT_HEIGHT, 60.f);
	m_event_bounding_box.Extents =
		XMFLOAT3(RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS);
}

void WarpPortal::SendEvent(INT player_id, void* c)
{
	BYTE* floor = reinterpret_cast<BYTE*>(c);

	SC_WARP_NEXT_FLOOR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_WARP_NEXT_FLOOR;
	packet.floor = *floor;

	Server& server = Server::GetInstance();
	server.m_clients[player_id]->DoSend(&packet);
}

void WarpPortal::SendEvent(const std::span<INT>& ids, void* c)
{
	BYTE* floor = reinterpret_cast<BYTE*>(c);

	SC_WARP_NEXT_FLOOR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_WARP_NEXT_FLOOR;
	packet.floor = *floor;

	Server& server = Server::GetInstance();
	for (INT id : ids) {
		if (-1 == id) continue;
		{
			std::lock_guard<std::mutex> lock{ server.m_clients[id]->GetStateMutex() };
			if (State::ST_INGAME != server.m_clients[id]->GetState()) continue;
		}

		server.MoveObject(server.m_clients[id], XMFLOAT3(0.f, 0.f, 0.f));
		server.m_clients[id]->DoSend(&packet);
	}
}

MovementObject::MovementObject() : m_velocity{ 0.f, 0.f, 0.f },
m_state{ State::ST_FREE }, m_name{}, m_hp{}, m_damage{},
m_room_num{ static_cast<USHORT>(-1) }
{
}

void MovementObject::SetVelocity(FLOAT x, FLOAT y, FLOAT z)
{
	m_velocity.x = x;
	m_velocity.y = y;
	m_velocity.z = z;
}

void MovementObject::SetName(const char* c)
{
	SetName(std::string(c));
}

