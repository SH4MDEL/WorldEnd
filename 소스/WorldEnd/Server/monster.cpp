#include "monster.h"
#include "stdafx.h"
#include "Server.h"

Monster::Monster() : m_target_id{ -1 }, m_current_animation{ ObjectAnimation::IDLE }
{
}

void Monster::UpdatePosition(XMVECTOR& dir, FLOAT elapsed_time)
{
	XMStoreFloat3(&m_velocity, dir * MonsterSetting::MONSTER_WALK_SPEED);

	// 몬스터 이동
	m_position.x += m_velocity.x * elapsed_time;
	m_position.y += m_velocity.y * elapsed_time;
	m_position.z += m_velocity.z * elapsed_time;

	m_bounding_box.Center = m_position;
}

void Monster::UpdateRotation(const XMVECTOR& dir)
{
	XMVECTOR z_axis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
	XMVECTOR radian{ XMVector3AngleBetweenNormals(z_axis, dir) };
	FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };

	m_yaw = XMVectorGetX(dir) < 0 ? -angle : angle;
}


XMVECTOR Monster::GetPlayerVector(INT player_id)
{
	XMFLOAT3 pos = g_server.m_clients[player_id].GetPosition();
	XMVECTOR player_pos{ XMLoadFloat3(&pos)};
	XMVECTOR monster_pos{ XMLoadFloat3(&m_position) };
	return XMVector3Normalize(player_pos - monster_pos);
}

void Monster::SetTargetId(INT id)
{
	m_target_id = id;
}

MONSTER_DATA Monster::GetData() const
{
	return MONSTER_DATA( m_id, m_position, m_velocity, m_yaw, m_hp );
}

void Monster::DecreaseHp(USHORT damage)
{
	m_hp -= damage;
	
	// 0이하로 떨어지면 사망처리 해야 함
	if (m_hp <= 0)
		m_id = -1;
}

bool Monster::ChangeAnimation(BYTE animation)
{
	if (m_current_animation == animation)
		return false;

	m_current_animation = animation;
	return true;
}

void Monster::InitializePosition()
{
	// 파일을 읽어서 초기 위치를 정하는 함수 필요
	// 던전 매니저에서 해야하는 일이므로 나중에 던전매니저로 옮겨야 하는 함수
	// 던전 매니저는 지형지물, 몬스터 배치 등의 던전 정보들을 관리함

	constexpr DirectX::XMFLOAT3 monster_create_area[]
	{
		{ 4.0f,	0.0f, 5.0f }, { 2.0f, 0.0f, -6.0f },{ 4.0f,  0.0f, 6.0f },	{ -3.0f, 0.0f, 7.0f },
		{ 5.0f, 0.0f, 7.0f }, { -10.0f, 0.0f,  15.0f },	{ 15.0f, 0.0f, 16.0f }, {  14.0f, 0.0f, -17.0f }
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

	XMVECTOR look{ GetPlayerVector(m_target_id) };

	// 플레이어를 향해서 이동
	UpdatePosition(look, elapsed_time);

	UpdateRotation(look);
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
