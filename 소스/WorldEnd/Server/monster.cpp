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

	// ���� �̵�
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

	// degree - Ŭ���̾�Ʈ���� degree �� Rotation �� ó����
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
		// �߰����� �ٲ�� retarget �ð� ���� �ѷ������� ��
		// retarget �ð� ���� �ѷ������ ����
		m_current_animation = ObjectAnimation::WALK;
		ev.event_time = current_time + MonsterSetting::MONSTER_RETARGET_TIME;
		ev.behavior_type = MonsterBehavior::LOOK_AROUND;
		break;
	case MonsterBehavior::LOOK_AROUND:
		// Retarget �ϸ鼭 �ѷ����� �ִϸ��̼� ���
		// �ѷ����� �ð� ���� CHASE �� ����
		m_current_animation = MonsterAnimation::LOOK_AROUND;
		ev.event_time = current_time + MonsterSetting::MONSTER_LOOK_AROUND_TIME;
		ev.behavior_type = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		// ���� �غ� �ð� ���� �������� ����
		m_current_animation = MonsterAnimation::TAUNT;
		ev.event_time = current_time + MonsterSetting::MONSTER_PREPARE_ATTACK_TIME;
		ev.behavior_type = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		// ���� �����ϸ� ����, ���� ���� ���� �غ� ����ȯ 
		// �Ұ����ϸ� �ٷ� �߰�
		
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
	
	// �ൿ �� PREPARE_ATTACK, ATTACK �� �ƴϸ� ���� �������� üũ
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
		// ������ ����
		ChangeBehavior(MonsterBehavior::DEAD);
		return;
	}

	// ���� ���� �̻� �������� ������ Ÿ�� ����
	if ((m_hp / 11.f - damage) <= std::numeric_limits<FLOAT>::epsilon()) {
		SetTarget(id);
		m_last_behavior_time = std::chrono::system_clock::now();
	}
}

void Monster::UpdateTarget()
{
	// �Ÿ��� ����Ͽ� ���� ����� �÷��̾ Ÿ������ ��
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
	// Ÿ������ �÷��̾� �߰�
	XMFLOAT3 player_dir = GetPlayerDirection(m_target_id);

	UpdatePosition(player_dir, elapsed_time);
	UpdateRotation(player_dir);
	CollisionCheck();
}

void Monster::LookAround()
{
	// Ÿ���� ���� �� �θ��� �Ÿ��� �ൿ
	// ��� �߰����� �ʰ� ���� �ð����� ���缭���� ��
	UpdateTarget();
}

void Monster::PrepareAttack()
{
	// ���� �� �������� �˸��� ���� �ൿ

	XMFLOAT3 player_dir = GetPlayerDirection(m_target_id);
	UpdateRotation(player_dir);
}

void Monster::Attack()
{
	// �÷��̾� ����
	Server& server = Server::GetInstance();

	
	// Ÿ�ٰ� �Ÿ��� �ָ� �߰� �ൿ���� ��ȯ
	// Ÿ�ٰ� �Ÿ��� ������ ���� ���� �� �ൿ���� ��ȯ
	// ���ͺ� ���� �� �ʿ�����
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
	// ������ �о �ʱ� ��ġ�� ���ϴ� �Լ� �ʿ�
	// ���� �Ŵ������� �ؾ��ϴ� ���̹Ƿ� ���߿� �����Ŵ����� �Űܾ� �ϴ� �Լ�
	// ���� �Ŵ����� ��������, ���� ��ġ ���� ���� �������� ������

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

	// �ൿ �� PREPARE_ATTACK, ATTACK �� �ƴϸ� ���� �������� üũ
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
		// �߰����� �ٲ�� retarget �ð� ���� �ѷ������� ��
		// retarget �ð� ���� �ѷ������ ����
		m_current_animation = ObjectAnimation::WALK;
		ev.event_time = current_time + MonsterSetting::MONSTER_RETARGET_TIME;
		ev.behavior_type = MonsterBehavior::LOOK_AROUND;
		break;
	case MonsterBehavior::LOOK_AROUND:
		// Retarget �ϸ鼭 �ѷ����� �ִϸ��̼� ���
		// �ѷ����� �ð� ���� CHASE �� ����
		m_current_animation = MonsterAnimation::LOOK_AROUND;
		ev.event_time = current_time + MonsterSetting::MONSTER_LOOK_AROUND_TIME;
		ev.behavior_type = MonsterBehavior::CHASE;
		break;
	case MonsterBehavior::PREPARE_ATTACK:
		// ���� �غ� �ð� ���� �������� ����
		m_current_animation = MonsterAnimation::TAUNT;
		ev.event_time = current_time + MonsterSetting::MONSTER_PREPARE_ATTACK_TIME;
		ev.behavior_type = MonsterBehavior::ATTACK;
		break;
	case MonsterBehavior::ATTACK:
		// ���� �����ϸ� ����, ���� ���� ���� �غ� ����ȯ 
		// �Ұ����ϸ� �ٷ� �߰�

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
	// �÷��̾� ����
	Server& server = Server::GetInstance();

	AttackPlayer();

	// Ÿ�ٰ� �Ÿ��� �ָ� �߰� �ൿ���� ��ȯ
	// Ÿ�ٰ� �Ÿ��� ������ ���� ���� �� �ൿ���� ��ȯ
	// ���ͺ� ���� �� �ʿ�����

	
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
