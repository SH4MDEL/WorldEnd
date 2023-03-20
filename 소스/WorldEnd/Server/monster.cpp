#include "monster.h"
#include "stdafx.h"
#include "Server.h"

Monster::Monster() : m_target_id{ -1 }, m_current_animation{ ObjectAnimation::IDLE }
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
	return Vector3::Sub(server.m_clients[player_id]->GetPosition(), m_position);
}

void Monster::SetTarget(INT id)
{
	m_target_id = id;
}

MONSTER_DATA Monster::GetMonsterData() const
{
	return MONSTER_DATA( m_id, m_position, m_velocity, m_yaw, m_hp );
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

	// 애니메이션 추가가 가능하면
	// LOOKAROUND 는 두리번거리기
	// PREPAREATTACK 는 경계하기 로 변경하면 됨
	switch (m_current_behavior) {
	case MonsterBehavior::CHASE:
		m_current_animation = ObjectAnimation::WALK;
		break;
	case MonsterBehavior::LOOKAROUND:
		m_current_animation = ObjectAnimation::IDLE;
		break;
	case MonsterBehavior::PREPAREATTACK:
		m_current_animation = ObjectAnimation::IDLE;
		break;
	case MonsterBehavior::ATTACK:
		m_current_animation = ObjectAnimation::ATTACK;
		break;
	case MonsterBehavior::NONE:
		break;
	}

	// 동작이 변할 때 해당 변화를 클라이언트에 전송
	// 동작이 변할 때 애니메이션도 변하므로 같이 전송함
	Server& server = Server::GetInstance();
	auto game_room = server.GetGameRoomManager()->GetGameRoom(m_room_num);
	auto& ids = game_room->GetPlayerIds();

	SC_CHANGE_MONSTER_BEHAVIOR_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_MONSTER_BEHAVIOR;
	packet.behavior = m_current_behavior;
	packet.animation = m_current_animation;

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
	case MonsterBehavior::LOOKAROUND:
		LookAround();
		break;
	case MonsterBehavior::PREPAREATTACK:
		PrepareAttack();
		break;
	case MonsterBehavior::ATTACK:
		Attack();
		break;
	case MonsterBehavior::NONE:
		break;
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

	// 타이머 큐에 이동주기(지정 필요) 후에 추격 행동을 이어가는 이벤트 추가
	// ChangeBehavior가 불릴 필요 없음
}

void Monster::LookAround()
{
	// 타게팅 변경 중 두리번 거리는 행동
	// 계속 추격하지 않고 일정 시간마다 멈춰서도록 함

	// 타이머 큐에 0.5~1초 뒤에 추격 행동으로 전환하는 이벤트 추가
}

void Monster::PrepareAttack()
{
	// 공격 전 공격함을 알리기 위한 행동

	// 타이머 큐에 0.5~1초 뒤에 공격 행동으로 전환하는 이벤트 추가
}

void Monster::Attack()
{
	// 플레이어 공격
	
	UpdateTarget();

	Server& server = Server::GetInstance();
	

	FLOAT dist = Vector3::Distance(m_position, server.m_clients[m_target_id]->GetPosition());
	
	// 타겟과 거리가 멀면 추격 행동으로 전환
	// 타겟과 거리가 가까우면 공격 범위 내 행동으로 전환
	// 몬스터별 차이 둘 필요있음
}

void Monster::InitializePosition()
{
	// 파일을 읽어서 초기 위치를 정하는 함수 필요
	// 던전 매니저에서 해야하는 일이므로 나중에 던전매니저로 옮겨야 하는 함수
	// 던전 매니저는 지형지물, 몬스터 배치 등의 던전 정보들을 관리함

	constexpr DirectX::XMFLOAT3 monster_create_area[]
	{
		{ 4.0f,	0.0f, 5.0f }, { 2.0f, 0.0f, -6.0f },{ 4.0f,  0.0f, 6.0f },	{ -3.0f, 0.0f, 7.0f },
		{ 5.0f, 0.0f, 7.0f }, { -1.0f, 0.0f,  5.0f },	{ 5.0f, 0.0f, 6.0f }, {  7.0f, 0.0f, -1.0f }
	};
	std::uniform_int_distribution<int> area_distribution{ 0, static_cast<int>(std::size(monster_create_area) - 1) };
	SetPosition(monster_create_area[area_distribution(g_random_engine)]);
}


WarriorMonster::WarriorMonster()
{
	m_monster_type = MonsterType::WARRIOR;
	m_hp = 200;
	m_damage = 20;
	m_bounding_box.Center = XMFLOAT3(0.028f, 1.27f, 0.f);
	m_bounding_box.Extents = XMFLOAT3(0.8f, 1.3f, 0.5f);
	m_bounding_box.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void WarriorMonster::Update(FLOAT elapsed_time)
{
	if (m_hp <= 0) return;

	DoBehavior(elapsed_time);
}

ArcherMonster::ArcherMonster()
{

}

void ArcherMonster::Update(FLOAT elapsed_time)
{

}

WizardMonster::WizardMonster()
{

}

void WizardMonster::Update(FLOAT elapsed_time)
{

}
