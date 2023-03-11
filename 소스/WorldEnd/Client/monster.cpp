#include "monster.h"

Monster::Monster() : m_hp{ 200.f }, m_maxHp{ 200.f }, m_id{ -1 }
{

}

void Monster::Update(FLOAT timeElapsed)
{
	
}

void Monster::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	m_yaw += yaw;

	GameObject::Rotate(0.f, 0.f, yaw);
}

void Monster::SetPosition(const XMFLOAT3& position)
{
	XMFLOAT3 right = GetRight();

	GameObject::SetPosition(Vector3::Add(position, Vector3::Mul(right, 2.5)));

	if (m_hpBar) {
		XMFLOAT3 hpBarPosition = position;
		hpBarPosition.y += 2.3f;
		m_hpBar->SetPosition(hpBarPosition);
	}
}

void Monster::SetHp(FLOAT hp)
{
	m_hp = hp;
	if (m_hp <= 0)
		m_hp = 0;
	// 사망처리 필요함

	if (m_hpBar) {
		m_hpBar->SetMaxHp(m_maxHp);
		m_hpBar->SetHp(m_hp);
	}
}

void Monster::SetVelocity(XMFLOAT3& velocity)
{

}
