#pragma once
#include "stdafx.h"
#include "object.h"
#include "ui.h"


class Camera;

class Player : public AnimationObject
{
public:
	Player();
	virtual ~Player() override;

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
	void SetInteractableType(InteractionType type) { m_interactableType = type; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	FLOAT GetHp() const { return m_hp; }
	FLOAT GetMaxHp() const { return m_maxHp; }
	FLOAT GetStamina() const { return m_stamina; }
	FLOAT GetMaxStamina() const { return m_maxStamina; }
	PlayerType GetType() const { return m_type; }
	InteractionType GetInteractableType() const { return m_interactableType; }	

	void ResetCooldown(char type);
	void ResetAllCooldown();
	virtual void ChangeAnimation(USHORT animation, bool doSend) override;

	// 추가
	void MoveOnStairs();

	void SetId(INT id) { m_id = id; }
	INT GetId() const { return m_id; }

	void CreateMovePacket();
	void CreateCooldownPacket(ActionType type);
	void CreateAttackPacket(ActionType cooldownType);
	void CreateInteractPacket();
	void CreateChangeAnimation(USHORT animation);
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

	array<bool, ActionType::COUNT> m_cooldownList;	// 쿨타임이면 true, 쿨타임중이 아니면 false
	
	chrono::system_clock::time_point	m_startDash;
	bool								m_dashed;
	FLOAT								m_moveSpeed;

	bool				m_interactable;
	InteractionType		m_interactableType;

	CHAR				m_sendBuffer[BUFSIZ];
	int					m_bufSize;
};

class Arrow : public GameObject
{
public:
	Arrow();
	~Arrow() = default;

	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void Reset();

	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	XMFLOAT3 GetVelocity() const { return m_velocity; }

	void SetVelocity(XMFLOAT3 velocity) { m_velocity = velocity; }
	void SetEnable() { m_enable = true; }
	void SetDisable() { m_enable = false; }

private:
	BOOL		m_enable;
	XMFLOAT3	m_velocity;
};

constexpr INT MAX_ARROWRAIN_ARROWS = 30;
constexpr FLOAT ARROW_LIFECYCLE = 0.8f;
constexpr FLOAT MAX_ARROW_HEIGHT = 4.f;
class InstancingShader;
class ArrowRain : public GameObject
{
public:
	ArrowRain();
	~ArrowRain() override = default;

	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void RenderMagicCircle(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	BOOL IsEnable() { return m_enable; }

	void SetPosition(const XMFLOAT3& position) override;
	void SetEnable() { m_enable = true; }
	void SetDisable() { m_enable = false; }

	array<pair<shared_ptr<Arrow>, FLOAT>, MAX_ARROWRAIN_ARROWS>& GetArrows();

private:
	BOOL														m_enable;
	const FLOAT													m_lifeTime;
	FLOAT														m_age;

	array<pair<shared_ptr<Arrow>, FLOAT>, MAX_ARROWRAIN_ARROWS>	m_arrows;
	unique_ptr<GameObject>										m_magicCircle;
};