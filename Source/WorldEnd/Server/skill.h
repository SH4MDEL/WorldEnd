#pragma once
#include "stdafx.h"

enum class SkillType : CHAR {
	NORMAL, ULTIMATE, COUNT
};

class Skill
{
public:
	Skill();
	~Skill() = default;

	void SetName(const std::string_view name);
	void SetDamage(FLOAT damage);
	void SetCooldown(std::chrono::milliseconds cooldown);
	void SetType(SkillType type);
	void SetSkillType(UCHAR type);

	std::string GetName() const { return m_name; }
	FLOAT GetDamage() const { return m_damage; }
	std::chrono::milliseconds GetCooldown() const { return m_cooldown; }
	SkillType GetType() const { return m_type; }
	UCHAR GetSkillType() const { return m_skill_type; }

private:
	std::string					m_name;
	FLOAT						m_damage;
	std::chrono::milliseconds	m_cooldown;
	SkillType					m_type;
	UCHAR						m_skill_type;

};

