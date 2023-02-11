#pragma once
#include "stdafx.h"

class Monster abstract
{
protected:
	CHAR							m_id;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT							m_yaw;

	INT								m_hp;			// 체력
	INT								m_damage;		// 데미지
	FLOAT							m_speed;		// 이동속도 

public:

	void SetPosition(const DirectX::XMFLOAT3& position);\

	CHAR GetId() const;
	DirectX::XMFLOAT3 GetPosition() const;
	MonsterData GetData() const;
};

