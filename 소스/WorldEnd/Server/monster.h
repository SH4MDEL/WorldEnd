#pragma once
#include "stdafx.h"
#include "object.h"

class Monster : public MovementObject, public std::enable_shared_from_this<Monster>
{
public:
	Monster();
	virtual ~Monster() = default;

	virtual void Update(FLOAT elapsed_time) override {};

	void SetTarget(INT player_id);

	virtual MONSTER_DATA GetMonsterData() const override;
	virtual MonsterType GetMonsterType() const override { return m_monster_type; }
	UCHAR GetTargetId() const { return m_target_id; }
	std::chrono::system_clock::time_point GetLastBehaviorTime() const;
	MonsterBehavior GetBehavior() const { return m_current_behavior; }

	bool ChangeAnimation(BYTE animation);

	void ChangeBehavior(MonsterBehavior behavior);
	void DoBehavior(FLOAT elapsed_time);
	bool IsDoAttack();
	void DecreaseHp(FLOAT damage, INT id);

	void UpdateTarget();					// Ÿ���� ����
	void ChasePlayer(FLOAT elapsed_time);	// �߰�
	void LookAround();						// �߰� �� ���
	void PrepareAttack();					// ���� �غ�
	void Attack();							// ����
	void CollisionCheck();

	// ���߿� ���� �Ŵ����� �Űܾ� �� �Լ�
	void InitializePosition();

protected:
	MonsterType								m_monster_type;
	FLOAT									m_range;
	INT										m_target_id;
	USHORT									m_current_animation;
	MonsterBehavior							m_current_behavior;
	std::chrono::system_clock::time_point	m_last_behavior_time;

	void UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time);
	void UpdateRotation(const XMFLOAT3& dir);
	XMFLOAT3 GetPlayerDirection(INT player_id);
	bool CanAttack();
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

