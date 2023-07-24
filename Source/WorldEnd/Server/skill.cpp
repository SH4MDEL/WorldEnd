#include "skill.h"

Skill::Skill() : m_name{}, m_damage{}, m_cooldown{}
{

}

void Skill::SetName(const std::string_view name)
{
	m_name = name;
}

void Skill::SetDamage(FLOAT damage)
{
	m_damage = damage;
}

void Skill::SetCooldown(std::chrono::milliseconds cooldown)
{
	m_cooldown = cooldown;
}

void Skill::SetType(SkillType type)
{
	m_type = type;
}

void Skill::SetSkillType(UCHAR type)
{
	m_skill_type = type;
}
