#pragma once
#include "stdafx.h"

class Monster abstract
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

public:

	virtual void CreateMonster() {};

	void SetId(char id);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetMonsterPosition();

	CHAR GetId() const;
	DirectX::XMFLOAT3 GetPosition() const;
	MonsterData GetData() const;
};

class WarriorMonster : public Monster
{
public:
	WarriorMonster();
	~WarriorMonster() = default;

	virtual void CreateMonster();

};

class ArcherMonster : public Monster
{
public:
	ArcherMonster();
	~ArcherMonster() = default;

	virtual void CreateMonster();

};

class WizsadMonster : public Monster
{
public:
	WizsadMonster();
	~WizsadMonster() = default;

	virtual void CreateMonster();

};

