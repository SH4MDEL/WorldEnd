#pragma once
#include "stdafx.h"

class Monster
{
protected:
	CHAR							m_id;
	MonsterType                     m_monster_type;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT							m_yaw;

	INT								m_hp;			
	INT								m_damage;		
	FLOAT							m_speed;	

	UCHAR                           m_target_id;

	BoundingOrientedBox				m_bounding_box;

	INT								m_current_animation;

	void MonsterStateUpdate(XMVECTOR& look, float taketime);
	XMVECTOR GetPlayerVector(UCHAR pl_id);

public:

	Monster();
	virtual ~Monster() = default;

	virtual void Update(float taketime) = 0;

	virtual void CreateMonster(float taketime) {};

	void SetId(char id);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetMonsterPosition();
	void SetTargetId(char id);

	CHAR GetId() const;
	DirectX::XMFLOAT3 GetPosition();
	MonsterData GetData() const;
	UCHAR GetTargetId() const;
	MonsterType GetType() const;
	BoundingOrientedBox GetBoundingBox() const;

	void DecreaseHp(INT damage);

	bool ChangeAnimation(int animation);
};

class WarriorMonster : public Monster
{
private:
	void TargetUpdate();

public:
	WarriorMonster();
	~WarriorMonster() = default;

	virtual void CreateMonster(float taketime);

	virtual void Update(float taketime) override;
};

class ArcherMonster : public Monster
{
private:
	void TargetUpdate();

public:
	ArcherMonster();
	~ArcherMonster() = default;

	virtual void CreateMonster(float take_time);

	virtual void Update(float taketime) override;
};

class WizsadMonster : public Monster
{
private:
	void TargetUpdate();

public:
	WizsadMonster();
	~WizsadMonster() = default;

	virtual void CreateMonster(float taketime);

	virtual void Update(float taketime) override;
};

