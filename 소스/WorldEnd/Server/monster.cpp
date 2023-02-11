#include "monster.h"

void Monster::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_position = position;
}




CHAR Monster::GetId() const
{
	return m_id;
}

DirectX::XMFLOAT3 Monster::GetPosition() const
{
	return m_position;
}

MonsterData Monster::GetData() const
{
	return MonsterData{ m_id, m_position, m_velocity, m_yaw };
}


