#pragma once
#include "stdafx.h"

class Session;

class GameObject
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;
	
	virtual void CheckCollision(Session& player) {}
	virtual void CheckCollision(GameObject& object) {}

	void SetPosition(const XMFLOAT3& pos);
	void SetRotation(const FLOAT& yaw);
	void SetBoundingBox(const BoundingOrientedBox& obb);

	BoundingOrientedBox GetBoundingBox() const { return m_bounding_box; }

protected:
	XMFLOAT3				m_position;
	FLOAT					m_yaw;
	BoundingOrientedBox		m_bounding_box;
};

class NPC : public GameObject
{
public:
	NPC() = default;
	virtual ~NPC() = default;

private:

};
