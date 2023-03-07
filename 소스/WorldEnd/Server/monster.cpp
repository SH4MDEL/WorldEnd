#include "monster.h"
#include "stdafx.h"
#include "Server.h"


void Monster::MonsterStateUpdate(XMVECTOR& look, float taketime)
{
	XMFLOAT3 velocity{};

	XMStoreFloat3(&velocity, look * m_speed);
	m_velocity = XMFLOAT3{ 0.0f, 0.0f, m_speed };

	// 몬스터 이동

	m_position.x += velocity.x * taketime;
	m_position.y += velocity.y * taketime;
	m_position.z += velocity.z * taketime;

	//cout << "taketime - " << taketime << endl;
}

XMVECTOR Monster::GetPlayerVector(UCHAR pl_id)
{
	XMVECTOR pl_pos{ XMLoadFloat3(&g_server.m_clients[pl_id].m_player_data.pos) };
	XMVECTOR mon_pos{ XMLoadFloat3(&m_position) };
	return XMVector3Normalize(pl_pos - mon_pos);
}

void Monster::SetId(char id)
{
	m_id = id;
}

void Monster::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_position = position;
}

void Monster::SetMonsterPosition()
{
	constexpr DirectX::XMFLOAT3 monster_create_area[]
	{
		{ 4.0f,	0.0f, 5.0f }, { 2.0f, 0.0f, -6.0f },{ 4.0f,  0.0f, 6.0f },	{ -3.0f, 0.0f, 7.0f },
		{ 5.0f, 0.0f, 7.0f }, { -10.0f, 0.0f,  15.0f },	{ 15.0f, 0.0f, 16.0f }, {  14.0f, 0.0f, -17.0f }
	};
	std::uniform_int_distribution<int> area_distribution{ 0, static_cast<int>(std::size(monster_create_area) - 1) };
	SetPosition(monster_create_area[area_distribution(g_random_engine)]);
}

void Monster::SetTargetId(char id)
{
	m_target_id = id;
}

CHAR Monster::GetId() const
{
	return m_id;
}

DirectX::XMFLOAT3 Monster::GetPosition()
{
	return m_position;
}

MonsterData Monster::GetData() const
{
	return MonsterData{ m_id, m_position, m_velocity, m_yaw };
}

UCHAR Monster::GetTargetId() const
{
	return m_target_id;
}

void WarriorMonster::TargetUpdate()
{
	if (!g_server.m_clients[m_target_id].m_exist_check)
		m_target_id = g_server.RecognizePlayer(GetPosition());
}

MonsterType Monster::GetType() const
{
	return m_monster_type;
}

WarriorMonster::WarriorMonster()
{
	m_monster_type = MonsterType::WARRIOR;
	m_hp = 200;
	m_damage = 20;
	m_speed = 0.8;
}

void WarriorMonster::CreateMonster(float taketime)
{
	if (m_hp <= 0) return;

	TargetUpdate();

	XMVECTOR look{GetPlayerVector(m_target_id)};

	// 플레이어를 향해서 이동
	MonsterStateUpdate(look , taketime);


}

void ArcherMonster::TargetUpdate()
{
}

ArcherMonster::ArcherMonster()
{
}

void ArcherMonster::CreateMonster(float taketime)
{
}

void WizsadMonster::TargetUpdate()
{
}

WizsadMonster::WizsadMonster()
{
}

void WizsadMonster::CreateMonster(float taketime)
{
}
