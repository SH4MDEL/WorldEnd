#include "monster.h"

Monster::Monster() : m_hp{ 200.f }, m_maxHp{ 200.f }, m_isShowing{ false }
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
	GameObject::SetPosition(position);

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

	if (m_hpBar) {
		m_hpBar->SetMaxGauge(m_maxHp);
		m_hpBar->SetGauge(m_hp);
	}
}

void Monster::SetVelocity(XMFLOAT3& velocity)
{

}

void Monster::SetIsShowing(bool isShowing)
{
	m_isShowing = isShowing;
}




