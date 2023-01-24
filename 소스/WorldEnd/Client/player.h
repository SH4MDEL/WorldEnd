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
	
	// 추가
	void SetId(INT id) { m_id = id; }
	INT GetId() const;

private:
	XMFLOAT3						m_velocity;		// 속도
	FLOAT							m_maxVelocity;	// 최대속도
	FLOAT							m_friction;		// 마찰력

	shared_ptr<Camera>				m_camera;		// 카메라

	// 플레이어를 id로 구별하기 위해 추가함.
	INT					m_id;				// 플레이어 고유 아이디
	BOOL				m_is_multi_player;	// 멀티플레이어 여부
};
