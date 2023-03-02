#pragma once
#include "stdafx.h"
#include "object.h"

class Monster : public GameObject
{
public:
	Monster();
	~Monster() = default;

	void Update(FLOAT timeElapsed) override;
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw) override;

	void SetPosition(const XMFLOAT3& position) override;
	void SetHp(FLOAT hp) { m_hp = hp; }
	void SetHpBar(const shared_ptr<HpBar>& hpBar) { m_hpBar = hpBar; }

	FLOAT GetHp() const { return m_hp; }

	void SetID(INT id) { m_id = id; }
	INT GetID() const { return m_id; }

private:
	FLOAT				m_hp;			// ü��
	FLOAT				m_maxHp;		// �ִ� ü��
	shared_ptr<HpBar>	m_hpBar;		// HP��

	INT					m_id;			// ���� ���̵�
};

