#include <bitset>
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

BattleStarter::BattleStarter()
{
	m_event_bounding_box.Center = RoomSetting::BATTLE_STARTER_POSITION;
	m_event_bounding_box.Extents =
		XMFLOAT3(RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS);
	m_valid = true;
}

void BattleStarter::SendEvent(INT player_id, void* c)
{
	InteractionType* type = reinterpret_cast<InteractionType*>(c);

	SC_INTERACT_OBJECT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_INTERACT_OBJECT;
	packet.interaction_type = *type;

	Server& server = Server::GetInstance();
	server.m_clients[player_id]->DoSend(&packet);
}

void BattleStarter::SendEvent(const std::span<INT>& ids, void* c)
{
	std::unique_lock<std::mutex> l{ m_valid_lock };
	if (m_valid) {
		m_valid = false;
		l.unlock();

		InteractionType* type = reinterpret_cast<InteractionType*>(c);

		SC_INTERACT_OBJECT_PACKET packet{};
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_INTERACT_OBJECT;
		packet.interaction_type = *type;

		Server& server = Server::GetInstance();
		for (INT id : ids) {
			if (-1 == id) continue;
			{
				std::lock_guard<std::mutex> lock{ server.m_clients[id]->GetStateMutex() };
				if (State::INGAME != server.m_clients[id]->GetState()) continue;
			}

			server.m_clients[id]->DoSend(&packet);
		}
	}
}

WarpPortal::WarpPortal()
{
	m_event_bounding_box.Center = RoomSetting::WARP_PORTAL_POSITION;
	m_event_bounding_box.Extents =
		XMFLOAT3(RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS, RoomSetting::EVENT_RADIUS);
	m_valid = false;
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

	m_valid = false;

	SC_WARP_NEXT_FLOOR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_WARP_NEXT_FLOOR;
	packet.floor = *floor;

	Server& server = Server::GetInstance();
	for (INT id : ids) {
		if (-1 == id) continue;
		{
			std::lock_guard<std::mutex> lock{ server.m_clients[id]->GetStateMutex() };
			if (State::INGAME != server.m_clients[id]->GetState()) continue;
		}

		server.MoveObject(server.m_clients[id], RoomSetting::START_POSITION);
		server.m_clients[id]->DoSend(&packet);
	}
}

MovementObject::MovementObject() : m_velocity{ 0.f, 0.f, 0.f },
	m_state{ State::FREE }, m_name{}, m_hp{}, m_damage{},
	m_room_num{ -1 }, m_trigger_flag{ 0 }
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

void MovementObject::SetTriggerFlag(UCHAR trigger, bool val)
{
	if(val)
		m_trigger_flag |= (1 << static_cast<INT>(trigger));
	else
		m_trigger_flag &= ~(1 << static_cast<INT>(trigger));
}

XMFLOAT3 MovementObject::GetFront() const
{
	XMFLOAT3 front{ 0.f, 0.f, 1.f };
	XMMATRIX rotate{ XMMatrixRotationRollPitchYaw(0.f, XMConvertToRadians(m_yaw), 0.f) };
	XMStoreFloat3(&front, XMVector3TransformNormal(XMLoadFloat3(&front), rotate));
	return front;
}

bool MovementObject::CheckTriggerFlag(UCHAR trigger)
{
	if (m_trigger_flag & (1 << static_cast<INT>(trigger)))
		return false;
	return true;
}

Trigger::Trigger()
{
	m_state = State::FREE;
}

void Trigger::SetCooldownEvent(INT id)
{
	Server& server = Server::GetInstance();

	// 해당 오브젝트의 트리거 타이머 이벤트 설정
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + m_cooldown,
		.obj_id = m_id, .target_id = id, .event_type = EventType::TRIGGER_COOLDOWN,
		.trigger_type = m_type };

	server.SetTimerEvent(ev);
}

void Trigger::SetRemoveEvent(INT id)
{
	Server& server = Server::GetInstance();

	INT room_num = server.m_clients[id]->GetRoomNum();

	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + m_duration,
		.obj_id = m_id, .target_id = room_num, .event_type = EventType::TRIGGER_REMOVE };

	server.SetTimerEvent(ev);
}

void Trigger::Activate(INT id)
{
	// 트리거의 공통 처리
	//  - 트리거 플래그 확인
	//  - 트리거 플래그 활성화
	//  - 타이머 이벤트 생성
	
	// 트리거 처리, 유효성검사만 트리거 별로 따로 처리되도록 하면 됨

	/*std::string s = (m_type == ARROW_RAIN) ? "ARROW RAIN" : "UNDEAD GRASP";
	printf("트리거 활성화, id : %d, %s\n", id, s.c_str());*/

	Server& server = Server::GetInstance();
	UCHAR trigger_type = static_cast<UCHAR>(m_type);
	if (IsValid(id)) {
		if (server.m_clients[id]->CheckTriggerFlag(trigger_type)) {
			// 플래그 활성화

			server.m_clients[id]->SetTriggerFlag(trigger_type, true);

			ProcessTrigger(id);

			// 타이머 이벤트 생성, 연속발생 쿨타임이 0이면 이벤트 생성 X
			if (m_cooldown != std::chrono::milliseconds(0)) {
				SetCooldownEvent(id);
			}
		}
	}
	
}

void Trigger::Create(FLOAT damage, INT id)
{
	Server& server = Server::GetInstance();
	m_event_bounding_box.Center = server.m_clients[id]->GetPosition();

	m_damage = damage;
	m_created_id = id;
	m_state = State::INGAME;
	SetRemoveEvent(id);
}

void Trigger::Create(FLOAT damage, INT id, const XMFLOAT3& position)
{
	Server& server = Server::GetInstance();
	m_event_bounding_box.Center = position;

	m_damage = damage;
	m_created_id = id;
	m_state = State::INGAME;
	SetRemoveEvent(id);
}

ArrowRain::ArrowRain()
{
	INT type = static_cast<INT>(TriggerType::ARROW_RAIN);
	m_event_bounding_box.Extents = TriggerSetting::EXTENT[type];
	m_cooldown = TriggerSetting::COOLDOWN[type];
	m_duration = TriggerSetting::DURATION[type];

	m_damage = 0.f;
	m_created_id = -1;
	m_type = TriggerType::ARROW_RAIN;
}

void ArrowRain::ProcessTrigger(INT id)
{
	Server& server = Server::GetInstance();
	server.m_clients[id]->DecreaseHp(m_damage, -1);
}

bool ArrowRain::IsValid(INT id)
{
	if (MAX_USER <= id && id < MAX_OBJECT)
		return true;
	return false;
}

UndeadGrasp::UndeadGrasp()
{
	INT type = static_cast<INT>(TriggerType::UNDEAD_GRASP);
	m_event_bounding_box.Extents = TriggerSetting::EXTENT[type];
	m_cooldown = TriggerSetting::COOLDOWN[type];
	m_duration = TriggerSetting::DURATION[type];

	m_damage = 0.f;
	m_created_id = -1;
	m_type = TriggerType::UNDEAD_GRASP;
}

void UndeadGrasp::ProcessTrigger(INT id)
{
	Server& server = Server::GetInstance();
	server.m_clients[id]->DecreaseHp(m_damage, m_created_id);

	if (State::DEATH != server.m_clients[id]->GetState()) {
		server.SendChangeHp(id, server.m_clients[id]->GetHp());
	}
}

bool UndeadGrasp::IsValid(INT id)
{
	if (0 <= id && id < MAX_USER)
		return true;
	return false;
}
