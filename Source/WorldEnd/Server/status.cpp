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

void Status::SetHpLevel(UCHAR level)
{
	m_hp_level = level;
	m_max_hp = PlayerSetting::DEFAULT_HP + PlayerSetting::HP_INCREASEMENT * level;
	m_hp = m_max_hp;
}

void Status::SetAtkLevel(UCHAR level)
{
	m_atk_level = level;
	m_atk = PlayerSetting::DEFAULT_ATK + PlayerSetting::ATK_INCREASEMENT * level;
}

void Status::SetDefLevel(UCHAR level)
{
	m_def_level = level;
	m_def = PlayerSetting::DEFAULT_DEF + PlayerSetting::DEF_INCREASEMENT * level;
}

void Status::SetCritRateLevel(UCHAR level)
{
	m_crit_rate_level = level;
	m_crit_rate = PlayerSetting::DEFAULT_CRIT_RATE + PlayerSetting::CRIT_RATE_INCREASEMENT * level;
}

void Status::SetCritDamageLevel(UCHAR level)
{
	m_crit_damage_level = level;
	m_crit_damage = PlayerSetting::DEFAULT_CRIT_DAMAGE + PlayerSetting::CRIT_DAMAGE_INCREASEMENT * level;
}

UCHAR Status::GetLevel(EnhancementType type) const
{
	UCHAR level{};
	
	switch(type) {
	case EnhancementType::HP:
		level = m_hp_level;
		break;
	case EnhancementType::ATK:
		level = m_atk_level;
		break;
	case EnhancementType::DEF:
		level = m_def_level;
		break;
	case EnhancementType::CRIT_RATE:
		level = m_crit_rate_level;
		break;
	case EnhancementType::CRIT_DAMAGE:
		level = m_crit_damage_level;
		break;
	}
	return level;
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
	FLOAT result_damage{ damage - m_def };
	if (result_damage <= std::numeric_limits<FLOAT>::epsilon())
		result_damage = 1.0f;

	m_hp -= result_damage;
	
	if (m_hp <= std::numeric_limits<FLOAT>::epsilon()) {
		m_hp = 0.f;
		return true;
	}
	return false;
}
