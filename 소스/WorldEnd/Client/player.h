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

	void SetPosition(const XMFLOAT3& position) override;
	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void AddVelocity(const XMFLOAT3& increase);
	void SetHp(FLOAT hp) { m_hp = hp; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetHpBar(const shared_ptr<HpBar>& hpBar) { m_hpBar = hpBar; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	FLOAT GetHp() const { return m_hp; }
	
	// 추가
	void SetID(INT id) { m_id = id; }
	void SetKey(CHAR key) { m_key = key; }
	INT GetID() const { return m_id; }

private:
	XMFLOAT3			m_velocity;		// 속도
	FLOAT				m_maxVelocity;	// 최대속도
	FLOAT				m_friction;		// 마찰력

	FLOAT				m_hp;			// 플레이어 체력
	FLOAT				m_maxHp;		// 플레이어 최대 체력

	shared_ptr<Camera>	m_camera;		// 카메라
	shared_ptr<HpBar>	m_hpBar;		// HP바

	INT					m_id;				// 플레이어 고유 아이디
	CHAR                m_key = INPUT_KEY_E;
};

class AttackCallbackHandler : public AnimationCallbackHandler
{
public:
	AttackCallbackHandler() = default;
	~AttackCallbackHandler() = default;

	virtual void Callback(void* callbackData, float trackPosition);
};