#include "monster.h"

Monster::Monster() : m_hp{ 100.f }, m_maxHp{ 100.f }, m_id{ -1 }
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