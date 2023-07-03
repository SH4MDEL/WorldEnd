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
	void SetEnable(bool value) { m_enable = value; }

	FLOAT GetMaxHp() const override { return m_maxHp; }
	FLOAT GetHp() const override { return m_hp; }
	MonsterType GetType() const { return m_type; }
	bool GetEnable() const { return m_enable; }

	virtual void ChangeAnimation(USHORT animation, bool doSend) override;

private:
	FLOAT				m_hp;			// 체력
	FLOAT				m_maxHp;		// 최대 체력
	shared_ptr<GaugeBar>	m_hpBar;		// HP바

	MonsterType			m_type;
	bool				m_enable;
};

class MonsterMagicCircle : public GameObject
{
public:
	MonsterMagicCircle();
	~MonsterMagicCircle() = default;

	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	BOOL IsEnable() { return m_enable; }

	void SetEnable() { m_enable = true; }
	void SetDisable() { m_enable = false; }

private:
	BOOL		m_enable;

	const FLOAT	m_lifeTime;
	FLOAT		m_age;
};