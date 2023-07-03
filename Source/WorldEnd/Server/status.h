#pragma once
#include "stdafx.h"


class Status
{
public:
	Status();
	~Status() = default;

	void Init();

	void SetHp(FLOAT hp);
	void SetMaxHp(FLOAT hp);
	void SetAtk(FLOAT atk);
	void SetDef(FLOAT def);
	void SetCritRate(FLOAT value);
	void SetCritDamage(FLOAT value);

	FLOAT GetHp() const { return m_hp; }
	FLOAT GetMaxHp() const { return m_max_hp; }
	FLOAT GetAtk() const { return m_atk; }
	FLOAT GetDef() const { return m_def; }
	FLOAT GetCritRate() const { return m_crit_rate; }
	FLOAT GetCritDamage() const { return m_crit_damage; }

	FLOAT CalculateDamage();
	bool CalculateHitDamage(FLOAT damage);

private:
	FLOAT			m_hp;
	FLOAT			m_max_hp;
	FLOAT			m_atk;
	FLOAT			m_def;
	FLOAT			m_crit_rate;
	FLOAT			m_crit_damage;
};

