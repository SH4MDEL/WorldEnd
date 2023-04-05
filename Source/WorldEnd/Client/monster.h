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
	void SetHpBar(const shared_ptr<HpBar>& hpBar) override { m_hpBar = hpBar; }
	void SetVelocity(XMFLOAT3& velocity);
	void SetType(MonsterType type) { m_type = type; }
	void SetIsShowing(bool isShowing);

	FLOAT GetMaxHp() const override { return m_maxHp; }
	FLOAT GetHp() const override { return m_hp; }
	MonsterType GetType() const { return m_type; }
	bool GetIsShowing() const { return m_isShowing; }



private:
	FLOAT				m_hp;			// ü��
	FLOAT				m_maxHp;		// �ִ� ü��
	shared_ptr<HpBar>	m_hpBar;		// HP��

	MonsterType			m_type;
	bool				m_isShowing;
};

