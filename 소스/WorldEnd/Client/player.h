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

	void Update(FLOAT timeElapsed) override;
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw) override;

	void ApplyFriction(FLOAT deltaTime);

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	FLOAT GetHp() const { return m_hp; }

	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void AddVelocity(const XMFLOAT3& increase);
	void SetHp(FLOAT hp) { m_hp = hp; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetHpBar(const shared_ptr<HpBar>& hpBar) { m_hpBar = hpBar; }
	void SetHpBarPosition(const XMFLOAT3& position) { if (m_hpBar) m_hpBar->SetPosition(position); }
	
	// �߰�
	void SetID(INT id) { m_id = id; }
	INT GetID() const { return m_id; }

private:
	XMFLOAT3			m_velocity;		// �ӵ�
	FLOAT				m_maxVelocity;	// �ִ�ӵ�
	FLOAT				m_friction;		// ������

	FLOAT				m_hp;			// �÷��̾� ü��
	FLOAT				m_maxHp;		// �÷��̾� �ִ� ü��

	shared_ptr<Camera>	m_camera;		// ī�޶�
	shared_ptr<HpBar>	m_hpBar;		// HP��

	INT					m_id;				// �÷��̾� ���� ���̵�
};
