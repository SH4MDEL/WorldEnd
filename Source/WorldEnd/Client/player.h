﻿#pragma once
#include "stdafx.h"
#include "object.h"


class Camera;

class Player : public AnimationObject
{
public:
	Player();
	~Player();

	void OnProcessingKeyboardMessage(FLOAT timeElapsed);
	void OnProcessingMouseMessage(UINT message, LPARAM lParam);

	void Update(FLOAT timeElapsed) override;
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw) override;

	void ApplyFriction(FLOAT deltaTime);

	void SetPosition(const XMFLOAT3& position) override;
	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void AddVelocity(const XMFLOAT3& increase);
	void SetHp(FLOAT hp);
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetHpBar(const shared_ptr<GaugeBar>& hpBar) override { m_hpBar = hpBar; }
	void SetStaminaBar(const shared_ptr<GaugeBar>& staminaBar) { m_staminaBar = staminaBar; }
	void SetType(PlayerType type) { m_type = type; }
	void SetStamina(FLOAT stamina) { m_stamina = stamina; }
	void SetInteractable(bool value) { m_interactable = value; }
	void SetInteractableType(InteractableType type) { m_interactableType = type; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	FLOAT GetHp() const { return m_hp; }
	FLOAT GetMaxHp() const { return m_maxHp; }
	PlayerType GetType() const { return m_type; }
	InteractableType GetInteractableType() const { return m_interactableType; }

	void ResetCooltime(CooltimeType type);
	virtual bool ChangeAnimation(int animation) override;
	void ChangeAnimation(int animation, bool other);

	// 추가
	void SetID(INT id) { m_id = id; }
	INT GetID() const { return m_id; }

	void SendCooltimePacket(CooltimeType type);
	void SendAttackPacket(const XMFLOAT3& pos, AttackType attackType,
		CollisionType collisionType, chrono::system_clock::time_point eventTime,
		CooltimeType cooltimeType);
	void SendInteractPacket();

private:
	XMFLOAT3				m_velocity;		// 속도
	FLOAT					m_maxVelocity;	// 최대속도
	FLOAT					m_friction;		// 마찰력

	FLOAT					m_hp;			// 플레이어 체력
	FLOAT					m_maxHp;		// 플레이어 최대 체력

	shared_ptr<Camera>		m_camera;		// 카메라
	shared_ptr<GaugeBar>	m_hpBar;		// HP바
	shared_ptr<GaugeBar>	m_staminaBar;	// 스테미너 바

	INT						m_id;			// 플레이어 고유 아이디

	PlayerType				m_type = PlayerType::WARRIOR;

	array<bool, CooltimeType::COUNT> m_cooltimeList;	// 쿨타임이면 true, 쿨타임중이 아니면 false
	
	chrono::system_clock::time_point	m_startDash;
	bool								m_dashed;
	FLOAT								m_moveSpeed;
	FLOAT								m_stamina;

	bool				m_interactable;
	InteractableType	m_interactableType;
};

class AttackCallbackHandler : public AnimationCallbackHandler
{
public:
	AttackCallbackHandler() = default;
	~AttackCallbackHandler() = default;

	virtual void Callback(void* callbackData, float trackPosition);
};