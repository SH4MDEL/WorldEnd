#pragma once
#include "stdafx.h"

class Monster abstract
{
protected:
	CHAR							m_id;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT							m_yaw;

	INT								m_hp;			// ü��
	INT								m_damage;		// ������
	FLOAT							m_speed;		// �̵��ӵ� 

public:

	void SetPosition(const DirectX::XMFLOAT3& position);\

	CHAR GetId() const;
	DirectX::XMFLOAT3 GetPosition() const;
	MonsterData GetData() const;
};

