#include "monster.h"
#include "stdafx.h"


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
		{ 0.0f,	0.0f, 5.0f }, { 2.0f, 0.0f, -6.0f },{ 4.0f,  0.0f, 0.0f },	{ -3.0f, 0.0f, 0.0f },
		{ 5.0f, 0.0f, 7.0f }, { -10.0f, 0.0f,  15.0f },	{ 15.0f, 0.0f, 16.0f }, {  14.0f, 0.0f, -17.0f }
	};
	const std::uniform_int_distribution<int> area_distribution{ 0, static_cast<int>(std::size(monster_create_area) - 1) };
	SetPosition(monster_create_area[area_distribution(g_random_engine)]);
}

CHAR Monster::GetId() const
{
	return m_id;
}

DirectX::XMFLOAT3 Monster::GetPosition() const
{
	return m_position;
}

MonsterData Monster::GetData() const
{
	return MonsterData{ m_id, m_position, m_velocity, m_yaw };
}

WarriorMonster::WarriorMonster()
{
	m_monster_type = MonsterType::WARRIOR;
	m_hp = 200;
	m_damage = 20;
	m_speed = 10;
}

void WarriorMonster::CreateMonster()
{
	if (m_hp <= 0) return;


}

ArcherMonster::ArcherMonster()
{
}

void ArcherMonster::CreateMonster()
{
}

WizsadMonster::WizsadMonster()
{
}

void WizsadMonster::CreateMonster()
{
}
