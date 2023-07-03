#include "status.h"

std::uniform_real_distribution<FLOAT> urd(0, 100.f);

Status::Status() : m_hp{ 0.f }, m_max_hp{ 0.f }, m_atk { 0.f }, m_def{ 0.f },
	m_crit_rate{ 0.f }, m_crit_damage{ 0.f }
{

}

void Status::Init()
{
	m_hp = m_max_hp;
}

void Status::SetHp(FLOAT hp)
{
	m_hp = hp;
}

void Status::SetMaxHp(FLOAT hp)
{
	m_max_hp = hp;
}

void Status::SetAtk(FLOAT atk)
{
	m_atk = atk;
}

void Status::SetDef(FLOAT def)
{
	m_def = def;
}

void Status::SetCritRate(FLOAT value)
{
	m_crit_rate = value;
}

void Status::SetCritDamage(FLOAT value)
{
	m_crit_damage = value;
}

FLOAT Status::CalculateDamage()
{
	if (m_crit_rate >= urd(g_random_engine)) {
		return m_atk * m_crit_damage;
	}

	return m_atk;
}

bool Status::CalculateHitDamage(FLOAT damage)
{
	m_hp -= (damage - m_def);

	if (m_hp <= 0.f) {
		m_hp = 0.f;
		return true;
	}
	return false;
}
