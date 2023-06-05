#include "QuadtreeFrustum.h"

QuadtreeFrustum::QuadtreeFrustum() : m_depth{ 0 } 
{

}

QuadtreeFrustum::QuadtreeFrustum(XMFLOAT3 position, XMFLOAT3 extent, INT depth) :
	m_boundingBox{ position, extent }, m_depth{depth}
{
	const array<FLOAT, 4> dx = { -1.f, -1.f, 1.f, 1.f };
	const array<FLOAT, 4> dz = { -1.f, 1.f, -1.f, 1.f };
	// depth�� 1�̸� ���������� ���� ����Ʈ�� ����
	if (depth == 1) {
		for (int i = 0; auto & child : m_children) {
			child = make_shared<LeafQuadtree>(
				XMFLOAT3{ position.x + dx[i] * extent.x / 2, position.y, position.z + dz[i] * extent.z / 2 },
				XMFLOAT3{ extent.x / 2, extent.y, extent.z / 2 }, depth - 1);
			++i;
		}
	}
	// �ƴϸ� �ڽ� ����Ʈ�� ����
	else {
		for (int i = 0;  auto & child : m_children) {
			child = make_shared<QuadtreeFrustum>(
				XMFLOAT3{ position.x + dx[i] * extent.x / 2, position.y, position.z + dz[i] * extent.z / 2 },
				XMFLOAT3{ extent.x / 2, extent.y, extent.z / 2 }, depth - 1);
			++i;
		}
	}
}

void QuadtreeFrustum::SetGameObject(const shared_ptr<GameObject>& gameObject)
{
	// ���� ������ �ʴ� ������Ʈ��� �Ⱒ
	if (m_boundingBox.Contains(gameObject->GetBoundingBox()) != DISJOINT) {
		for (auto& child : m_children) {
			child->SetGameObject(gameObject);
		}
	}
}

unordered_set<shared_ptr<GameObject>> QuadtreeFrustum::GetGameObjects(const BoundingFrustum& viewFrustum)
{
	unordered_set<shared_ptr<GameObject>> objectList;
	if (m_boundingBox.Contains(viewFrustum) != DISJOINT) {
		for (auto& child : m_children) {
			for (const auto& object : child->GetGameObjects(viewFrustum)) {
				objectList.insert(object);
			}
		}
	}
	return objectList;
}


LeafQuadtree::LeafQuadtree(XMFLOAT3 position, XMFLOAT3 extent, INT depth) : QuadtreeFrustum()
{
	// �� �̻� �ڽ��� ���� ���� ���
	m_boundingBox.Center = position;
	m_boundingBox.Extents = extent;
}

void LeafQuadtree::SetGameObject(const shared_ptr<GameObject>& gameObject)
{
	if (m_boundingBox.Contains(gameObject->GetBoundingBox()) != DISJOINT) {
		m_objects.push_back(gameObject);
	}
}

unordered_set<shared_ptr<GameObject>> LeafQuadtree::GetGameObjects(const BoundingFrustum& viewFrustum)
{
	unordered_set<shared_ptr<GameObject>> objectList;
	for (const auto& object : m_objects) {
		if (viewFrustum.Contains(object->GetBoundingBox()) != DISJOINT) {
			objectList.insert(object);
		}
	}
	return objectList;
}
