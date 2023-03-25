#pragma once
#include "stdafx.h"
#include "object.h"

class Monster : public AnimationObject
{
public:
	Monster();
	~Monster() = default;

	void Update(FLOAT timeElapsed) override;
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw) override;

	void SetPosition(const XMFLOAT3& position) override;
	void SetHp(FLOAT hp);
	void SetHpBar(const shared_ptr<HpBar>& hpBar) { m_hpBar = hpBar; }
	void SetVelocity(XMFLOAT3& velocity);



	MonsterType GetType() const { return m_type; }
	void SetType(MonsterType type) { m_type = type; }

private:
	FLOAT				m_hp;			// ü��
	FLOAT				m_maxHp;		// �ִ� ü��
	shared_ptr<HpBar>	m_hpBar;		// HP��

	MonsterType			m_type;
};

