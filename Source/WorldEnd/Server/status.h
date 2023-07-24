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
	void SetHpLevel(UCHAR level);
	void SetAtkLevel(UCHAR level);
	void SetDefLevel(UCHAR level);
	void SetCritRateLevel(UCHAR level);
	void SetCritDamageLevel(UCHAR level);


	FLOAT GetHp() const { return m_hp; }
	FLOAT GetMaxHp() const { return m_max_hp; }
	FLOAT GetAtk() const { return m_atk; }
	FLOAT GetDef() const { return m_def; }
	FLOAT GetCritRate() const { return m_crit_rate; }
	FLOAT GetCritDamage() const { return m_crit_damage; }
	UCHAR GetHpLevel() const { return m_hp_level; }
	UCHAR GetAtkLevel() const { return m_atk_level; }
	UCHAR GetDefLevel() const { return m_def_level; }
	UCHAR GetCritRateLevel() const { return m_crit_rate_level; }
	UCHAR GetCritDamageLevel() const { return m_crit_damage_level; }

	FLOAT CalculateDamage();
	bool CalculateHitDamage(FLOAT damage);

private:
	FLOAT			m_hp;
	FLOAT			m_max_hp;
	FLOAT			m_atk;
	FLOAT			m_def;
	FLOAT			m_crit_rate;
	FLOAT			m_crit_damage;

	UCHAR			m_hp_level;
	UCHAR			m_atk_level;
	UCHAR			m_def_level;
	UCHAR			m_crit_rate_level;
	UCHAR			m_crit_damage_level;
};

