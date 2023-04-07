#include <functional>
#include <random>
#include "monster.h"
#include "stdafx.h"
#include "Server.h"

std::random_device rd;
std::default_random_engine dre(rd());
std::uniform_int_distribution<int> random_behavior(1, 100);
std::uniform_int_distribution<int> random_retarget_time(5, 10);

Monster::Monster() : m_target_id{ -1 }, m_current_animation{ ObjectAnimation::IDLE },
	m_current_behavior{ MonsterBehavior::NONE }, m_aggro_level{ 0 },
	m_last_behavior_id{ 0 }
{
}

void Monster::Init()
{
	m_target_id = -1;
	m_current_animation = ObjectAnimation::IDLE;
	m_current_behavior = MonsterBehavior::NONE;
	m_aggro_level = 0;
	m_last_behavior_id = 0;
}

void Monster::UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time)
{
	m_velocity = Vector3::Mul(dir, MonsterSetting::WALK_SPEED);

	// 몬스터 이동
	m_position = Vector3::Add(m_position, Vector3::Mul(m_velocity, elapsed_time));

	m_bounding_box.Center = m_position;
}

void Monster::UpdateRotation(const XMFLOAT3& dir)
{
	XMVECTOR z_axis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
	XMVECTOR radian{ XMVector3AngleBetweenNormals(z_axis, XMLoadFloat3(&dir)) };
	FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };

	XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, XMVectorGetX(radian), 0.f);
	XMFLOAT4 q{};
	XMStoreFloat4(&q, vec);
	m_bounding_box.Orientation = XMFLOAT4{ q.x, q.y, q.z, q.w };

	// degree - 클라이언트에서 degree 로 Rotation 을 처리함
	m_yaw = dir.x < 0 ? -angle : angle;
}


XMFLOAT3 Monster::GetPlayerDirection(INT player_id)
{
	if (-1 == player_id)
		return XMFLOAT3(0.f, 0.f, 0.f);

	Server& server = Server::GetInstance();

	// 타겟 플레이어가 사망 or 접속 종료되면 타겟 변경
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();
	auto it = find(ids.begin(), ids.end(), player_id);
	if (it == ids.end()) {
		UpdateTarget();
		return XMFLOAT3(0.f, 0.f, 0.f);
	}

	if (State::ST_INGAME != server.m_clients[player_id]->GetState()) {
		UpdateTarget();
		return XMFLOAT3(0.f, 0.f, 0.f);
	}

	XMFLOAT3 sub = Vector3::Sub(server.m_clients[player_id]->GetPosition(), m_position);
	return Vector3::Normalize(sub);
}

bool Monster::CanAttack()
{
	Server& server = Server::GetInstance();
	if (-1 == server.m_clients[m_target_id]->GetId())
		return false;

	FLOAT dist = Vector3::Distance(server.m_clients[m_target_id]->GetPosition(), m_position);

	if (dist <= m_range)
		return true;

	return false;
}

void Monster::MakeDecreaseAggroLevelEvent()
{
	TIMER_EVENT ev{ .event_time = std::chrono::system_clock::now() + 
			MonsterSetting::DECREASE_AGRO_LEVEL_TIME, .obj_id = m_id,
			.event_type = EventType::AGRO_LEVEL_DECREASE,
		.aggro_level = m_aggro_level };

	Server& server = Server::GetInstance();
	server.SetTimerEvent(ev);
}

void Monster::SetTarget(INT id)
{
	m_target_id = id;
}

void Monster::SetAggroLevel(BYTE aggro_level)
{
	// 변경하려는 어그로 레벨이 더 클때만 변경하고 타이머 이벤트 생성
	if (m_aggro_level < aggro_level) {
		m_aggro_level = aggro_level;
		
		MakeDecreaseAggroLevelEvent();
	}
}

MONSTER_DATA Monster::GetMonsterData() const
{
	return MONSTER_DATA( m_id, m_position, m_velocity, m_yaw, m_hp );
}

XMFLOAT3 Monster::GetFront() const
{
	XMFLOAT3 front{ 0.f, 0.f, 1.f };
	XMMATRIX rotate{ XMMatrixRotationRollPitchYaw(0.f, XMConvertToRadians(m_yaw), 0.f) };
	XMStoreFloat3(&front, XMVector3TransformNormal(XMLoadFloat3(&front), rotate));
	return front;
}

bool Monster::ChangeAnimation(BYTE animation)
{
	if (m_current_animation == animation)
		return false;

	m_current_animation = animation;
	return true;
}

void Monster::ChangeBehavior(MonsterBehavior behavior)
{
	m_current_behavior = behavior;

	TIMER_EVENT ev{};
	ev.obj_id = m_id;
	ev.event_type = EventType::BEHAVIOR_CHANGE;
	ev.aggro_level = m_aggro_level;
	auto current_time = std::chrono::system_clock::now();

	if (std::numeric_limits<BYTE>::max() == m_last_behavior_id)
		m_last_behavior_id = 0;
	ev.latest_id = ++m_last_behavior_id;

	switch (m_current_behavior) {
	case MonsterBehavior::CHASE: {
		m_current_animation = ObjectAnimation::RUN;
		int time = random_retarget_time(dre);
		ev.event_time = current_time + static_cast<std::chrono::seconds>(time);

		int percent = random_behavior(dre);
		if (0 < percent && percent <= 50) {
			ev.next_behavior_type = MonsterBehavior::RETARGET;
		}
		else {
			ev.next_behavior_type = MonsterBehavior::TAUNT;
		}
		break;
	}
	case MonsterBehavior::RETARGET:
		m_current_animation = MonsterAnimation::LOOK_AROUND;
		ev.event_time = current_time + MonsterSetting::LOOK_AROUND_TIME;
		ev.next_behavior_type = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::TAUNT:
		m_current_animation = MonsterAnimation::TAUNT;
		ev.event_time = current_time + MonsterSetting::TAUNT_TIME;
		ev.next_behavior_type = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		// 공격 준비 시간 이후 공격으로 변경
		m_current_animation = MonsterAnimation::TAUNT;
		ev.event_time = current_time + MonsterSetting::PREPARE_ATTACK_TIME;
		ev.next_behavior_type = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		// 공격 가능하면 공격, 공격 이후 공격 준비 재전환 
		// 불가능하면 바로 추격
		if (CanAttack()) {
			m_current_animation = ObjectAnimation::ATTACK;
			ev.event_time = current_time + MonsterSetting::ATTACK_TIME;
			ev.next_behavior_type = MonsterBehavior::PREPARE_ATTACK;
		}
		else {
			m_current_animation = ObjectAnimation::WALK;
			ev.event_time = current_time;
			ev.next_behavior_type = MonsterBehavior::CHASE;
		}

		break;
	case MonsterBehavior::DEAD:
		m_current_animation = ObjectAnimation::DEAD;
		ev.event_time = current_time + MonsterSetting::DEAD_TIME;
		ev.next_behavior_type = MonsterBehavior::DEAD;
		break;
	default:
		return;
	}

	Server& server = Server::GetInstance();
	server.SetTimerEvent(ev);

	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	SC_CHANGE_MONSTER_BEHAVIOR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_MONSTER_BEHAVIOR;
	packet.behavior = m_current_behavior;
	packet.animation = m_current_animation;
	packet.id = m_id;

	for (INT id : ids) {
		if (-1 == id) continue;

		server.m_clients[id]->DoSend(&packet);
	}
}

void Monster::DoBehavior(FLOAT elapsed_time)
{
	switch (m_current_behavior) {
	case MonsterBehavior::CHASE:
		ChasePlayer(elapsed_time);
		break;
	case MonsterBehavior::RETARGET:
		Retarget();
		break;
	case MonsterBehavior::TAUNT:
		Taunt();
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		PrepareAttack();
		break;
	case MonsterBehavior::ATTACK:
		Attack();
		break;
	default:
		return;
	}
	
	// 행동 후 PREPARE_ATTACK, ATTACK 이 아니면 공격 가능한지 체크
	if (!IsDoAttack()){
		if (CanAttack()) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
		}
	}
}

bool Monster::IsDoAttack()
{
	if ((MonsterBehavior::PREPARE_ATTACK == m_current_behavior)||
		(MonsterBehavior::ATTACK == m_current_behavior))
	{
		return true;
	}
	return false;
}

void Monster::DecreaseHp(FLOAT damage, INT id)
{
	if (m_hp <= 0)
		return;

	m_hp -= damage;
	if (m_hp <= 0) {
		{
			std::lock_guard<std::mutex> l{ m_state_lock };
			m_state = State::ST_DEAD;
		}
		// 죽은것 전송
		ChangeBehavior(MonsterBehavior::DEAD);
		return;
	}

	if (AggroLevel::HIT_AGGRO < m_aggro_level)
		return;

	// 일정 비율 이상 데미지가 들어오면 타겟 변경
	if ((m_max_hp / 11.f - damage) <= std::numeric_limits<FLOAT>::epsilon()) {
		SetTarget(id);
		SetAggroLevel(AggroLevel::HIT_AGGRO);
	}
}

void Monster::DecreaseAggroLevel()
{
	m_aggro_level -= 1;
	if (m_aggro_level <= 0) {
		m_aggro_level = 0;
	}
	else {
		MakeDecreaseAggroLevelEvent();
	}
	
}

void Monster::UpdateTarget()
{
	if (AggroLevel::NORMAL_AGGRO < m_aggro_level)
		return;

	// 거리를 계산하여 가장 가까운 플레이어를 타겟으로 함
	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	XMFLOAT3 sub{};
	FLOAT length{}, min_length{ std::numeric_limits<FLOAT>::max() };
	for (INT id : ids) {
		if (-1 == id) continue;

		sub = Vector3::Sub(m_position, server.m_clients[id]->GetPosition());
		length = Vector3::Length(sub);
		if (length < min_length) {
			min_length = length;
			m_target_id = id;
		}
	}
	SetAggroLevel(AggroLevel::NORMAL_AGGRO);
}

void Monster::ChasePlayer(FLOAT elapsed_time)
{
	// 타게팅한 플레이어 추격
	XMFLOAT3 player_dir = GetPlayerDirection(m_target_id);

	if (Vector3::Equal(player_dir, XMFLOAT3(0.f, 0.f, 0.f)))
		return;

	UpdatePosition(player_dir, elapsed_time);
	UpdateRotation(player_dir);
	CollisionCheck();
}

void Monster::Retarget()
{
	// 타게팅 변경 중 두리번 거리는 행동
	// 계속 추격하지 않고 일정 시간마다 멈춰서도록 함
	UpdateTarget();
}

void Monster::Taunt()
{
	XMFLOAT3 player_dir = GetPlayerDirection(m_target_id);
	UpdateRotation(player_dir);
}

void Monster::PrepareAttack()
{
	// 공격 전 공격함을 알리기 위한 행동

	XMFLOAT3 player_dir = GetPlayerDirection(m_target_id);
	UpdateRotation(player_dir);
}

void Monster::Attack()
{
	// 플레이어 공격
	Server& server = Server::GetInstance();

	
	// 타겟과 거리가 멀면 추격 행동으로 전환
	// 타겟과 거리가 가까우면 공격 범위 내 행동으로 전환
	// 몬스터별 차이 둘 필요있음
}

void Monster::CollisionCheck()
{
	Server& server = Server::GetInstance(); 
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& monster_ids = game_room->GetMonsterIds();
	auto& player_ids = game_room->GetPlayerIds();

	
	server.CollideObject(shared_from_this(), monster_ids, Server::CollideByStatic);
	server.CollideObject(shared_from_this(), player_ids, Server::CollideByStatic);
}

void Monster::InitializePosition()
{
	// 파일을 읽어서 초기 위치를 정하는 함수 필요
	// 던전 매니저에서 해야하는 일이므로 나중에 던전매니저로 옮겨야 하는 함수
	// 던전 매니저는 지형지물, 몬스터 배치 등의 던전 정보들을 관리함

	constexpr DirectX::XMFLOAT3 monster_create_area[]
	{
		{ 9.f, 0.f, 36.f }, { 17.f, 0.f, 28.f }, { 17.f, 0.f, 20.f }, { 9.f, 0.f, 12.f },
		{ -9.f, 0.f, 36.f }, { -17.f, 0.f, 28.f }, { -17.f, 0.f, 20.f }, { -9.f, 0.f, 12.f },
	};
	std::uniform_int_distribution<int> area_distribution{ 0, static_cast<int>(std::size(monster_create_area) - 1) };
	SetPosition(monster_create_area[area_distribution(g_random_engine)]);
}

WarriorMonster::WarriorMonster()
{
	m_max_hp = 200.f;
	m_damage = 20;
	m_range = 1.5f;
	m_monster_type = MonsterType::WARRIOR;
	m_bounding_box.Center = XMFLOAT3(0.028f, 1.27f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.8f, 1.3f, 0.6f);
	m_bounding_box.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void WarriorMonster::Init()
{
	Monster::Init();

	m_hp = 200.f;
}

void WarriorMonster::Update(FLOAT elapsed_time)
{
	if (State::ST_INGAME != m_state) return;

	DoBehavior(elapsed_time);
}

ArcherMonster::ArcherMonster()
{
}

void ArcherMonster::Init()
{
	Monster::Init();
}

void ArcherMonster::Update(FLOAT elapsed_time)
{

}

WizardMonster::WizardMonster()
{
}

void WizardMonster::Init()
{
	Monster::Init();
}

void WizardMonster::Update(FLOAT elapsed_time)
{

}
