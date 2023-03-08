#include "object.h"


void GameObject::SetPosition(const XMFLOAT3& pos)
{
	m_position = move(pos);
}

void GameObject::SetRotation(const FLOAT& yaw)
{
	m_yaw = yaw;
}

void GameObject::SetBoundingBox(const BoundingOrientedBox& obb)
{
	m_bounding_box = obb;
}