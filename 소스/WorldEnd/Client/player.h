#pragma once
#include "stdafx.h"
#include "object.h"

#define ROLL_MAX +20
#define ROLL_MIN -10

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
	FLOAT GetHp() const { return m_hp; }

	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void AddVelocity(const XMFLOAT3& increase);
	void SetHp(FLOAT hp) { m_hp = hp; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetHpBar(const shared_ptr<HpBar>& hpBar) { m_hpBar = hpBar; }
	void SetHpBarPosition(const XMFLOAT3& position) { if (m_hpBar) m_hpBar->SetPosition(position); }

	
	// 추가
	void SetID(INT id) { m_id = id; }
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
};

class AttackCallbackHandler : public AnimationCallbackHandler
{
public:
	AttackCallbackHandler() = default;
	~AttackCallbackHandler() = default;

	virtual void Callback(void* callbackData, float trackPosition);
};