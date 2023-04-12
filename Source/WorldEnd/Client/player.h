#pragma once
#include "stdafx.h"
#include "object.h"
#include "ui.h"


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
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetHp(FLOAT hp);
	void SetStamina(FLOAT stamina);
	void SetHpBar(const shared_ptr<GaugeBar>& hpBar) override { m_hpBar = hpBar; }
	void SetStaminaBar(const shared_ptr<GaugeBar>& staminaBar) { m_staminaBar = staminaBar; }
	void SetSkillGauge(const shared_ptr<VertGaugeUI>& skillGauge) { m_skillGauge = skillGauge; }
	void SetUltimateGauge(const shared_ptr<VertGaugeUI>& ultimateGauge) { m_ultimateGauge = ultimateGauge; }
	void SetType(PlayerType type) { m_type = type; }
	void SetInteractable(bool value) { m_interactable = value; }
	void SetInteractableType(InteractableType type) { m_interactableType = type; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	FLOAT GetHp() const { return m_hp; }
	FLOAT GetMaxHp() const { return m_maxHp; }
	FLOAT GetStamina() const { return m_stamina; }
	FLOAT GetMaxStamina() const { return m_maxStamina; }
	PlayerType GetType() const { return m_type; }
	InteractableType GetInteractableType() const { return m_interactableType; }	

	void ResetCooltime(char type);
	void ResetAllCooltime();
	virtual bool ChangeAnimation(USHORT animation) override;
	void ChangeAnimation(USHORT animation, bool other);

	// 추가
	void SetID(INT id) { m_id = id; }
	INT GetID() const { return m_id; }

	void CreateMovePacket();
	void CreateCooltimePacket(CooltimeType type);
	void CreateAttackPacket(const XMFLOAT3& pos, AttackType attackType,
		CollisionType collisionType, chrono::system_clock::time_point eventTime,
		CooltimeType cooltimeType);
	void CreateInteractPacket();
	void CreateChangeStaminaPacket(bool value);
	void SetBuffer(void* mess, size_t size);
	void SendPacket();

private:
	XMFLOAT3				m_velocity;		// 속도
	FLOAT					m_maxVelocity;	// 최대속도
	FLOAT					m_friction;		// 마찰력

	FLOAT					m_hp;			// 플레이어 체력
	FLOAT					m_maxHp;		// 플레이어 최대 체력
	FLOAT					m_stamina;
	FLOAT					m_maxStamina;

	FLOAT					m_skillCool;
	FLOAT					m_ultimateCool;

	shared_ptr<Camera>		m_camera;		// 카메라
	shared_ptr<GaugeBar>	m_hpBar;		// HP바
	shared_ptr<GaugeBar>	m_staminaBar;	// 스테미너 바

	shared_ptr<VertGaugeUI>	m_skillGauge;	// 스킬 쿨타임 게이지
	shared_ptr<VertGaugeUI>	m_ultimateGauge;// 궁극기 쿨타임 게이지

	INT						m_id;			// 플레이어 고유 아이디

	PlayerType				m_type = PlayerType::WARRIOR;

	array<bool, CooltimeType::COUNT> m_cooltimeList;	// 쿨타임이면 true, 쿨타임중이 아니면 false
	
	chrono::system_clock::time_point	m_startDash;
	bool								m_dashed;
	FLOAT								m_moveSpeed;

	bool				m_interactable;
	InteractableType	m_interactableType;

	CHAR				m_sendBuffer[BUFSIZ];
	int					m_bufSize;
};
