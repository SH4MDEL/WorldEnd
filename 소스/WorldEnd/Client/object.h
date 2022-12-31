#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"

class GameObject
{
public:
	GameObject();
	~GameObject();

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void UpdateTransform(XMFLOAT4X4* parentMatrix = nullptr);

	void SetMesh(const shared_ptr<Mesh>& mesh);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetMaterial(const shared_ptr<Material>& material);

	void SetPosition(const XMFLOAT3& position);
	void SetScale(FLOAT x, FLOAT y, FLOAT z);

	XMFLOAT4X4 GetWorldMatrix() const { return m_worldMatrix; }
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetRight() const { return m_right; }
	XMFLOAT3 GetUp() const { return m_up; }
	XMFLOAT3 GetFront() const { return m_front; }

	virtual void ReleaseUploadBuffer() const;

	void SetChild(const shared_ptr<GameObject>& child);
	shared_ptr<GameObject> FindFrame(string frameName);

	void LoadGeometry(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const wstring& fileName);
	void LoadFrameHierarchy(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in);

	void UpdateBoundingBox();
	void SetBoundingBox(const BoundingOrientedBox& boundingBox);

	//XMFLOAT3 GetPositionX(return XMFLOAT3{ m_worldMatrix._41 };)

	
protected:
	XMFLOAT4X4					m_transformMatrix;
	XMFLOAT4X4					m_worldMatrix;
	

	XMFLOAT3					m_right;		// ���� x��
	XMFLOAT3					m_up;			// ���� y��
	XMFLOAT3					m_front;		// ���� z��

	FLOAT						m_roll;			// x�� ȸ����
	FLOAT						m_pitch;		// y�� ȸ����
	FLOAT						m_yaw;			// z�� ȸ����

	shared_ptr<Mesh>			m_mesh;			// �޽�
	shared_ptr<Texture>			m_texture;		// �ؽ�ó
	shared_ptr<Material>		m_material;		// ����

	string						m_frameName;	// ���� �������� �̸�
	shared_ptr<GameObject>		m_parent;
	shared_ptr<GameObject>		m_sibling;
	shared_ptr<GameObject>		m_child;

	BoundingOrientedBox			m_boundingBox;	
};

class Field : public GameObject
{
public:
	Field(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT width, INT length, INT height, INT blockWidth, INT blockLength, INT blockHeight, XMFLOAT3 scale);
	~Field() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPosition(const XMFLOAT3& position);

	XMFLOAT3 GetPosition() const { return m_blocks.front()->GetPosition(); }
	INT GetWidth() const { return m_width; }
	INT GetLength() const { return m_length; }
	XMFLOAT3 GetScale() const { return m_scale; }

	void ReleaseUploadBuffer() const override;

private:
	unique_ptr<FieldMapImage>		m_fieldMapImage;	// ���̸� �̹���
	vector<unique_ptr<GameObject>>	m_blocks;			// ��ϵ�
	INT								m_width;			// �̹����� ���� ����
	INT								m_length;			// �̹����� ���� ����
	INT								m_height;
	XMFLOAT3						m_scale;			// Ȯ�� ����
};

class Fence : public GameObject
{
public:
	Fence(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		INT width, INT length, INT blockWidth, INT blockLength);
	~Fence() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>&commandList) const;
	virtual void Move(const XMFLOAT3 & shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPosition(const XMFLOAT3 & position);

	XMFLOAT3 GetPosition() const { return m_blocks.front()->GetPosition(); }
	INT GetWidth() const { return m_width; }
	INT GetLength() const { return m_length; }

	void ReleaseUploadBuffer() const override;

private:
	vector<unique_ptr<GameObject>>	m_blocks;			// ��ϵ�
	INT								m_width;			// �̹����� ���� ����
	INT								m_length;			// �̹����� ���� ����
};


class Skybox : public GameObject
{
public:
	Skybox(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		FLOAT width, FLOAT length, FLOAT depth);
	~Skybox() = default;

	virtual void Update(FLOAT timeElapsed);
};