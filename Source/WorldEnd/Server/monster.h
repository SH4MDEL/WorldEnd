#pragma once
#include "stdafx.h"
#include "object.h"

class Monster : public MovementObject, public std::enable_shared_from_this<Monster>
{
public:
	Monster();
	virtual ~Monster() = default;

	virtual void Init();
	virtual void Update(FLOAT elapsed_time) override {};

	void SetTarget(INT player_id);
	void SetAggroLevel(BYTE aggro_level);
	void SetBehaviorId(BYTE id);

	virtual MONSTER_DATA GetMonsterData() const override;
	virtual MonsterType GetMonsterType() const override { return m_monster_type; }
	UCHAR GetTargetId() const { return m_target_id; }
	BYTE GetAggroLevel() const { return m_aggro_level; }
	MonsterBehavior GetBehavior() const { return m_current_behavior; }
	BYTE GetLastBehaviorId() const { return m_last_behavior_id; }

	bool ChangeAnimation(BYTE animation);

	void ChangeBehavior(MonsterBehavior behavior);
	void DoBehavior(FLOAT elapsed_time);
	bool IsDoAttack();
	virtual void DecreaseHp (FLOAT damage, INT id) override;
	void DecreaseAggroLevel();
	bool CheckPlayer();

	void UpdateTarget();					// 타게팅 설정
	void ChasePlayer(FLOAT elapsed_time);	// 추격
	void Retarget();						// 추격 중 대기
	void Taunt();
	void PrepareAttack();					// 공격 준비
	void Attack();							// 공격
	void CollisionCheck();

	// 나중에 던전 매니저로 옮겨야 할 함수
	void InitializePosition();

protected:
	MonsterType			m_monster_type;
	FLOAT				m_range;
	INT					m_target_id;
	USHORT				m_current_animation;
	MonsterBehavior		m_current_behavior;
	BYTE				m_aggro_level;
	BYTE				m_last_behavior_id;

	void UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time);
	void UpdateRotation(const XMFLOAT3& dir);
	XMFLOAT3 GetPlayerDirection(INT player_id);
	bool CanAttack();
	void SetDecreaseAggroLevelEvent();
	void SetBehaviorTimerEvent(MonsterBehavior behavior);
};

class WarriorMonster : public Monster
{
public:
	WarriorMonster();
	virtual ~WarriorMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;
};

class ArcherMonster : public Monster
{
public:
	ArcherMonster();
	virtual ~ArcherMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;
};

class WizardMonster : public Monster
{
public:
	WizardMonster();
	virtual ~WizardMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;
};

