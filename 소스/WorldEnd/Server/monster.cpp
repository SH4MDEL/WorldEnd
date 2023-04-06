#include "monster.h"
#include <functional>
#include "stdafx.h"
#include "Server.h"

Monster::Monster() : m_target_id{ -1 }, m_current_animation{ ObjectAnimation::IDLE },
	m_current_behavior{ MonsterBehavior::CHASE }
{
}

void Monster::UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time)
{
	m_velocity = Vector3::Mul(dir, MonsterSetting::MONSTER_WALK_SPEED);

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
		return m_position;

	Server& server = Server::GetInstance();
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

void Monster::SetTarget(INT id)
{
	m_target_id = id;
}

MONSTER_DATA Monster::GetMonsterData() const
{
	return MONSTER_DATA( m_id, m_position, m_velocity, m_yaw, m_hp );
}

std::chrono::system_clock::time_point Monster::GetLastBehaviorTime() const
{
	return m_last_behavior_time;
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
	ev.event_type = EventType::CHANGE_BEHAVIOR;
	auto current_time = std::chrono::system_clock::now();
	m_last_behavior_time = current_time;

	switch (m_current_behavior) {
	case MonsterBehavior::CHASE:
		// 추격으로 바뀌면 retarget 시간 이후 둘러보도록 함
		// retarget 시간 이후 둘러보기로 변경
		m_current_animation = ObjectAnimation::WALK;
		ev.event_time = current_time + MonsterSetting::MONSTER_RETARGET_TIME;
		ev.behavior_type = MonsterBehavior::LOOK_AROUND;
		break;
	case MonsterBehavior::LOOK_AROUND:
		// Retarget 하면서 둘러보는 애니메이션 출력
		// 둘러보는 시간 이후 CHASE 로 변경
		m_current_animation = MonsterAnimation::LOOK_AROUND;
		ev.event_time = current_time + MonsterSetting::MONSTER_LOOK_AROUND_TIME;
		ev.behavior_type = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		// 공격 준비 시간 이후 공격으로 변경
		m_current_animation = MonsterAnimation::TAUNT;
		ev.event_time = current_time + MonsterSetting::MONSTER_PREPARE_ATTACK_TIME;
		ev.behavior_type = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		// 공격 가능하면 공격, 공격 이후 공격 준비 재전환 
		// 불가능하면 바로 추격
		
		if (CanAttack()) {
			m_current_animation = ObjectAnimation::ATTACK;
			ev.event_time = current_time + MonsterSetting::MONSTER_ATTACK_TIME;
			ev.behavior_type = MonsterBehavior::PREPARE_ATTACK;
		}
		else {
			m_current_animation = ObjectAnimation::WALK;
			ev.event_time = current_time;
			ev.behavior_type = MonsterBehavior::CHASE;
		}

		break;
	case MonsterBehavior::DEAD:
		m_current_animation = ObjectAnimation::DEAD;
		ev.event_time = current_time + MonsterSetting::MONSTER_DEAD_TIME;
		ev.behavior_type = MonsterBehavior::DEAD;
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
	case MonsterBehavior::LOOK_AROUND:
		LookAround();
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

	// 일정 비율 이상 데미지가 들어오면 타겟 변경
	if ((m_hp / 11.f - damage) <= std::numeric_limits<FLOAT>::epsilon()) {
		SetTarget(id);
		m_last_behavior_time = std::chrono::system_clock::now();
	}
}

void Monster::UpdateTarget()
{
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
}

void Monster::ChasePlayer(FLOAT elapsed_time)
{
	// 타게팅한 플레이어 추격
	XMFLOAT3 player_dir = GetPlayerDirection(m_target_id);

	UpdatePosition(player_dir, elapsed_time);
	UpdateRotation(player_dir);
	CollisionCheck();
}

void Monster::LookAround()
{
	// 타게팅 변경 중 두리번 거리는 행동
	// 계속 추격하지 않고 일정 시간마다 멈춰서도록 함
	UpdateTarget();
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

	
	server.CollisionCheck(shared_from_this(), monster_ids, Server::CollideByStaticOBB);
	server.CollisionCheck(shared_from_this(), player_ids, Server::CollideByStaticOBB);
}

void Monster::InitializePosition()
{
	// 파일을 읽어서 초기 위치를 정하는 함수 필요
	// 던전 매니저에서 해야하는 일이므로 나중에 던전매니저로 옮겨야 하는 함수
	// 던전 매니저는 지형지물, 몬스터 배치 등의 던전 정보들을 관리함

	constexpr DirectX::XMFLOAT3 monster_create_area[]
	{
		{ 14.0f, 0.0f, 15.0f }, { 12.0f, 0.0f, -16.0f },{ 14.0f, 0.0f, 16.0f },	{ -13.0f, 0.0f, 17.0f },
		{ 15.0f, 0.0f, 17.0f }, { -11.0f, 0.0f, 15.0f },{ 15.0f, 0.0f, 16.0f }, { 17.0f, 0.0f, -11.0f }
	};
	std::uniform_int_distribution<int> area_distribution{ 0, static_cast<int>(std::size(monster_create_area) - 1) };
	SetPosition(monster_create_area[area_distribution(g_random_engine)]);
}


WarriorMonster::WarriorMonster()
{
	m_monster_type = MonsterType::WARRIOR;
	m_hp = 200.f;
	m_damage = 20;
	m_range = 1.f;
	m_bounding_box.Center = XMFLOAT3(0.028f, 1.27f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.8f, 1.3f, 0.6f);
	m_bounding_box.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void WarriorMonster::Update(FLOAT elapsed_time)
{
	if (State::ST_INGAME != m_state) return;

	DoBehavior(elapsed_time);
}

ArcherMonster::ArcherMonster()
{
	m_monster_type = MonsterType::ARCHER;
	m_hp = 150.f;
	m_damage = 25;
	m_range = 1.f;
	m_bounding_box.Center = XMFLOAT3(0.028f, 1.27f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.8f, 1.3f, 0.6f);
	m_bounding_box.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void ArcherMonster::Update(FLOAT elapsed_time)
{
	if (State::ST_INGAME != m_state) return;

	DoBehavior(elapsed_time);
}

void ArcherMonster::DoBehavior(FLOAT elapsed_time)
{
	switch (m_current_behavior) {
	case MonsterBehavior::CHASE:
		ChasePlayer(elapsed_time);
		break;
	case MonsterBehavior::LOOK_AROUND:
		LookAround();
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
	if (!IsDoAttack()) {
		if (CanAttack()) {
			ChangeBehavior(MonsterBehavior::PREPARE_ATTACK);
		}
	}
}

void ArcherMonster::ChangeBehavior(MonsterBehavior behavior)
{
	m_current_behavior = behavior;

	TIMER_EVENT ev{};
	ev.obj_id = m_id;
	ev.event_type = EventType::CHANGE_BEHAVIOR;
	auto current_time = std::chrono::system_clock::now();
	m_last_behavior_time = current_time;

	switch (m_current_behavior) {
	case MonsterBehavior::CHASE:
		// 추격으로 바뀌면 retarget 시간 이후 둘러보도록 함
		// retarget 시간 이후 둘러보기로 변경
		m_current_animation = ObjectAnimation::WALK;
		ev.event_time = current_time + MonsterSetting::MONSTER_RETARGET_TIME;
		ev.behavior_type = MonsterBehavior::LOOK_AROUND;
		break;
	case MonsterBehavior::LOOK_AROUND:
		// Retarget 하면서 둘러보는 애니메이션 출력
		// 둘러보는 시간 이후 CHASE 로 변경
		m_current_animation = MonsterAnimation::LOOK_AROUND;
		ev.event_time = current_time + MonsterSetting::MONSTER_LOOK_AROUND_TIME;
		ev.behavior_type = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		// 공격 준비 시간 이후 공격으로 변경
		m_current_animation = MonsterAnimation::TAUNT;
		ev.event_time = current_time + MonsterSetting::MONSTER_PREPARE_ATTACK_TIME;
		ev.behavior_type = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		// 공격 가능하면 공격, 공격 이후 공격 준비 재전환 
		// 불가능하면 바로 추격

		if (CanAttack()) {
			m_current_animation = ObjectAnimation::ATTACK;
			ev.event_time = current_time + MonsterSetting::MONSTER_ATTACK_TIME;
			ev.behavior_type = MonsterBehavior::PREPARE_ATTACK;
		}
		else {
			m_current_animation = ObjectAnimation::WALK;
			ev.event_time = current_time;
			ev.behavior_type = MonsterBehavior::CHASE;
		}

		break;
	case MonsterBehavior::DEAD:
		m_current_animation = ObjectAnimation::DEAD;
		ev.event_time = current_time + MonsterSetting::MONSTER_DEAD_TIME;
		ev.behavior_type = MonsterBehavior::DEAD;
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

void ArcherMonster::Attack()
{
	// 플레이어 공격
	Server& server = Server::GetInstance();

	AttackPlayer();

	// 타겟과 거리가 멀면 추격 행동으로 전환
	// 타겟과 거리가 가까우면 공격 범위 내 행동으로 전환
	// 몬스터별 차이 둘 필요있음

	
}

void ArcherMonster::CollisionCheck()
{
	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& monster_ids = game_room->GetMonsterIds();
	auto& player_ids = game_room->GetPlayerIds();

	server.CollisionCheck(shared_from_this(), monster_ids, Server::CollideByStaticOBB);
	server.CollisionCheck(shared_from_this(), player_ids, Server::CollideByStaticOBB);
}

void ArcherMonster::AttackPlayer()
{
	CollisionCheck();



}

WizardMonster::WizardMonster()
{

}

void WizardMonster::Update(FLOAT elapsed_time)
{

}
