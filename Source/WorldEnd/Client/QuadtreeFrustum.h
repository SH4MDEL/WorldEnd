#pragma once
#include "stdafx.h"
#include "object.h"

class QuadtreeFrustum
{
public:
	QuadtreeFrustum();
	QuadtreeFrustum(XMFLOAT3 position, XMFLOAT3 extent, INT depth);
	virtual ~QuadtreeFrustum() = default;

	virtual void SetGameObject(const shared_ptr<GameObject>& gameObject);
	virtual unordered_set<shared_ptr<GameObject>> GetGameObjects(const BoundingFrustum& viewFrustum);
	virtual unordered_set<shared_ptr<GameObject>> GetGameObjects(const BoundingOrientedBox& boundingBox);

	virtual void Clear();

protected:
	BoundingBox								m_boundingBox;
	array<shared_ptr<QuadtreeFrustum>, 4>	m_children;

	INT										m_depth;
};

class LeafQuadtree : public QuadtreeFrustum
{
public:
	LeafQuadtree(XMFLOAT3 position, XMFLOAT3 extent, INT depth);
	virtual ~LeafQuadtree() = default;

	void SetGameObject(const shared_ptr<GameObject>& gameObject) override;
	unordered_set<shared_ptr<GameObject>> GetGameObjects(const BoundingFrustum& viewFrustum) override;
	unordered_set<shared_ptr<GameObject>> GetGameObjects(const BoundingOrientedBox& boundingBox) override;

	void Clear() override;

private:
	vector<shared_ptr<GameObject>> m_objects;
};