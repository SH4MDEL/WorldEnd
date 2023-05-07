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
	void SetMaxHp(FLOAT maxHp);
	void SetHp(FLOAT hp);
	void SetHpBar(const shared_ptr<GaugeBar>& hpBar) override { m_hpBar = hpBar; }
	void SetVelocity(XMFLOAT3& velocity);
	void SetType(MonsterType type) { m_type = type; }

	FLOAT GetMaxHp() const override { return m_maxHp; }
	FLOAT GetHp() const override { return m_hp; }
	MonsterType GetType() const { return m_type; }

	virtual void ChangeAnimation(USHORT animation, bool doSend) override;

private:
	FLOAT				m_hp;			// 체력
	FLOAT				m_maxHp;		// 최대 체력
	shared_ptr<GaugeBar>	m_hpBar;		// HP바

	MonsterType			m_type;
};

