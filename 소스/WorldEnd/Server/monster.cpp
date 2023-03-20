#include "monster.h"
#include "stdafx.h"
#include "Server.h"

Monster::Monster() : m_target_id{ -1 }, m_current_animation{ ObjectAnimation::IDLE }
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

	// �ִϸ��̼� �߰��� �����ϸ�
	// LOOKAROUND �� �θ����Ÿ���
	// PREPAREATTACK �� ����ϱ� �� �����ϸ� ��
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

	// ������ ���� �� �ش� ��ȭ�� Ŭ���̾�Ʈ�� ����
	// ������ ���� �� �ִϸ��̼ǵ� ���ϹǷ� ���� ������
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

	// Ÿ�̸� ť�� �̵��ֱ�(���� �ʿ�) �Ŀ� �߰� �ൿ�� �̾�� �̺�Ʈ �߰�
	// ChangeBehavior�� �Ҹ� �ʿ� ����
}

void Monster::LookAround()
{
	// Ÿ���� ���� �� �θ��� �Ÿ��� �ൿ
	// ��� �߰����� �ʰ� ���� �ð����� ���缭���� ��

	// Ÿ�̸� ť�� 0.5~1�� �ڿ� �߰� �ൿ���� ��ȯ�ϴ� �̺�Ʈ �߰�
}

void Monster::PrepareAttack()
{
	// ���� �� �������� �˸��� ���� �ൿ

	// Ÿ�̸� ť�� 0.5~1�� �ڿ� ���� �ൿ���� ��ȯ�ϴ� �̺�Ʈ �߰�
}

void Monster::Attack()
{
	// �÷��̾� ����
	
	UpdateTarget();

	Server& server = Server::GetInstance();
	

	FLOAT dist = Vector3::Distance(m_position, server.m_clients[m_target_id]->GetPosition());
	
	// Ÿ�ٰ� �Ÿ��� �ָ� �߰� �ൿ���� ��ȯ
	// Ÿ�ٰ� �Ÿ��� ������ ���� ���� �� �ൿ���� ��ȯ
	// ���ͺ� ���� �� �ʿ�����
}

void Monster::InitializePosition()
{
	// ������ �о �ʱ� ��ġ�� ���ϴ� �Լ� �ʿ�
	// ���� �Ŵ������� �ؾ��ϴ� ���̹Ƿ� ���߿� �����Ŵ����� �Űܾ� �ϴ� �Լ�
	// ���� �Ŵ����� ��������, ���� ��ġ ���� ���� �������� ������

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
