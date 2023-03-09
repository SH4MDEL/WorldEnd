#pragma once
#include "stdafx.h"

class Monster abstract
{
protected:
	CHAR							m_id;
	MonsterType                     m_monster_type;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT			                m_yaw;

	INT								m_hp;			
	INT								m_damage;		
	FLOAT							m_speed;	

	UCHAR                           m_target_id;

	XMFLOAT4X4                      m_worldmatrix;

	void MonsterStateUpdate(XMVECTOR& look, float taketime);
	void MonsterRotateUpdate(const XMVECTOR& look);
	XMVECTOR GetPlayerVector(UCHAR pl_id);

public:

	virtual void CreateMonster(float taketime) {};

	void SetId(char id);
	void SetPosition(const XMFLOAT3& position);
	void SetMonsterPosition();
	void SetTargetId(char id);
	void SetHp(int hp);

	CHAR GetId() const;
	DirectX::XMFLOAT3 GetPosition();
	MonsterData GetData() const;
	UCHAR GetTargetId() const;
	MonsterType GetType() const;
 
};

class WarriorMonster : public Monster
{
private:
	void TargetUpdate();

public:
	WarriorMonster();
	~WarriorMonster() = default;

	virtual void CreateMonster(float taketime);

};

class ArcherMonster : public Monster
{
private:
	void TargetUpdate();

public:
	ArcherMonster();
	~ArcherMonster() = default;

	virtual void CreateMonster(float taketime);

};

class WizsadMonster : public Monster
{
private:
	void TargetUpdate();

public:
	WizsadMonster();
	~WizsadMonster() = default;

	virtual void CreateMonster(float taketime);

};

