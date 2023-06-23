#pragma once
#include "stdafx.h"
#include "object.h"

class Monster : public MovementObject, public std::enable_shared_from_this<Monster>
{
public:
	Monster();
	virtual ~Monster() = default;

	virtual void Init();
	virtual void Update(FLOAT elapsed_time) override;

	void SetTarget(INT player_id);
	void SetAggroLevel(BYTE aggro_level);
	void SetBehaviorId(BYTE id);

	virtual MONSTER_DATA GetMonsterData() const override;
	virtual MonsterType GetMonsterType() const override { return m_monster_type; }
	UCHAR GetTargetId() const { return m_target_id; }
	BYTE GetAggroLevel() const { return m_aggro_level; }
	MonsterBehavior GetBehavior() const { return m_current_behavior; }
	BYTE GetLastBehaviorId() const { return m_last_behavior_id; }
	INT  GetPlayerHighestDamage();

	bool ChangeAnimation(BYTE animation);

	void ChangeBehavior(MonsterBehavior behavior);
	void DoBehavior(FLOAT elapsed_time);
	virtual void DecreaseHp (FLOAT damage, INT id) override;
	void DecreaseAggroLevel();
	bool CheckPlayer();

	void UpdateTarget();					// 타게팅 설정
	void ChasePlayer(FLOAT elapsed_time);	// 추격
	void Retarget();						// 추격 중 대기
	void Taunt();
	void PrepareAttack();					// 공격 준비
	void Attack();							// 공격
	virtual void StepBack(FLOAT elapsed_time) {}
	virtual void Flee(FLOAT elapsed_time) {}
	void CollisionCheck();

	void RandomTarget();

	

	// 나중에 던전 매니저로 옮겨야 할 함수
	void InitializePosition(INT mon_cnt, MonsterType mon_type, INT random_map, FLOAT mon_pos[]);

protected:
	void UpdatePosition(const XMFLOAT3& dir, FLOAT elapsed_time, FLOAT speed);
	void UpdateRotation(const XMFLOAT3& dir);
	XMFLOAT3 GetDirection(INT id);
	bool IsInRange(FLOAT range);
	void SetDecreaseAggroLevelEvent();
	void SetBehaviorTimerEvent(MonsterBehavior behavior);
	virtual bool CanSwapAttackBehavior() = 0;
	virtual MonsterBehavior SetNextBehavior(MonsterBehavior behavior) = 0;
	virtual void SetBehaviorAnimation(MonsterBehavior behavior) = 0;
	virtual std::chrono::milliseconds SetBehaviorTime(MonsterBehavior behavior) = 0;

protected:
	MonsterType			m_monster_type;
	FLOAT				m_attack_range;
	FLOAT				m_boundary_range;
	INT					m_target_id;
	USHORT				m_current_animation;
	MonsterBehavior		m_current_behavior;
	BYTE				m_aggro_level;
	BYTE				m_last_behavior_id;
	FLOAT               m_highest_damage;

	std::vector<INT>    m_pl_random_id;

	typedef std::pair<INT, FLOAT> m_save_damage_pair;
	std::vector<m_save_damage_pair> m_pl_save_damage;
};

class WarriorMonster : public Monster
{
public:
	WarriorMonster();
	virtual ~WarriorMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;

private:
	virtual bool CanSwapAttackBehavior() override;
	virtual MonsterBehavior SetNextBehavior(MonsterBehavior behavior) override;
	virtual void SetBehaviorAnimation(MonsterBehavior behavior) override;
	virtual std::chrono::milliseconds SetBehaviorTime(MonsterBehavior behavior) override;
};

class ArcherMonster : public Monster
{
public:
	ArcherMonster();
	virtual ~ArcherMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;
	
	virtual void StepBack(FLOAT elapsed_time) override;
	virtual void Flee(FLOAT elapsed_time) override;

private:
	virtual bool CanSwapAttackBehavior() override;
	virtual MonsterBehavior SetNextBehavior(MonsterBehavior behavior) override;
	virtual void SetBehaviorAnimation(MonsterBehavior behavior) override;
	virtual std::chrono::milliseconds SetBehaviorTime(MonsterBehavior behavior) override;
	bool DoesAttack();
	void SetFleeDirection();

private:
	FLOAT	m_max_step_back_time;
	FLOAT	m_step_back_time;
	FLOAT	m_recover_attack_range;

	XMFLOAT3 m_flee_direction;
};

class WizardMonster : public Monster
{
public:
	WizardMonster();
	virtual ~WizardMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;

private:
	virtual bool CanSwapAttackBehavior() override;
	virtual MonsterBehavior SetNextBehavior(MonsterBehavior behavior) override;
	virtual void SetBehaviorAnimation(MonsterBehavior behavior) override;
	virtual std::chrono::milliseconds SetBehaviorTime(MonsterBehavior behavior) override;
};

class BossMonster : public Monster
{
public:
	BossMonster();
	virtual ~BossMonster() = default;

	virtual void Init() override;
	virtual void Update(FLOAT elapsed_time) override;
	virtual void DecreaseHp(FLOAT damage, INT id) override;

private:
	virtual bool CanSwapAttackBehavior() override;
	virtual MonsterBehavior SetNextBehavior(MonsterBehavior behavior) override;
	virtual void SetBehaviorAnimation(MonsterBehavior behavior) override;
	virtual std::chrono::milliseconds SetBehaviorTime(MonsterBehavior behavior) override;
	void RandomTarget(FLOAT elapsed_time);
	void PlayerHighestDamageTarget();

	INT    m_behavior_cnt = 0;   
	INT    m_enhance_behavior_cnt = 0;
};

