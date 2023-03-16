#pragma once
#include "stdafx.h"
#include "object.h"

class Monster : public MovementObject
{
public:
	Monster();
	virtual ~Monster() = default;

	virtual void Update(FLOAT elapsed_time) override {};

	void SetTargetId(INT player_id);

	virtual MONSTER_DATA GetMonsterData() const override;
	virtual MonsterType GetMonsterType() const override { return m_monster_type; }
	UCHAR GetTargetId() const { return m_target_id; }

	void DecreaseHp(USHORT damage);

	bool ChangeAnimation(BYTE animation);

	// 나중에 던전 매니저로 옮겨야 할 함수
	void InitializePosition();

protected:
	MonsterType		m_monster_type;
	INT				m_target_id;
	USHORT			m_current_animation;

	void UpdatePosition(XMVECTOR& dir, FLOAT elapsed_time);
	void UpdateRotation(const XMVECTOR& dir);
	XMVECTOR GetPlayerVector(INT player_id);
};

class WarriorMonster : public Monster
{
public:
	WarriorMonster();
	virtual ~WarriorMonster() = default;

	virtual void Update(FLOAT elapsed_time) override;
};

class ArcherMonster : public Monster
{
public:
	ArcherMonster();
	virtual ~ArcherMonster() = default;

	virtual void Update(FLOAT elapsed_time) override;
};

class WizardMonster : public Monster
{
public:
	WizardMonster();
	virtual ~WizardMonster() = default;

	virtual void Update(FLOAT elapsed_time) override;
};

