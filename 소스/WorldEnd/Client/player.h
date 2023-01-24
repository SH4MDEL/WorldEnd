#pragma once
#include "stdafx.h"
#include "object.h"

class Camera;

class Player : public GameObject
{
public:
	Player();
	~Player() = default;

	void OnProcessingKeyboardMessage(FLOAT timeElapsed);

	virtual void Update(FLOAT timeElapsed);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void ApplyFriction(FLOAT deltaTime);

	XMFLOAT3 GetVelocity() const { return m_velocity; }

	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void AddVelocity(const XMFLOAT3& increase);
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	
	// �߰�
	void SetId(INT id) { m_id = id; }
	INT GetId() const;

private:
	XMFLOAT3						m_velocity;		// �ӵ�
	FLOAT							m_maxVelocity;	// �ִ�ӵ�
	FLOAT							m_friction;		// ������

	shared_ptr<Camera>				m_camera;		// ī�޶�

	// �÷��̾ id�� �����ϱ� ���� �߰���.
	INT					m_id;				// �÷��̾� ���� ���̵�
	BOOL				m_is_multi_player;	// ��Ƽ�÷��̾� ����
};
