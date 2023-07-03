#include "player.h"
#include "camera.h"
#include "particleSystem.h"
#include "objectManager.h"
#include "instancingShader.h"

Player::Player() : m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.5f }, 
	m_hp{ 100.f }, m_maxHp{ 100.f }, m_stamina{ PlayerSetting::MAX_STAMINA }, m_maxStamina{ PlayerSetting::MAX_STAMINA }, 
	m_skillCool{ static_cast<FLOAT>(PlayerSetting::SKILL_COOLDOWN[(INT)m_type].count())}, 
	m_ultimateCool{ static_cast<FLOAT>(PlayerSetting::ULTIMATE_COOLDOWN[(INT)m_type].count()) },
	m_id{ -1 }, m_cooldownList{ false, }, m_dashed{ false }, m_moveSpeed{ PlayerSetting::WALK_SPEED },
	m_interactable{ false }, m_interactableType{ InteractionType::NONE }, m_bufSize{ 0 }
{
}

Player::~Player()
{
	m_hpBar.reset();
}

void Player::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
	if (ObjectAnimation::DEATH == m_currentAnimation)
		return;

	if (GetAsyncKeyState('1') & 0x8000) {
		SetPosition(Vector3::Add(GetPosition(), { 0.f, 0.1f, 0.f }));
		cout << GetPosition();
	}
	if (GetAsyncKeyState('2') & 0x8000) {
		SetPosition(Vector3::Add(GetPosition(), { 0.f, -0.1f, 0.f }));
		cout << GetPosition();
	}

	if (GetAsyncKeyState('Q') & 0x8000) {
		if (!m_cooldownList[ActionType::ULTIMATE]) {
			m_ultimateCool = 0.f;
			ChangeAnimation(PlayerAnimation::ULTIMATE, true);

			XMFLOAT3 pos = Vector3::Add(Vector3::Mul(m_front, 0.8f), GetPosition());
			
			CreateAttackPacket(ActionType::ULTIMATE);

			SendPacket();
		}
	}

	// 궁극기 시전중이면 아래의 키보드 입력 X
	if (PlayerAnimation::ULTIMATE == m_currentAnimation) {
		return;
	}

	// 공격, 스킬, 궁극기, 구르기 중에는 이동 X
	if (PlayerAnimation::ATTACK != m_currentAnimation &&
		PlayerAnimation::SKILL != m_currentAnimation &&
		PlayerAnimation::ULTIMATE != m_currentAnimation &&
		PlayerAnimation::ROLL != m_currentAnimation)
	{
		XMFLOAT3 eye = m_camera->GetEye(); eye.y = 0.f;
		XMFLOAT3 direction{ Vector3::Normalize(Vector3::Sub(GetPosition(), eye)) };

		if (GetAsyncKeyState('W') && GetAsyncKeyState('A') & 0x8000) {
			XMFLOAT3 front{ Vector3::Normalize(Vector3::Sub(GetPosition(), eye)) };
			XMFLOAT3 left{ Vector3::Normalize(Vector3::Cross(direction, GetUp())) };
			XMFLOAT3 frontLeft{ Vector3::Normalize(Vector3::Add(front, left)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), frontLeft) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), frontLeft) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('W') && GetAsyncKeyState('D') & 0x8000) {
			XMFLOAT3 front{ Vector3::Normalize(Vector3::Sub(GetPosition(), eye)) };
			XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(GetUp(), direction)) };
			XMFLOAT3 frontRight{ Vector3::Normalize(Vector3::Add(front, right)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), frontRight) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), frontRight) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('S') && GetAsyncKeyState('A') & 0x8000) {
			XMFLOAT3 back{ Vector3::Normalize(Vector3::Sub(eye, GetPosition())) };
			XMFLOAT3 left{ Vector3::Normalize(Vector3::Cross(direction, GetUp())) };
			XMFLOAT3 backLeft{ Vector3::Normalize(Vector3::Add(back, left)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), backLeft) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), backLeft) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('S') && GetAsyncKeyState('D') & 0x8000) {
			XMFLOAT3 back{ Vector3::Normalize(Vector3::Sub(eye, GetPosition())) };
			XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(GetUp(), direction)) };
			XMFLOAT3 backRight{ Vector3::Normalize(Vector3::Add(back, right)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), backRight) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), backRight) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('W') & 0x8000) {
			XMFLOAT3 pos{ GetPosition() };
			pos.y = 0.f;
			XMFLOAT3 front{ Vector3::Normalize(Vector3::Sub(pos, eye)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), front) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), front) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('A') & 0x8000) {
			XMFLOAT3 left{ Vector3::Normalize(Vector3::Cross(direction, GetUp())) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), left) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), left) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('S') & 0x8000) {
			XMFLOAT3 pos{ GetPosition() };
			pos.y = 0.f;
			XMFLOAT3 back{ Vector3::Normalize(Vector3::Sub(eye, pos)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), back) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), back) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		else if (GetAsyncKeyState('D') & 0x8000) {
			XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(GetUp(), direction)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), right) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), right) };
			if (clockwise.y >= 0.f) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState('A') & 0x8000 ||
			GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('D') & 0x8000)
		{
			AddVelocity(Vector3::Mul(GetFront(), timeElapsed * m_moveSpeed));

			CreateMovePacket();

			SendPacket();
		}
	}

	if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
		if (!m_cooldownList[ActionType::ROLL]) {
			if (m_stamina >= PlayerSetting::MINIMUM_ROLL_STAMINA) {
				m_stamina -= PlayerSetting::ROLL_STAMINA_CHANGE_AMOUNT;
				SetStamina(m_stamina);

				ChangeAnimation(PlayerAnimation::ROLL, true);
				m_moveSpeed = PlayerSetting::ROLL_SPEED;

				CreateCooldownPacket(ActionType::ROLL);
				CreateChangeStaminaPacket(false);

				SendPacket();
			}
		}
	}
	
	if (GetAsyncKeyState('E') & 0x8000) {
		if (!m_cooldownList[ActionType::SKILL]) {
			m_skillCool = 0.f;
			ChangeAnimation(PlayerAnimation::SKILL, true);

			XMFLOAT3 pos = Vector3::Add(Vector3::Mul(m_front, 0.8f), GetPosition());
			CreateAttackPacket(ActionType::SKILL);

			SendPacket();
		}
	}

	if (GetAsyncKeyState('F') & 0x8000) {
		if (m_interactable) {
			CreateInteractPacket();
			SendPacket();
		}
	}

}

void Player::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{

	switch (message)
	{
	case WM_LBUTTONDOWN:
		if (ObjectAnimation::DEATH == m_currentAnimation)
			return;

		if (m_cooldownList[ActionType::NORMAL_ATTACK])
			return;

		ChangeAnimation(ObjectAnimation::ATTACK, true);
		CreateAttackPacket(ActionType::NORMAL_ATTACK);
		break;
	case WM_RBUTTONDOWN:
		if (ObjectAnimation::DEATH == m_currentAnimation)
			return;

		if (!m_dashed) {
			if (!m_cooldownList[ActionType::DASH]) {

				if (m_stamina >= PlayerSetting::MINIMUM_DASH_STAMINA) {
					m_dashed = true;
					ChangeAnimation(PlayerAnimation::DASH, true);
					m_moveSpeed = PlayerSetting::DASH_SPEED;
					m_startDash = chrono::system_clock::now();

					CreateCooldownPacket(ActionType::DASH);
					CreateChangeStaminaPacket(false);
				}
			}
		}
		break;
	case WM_RBUTTONUP:
		if (ObjectAnimation::DEATH == m_currentAnimation)
			return;

		if (m_dashed) {
			m_dashed = false;
			CreateChangeStaminaPacket(true);
		}
		break;
	default:
		break;
	}
	SendPacket();
}

void Player::Update(FLOAT timeElapsed)
{
	AnimationObject::Update(timeElapsed);

	// 애니메이션 상태 머신에 들어갈 내용
	// 상태 전환을 하는 함수를 작성하고 해당 함수를 호출하도록 해야 함
	if (m_animationController) {
		if (ObjectAnimation::DEATH == m_currentAnimation) {
			return;
		}

		if (ObjectAnimation::ATTACK == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			// 공격 애니메이션 종료 시 IDLE 로 변경
			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::IDLE, true);
				SendPacket();
			}
		}
		else if (PlayerAnimation::SKILL == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			// 스킬 애니메이션 종료 시 IDLE 로 변경
			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				if (m_dashed && (PlayerAnimation::RUN == m_prevAnimation))
					ChangeAnimation(PlayerAnimation::RUN, true);
				else
					ChangeAnimation(ObjectAnimation::IDLE, true);

				SendPacket();
			}
		}
		else if (PlayerAnimation::ULTIMATE == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			// 스킬 애니메이션 종료 시 IDLE 로 변경
			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				if (m_dashed && (PlayerAnimation::RUN == m_prevAnimation))
					ChangeAnimation(PlayerAnimation::RUN, true);
				else
					ChangeAnimation(ObjectAnimation::IDLE, true);

				SendPacket();
			}
		}
		else if (PlayerAnimation::ROLL == m_currentAnimation) {
			AddVelocity(Vector3::Mul(GetFront(), timeElapsed * m_moveSpeed));
			CreateMovePacket();

			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			if (fabs(track.GetPosition() - animation->GetLength()) <= 0.25f) {
				if (m_dashed && (PlayerAnimation::RUN == m_prevAnimation)) {
					ChangeAnimation(PlayerAnimation::RUN, true);
					m_moveSpeed = PlayerSetting::RUN_SPEED;
				}
				else {
					ChangeAnimation(ObjectAnimation::IDLE, true);
					m_moveSpeed = PlayerSetting::WALK_SPEED;
					
					CreateChangeStaminaPacket(true);
				}
			}
			SendPacket();
		}
		else if (PlayerAnimation::DASH == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			
			if (chrono::system_clock::now() > m_startDash + PlayerSetting::DASH_DURATION) {
				ChangeAnimation(ObjectAnimation::RUN, true);
				m_moveSpeed = PlayerSetting::RUN_SPEED;
				SendPacket();
			}
			else if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::WALK, true);
				m_moveSpeed = PlayerSetting::WALK_SPEED;
				SendPacket();
			}
		}
		else {
			float length = fabs(m_velocity.x) + fabs(m_velocity.z);
			if (length <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::IDLE, true);
				SendPacket();
			}
			else {
				if (m_dashed) {
					if (m_stamina <= numeric_limits<float>::epsilon()) {
						m_dashed = false;
						m_moveSpeed = PlayerSetting::WALK_SPEED;

						ChangeAnimation(ObjectAnimation::WALK, true);
						CreateChangeStaminaPacket(true);
						SendPacket();
					}
					else {
						ChangeAnimation(ObjectAnimation::RUN, true);
						SendPacket();
					}
				}
				else {
					ChangeAnimation(ObjectAnimation::WALK, true);
					m_moveSpeed = PlayerSetting::WALK_SPEED;
					SendPacket();
				}
			}
		}
	}

	if (m_hpBar) {
		XMFLOAT3 hpBarPosition = GetPosition();
		hpBarPosition.y += 1.8f;
		m_hpBar->SetPosition(hpBarPosition);
	}

	if (m_staminaBar) {
		XMFLOAT3 staminaBarPosition = GetPosition();
		XMFLOAT3 cameraRight = m_camera->GetRight();
		staminaBarPosition.x += cameraRight.x;
		staminaBarPosition.y += cameraRight.y;
		staminaBarPosition.z += cameraRight.z;
		staminaBarPosition.y += 1.f;
		m_staminaBar->SetPosition(staminaBarPosition);

		if (m_currentAnimation == PlayerAnimation::RUN || 
			m_currentAnimation == PlayerAnimation::DASH) {
			m_stamina -= PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT * timeElapsed;
			SetStamina(m_stamina);
		}
		else if (m_stamina < m_maxStamina) {
			m_stamina += PlayerSetting::DEFAULT_STAMINA_CHANGE_AMOUNT * timeElapsed;
			SetStamina(m_stamina);
		}
	}
	if (m_skillGauge) {
		if (PlayerSetting::SKILL_COOLDOWN[(INT)m_type].count() > m_skillCool) m_skillCool += timeElapsed;
		m_skillGauge->SetMaxGauge(static_cast<FLOAT>(PlayerSetting::SKILL_COOLDOWN[(INT)m_type].count()));
		m_skillGauge->SetGauge(m_skillCool);
	}
	if (m_ultimateGauge) {
		if (PlayerSetting::ULTIMATE_COOLDOWN[(INT)m_type].count() > m_ultimateCool) m_ultimateCool += timeElapsed;
		m_ultimateGauge->SetMaxGauge(static_cast<FLOAT>(PlayerSetting::ULTIMATE_COOLDOWN[(INT)m_type].count()));
		m_ultimateGauge->SetGauge(m_ultimateCool);
	}


//#ifndef USE_NETWORK
	Move(m_velocity);
//#endif // !USE_NETWORK

	//MoveOnStairs();

	ApplyFriction(timeElapsed);
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	m_yaw += yaw;

	GameObject::Rotate(0.f, 0.f, yaw);
}

void Player::ApplyFriction(FLOAT deltaTime)
{
	m_velocity = Vector3::Mul(m_velocity, 1 / m_friction * deltaTime);
}

void Player::SetPosition(const XMFLOAT3& position)
{
	GameObject::SetPosition(position);
	
	if (m_hpBar) {
		XMFLOAT3 hpBarPosition = position;
		hpBarPosition.y += 1.8f;
		m_hpBar->SetPosition(hpBarPosition);
	}
}

void Player::AddVelocity(const XMFLOAT3& increase)
{
	m_velocity = Vector3::Add(m_velocity, increase);

	FLOAT length{ Vector3::Length(m_velocity) };
	if (length > m_maxVelocity)
	{
		FLOAT ratio{ m_maxVelocity / length };
		m_velocity = Vector3::Mul(m_velocity, ratio);
	}
}

void Player::SetHp(FLOAT hp)
{
	m_hp = hp;
	if (m_hp <= 0)
		m_hp = 0;

	if (m_hpBar)
		m_hpBar->SetGauge(m_hp);
}

void Player::SetStamina(FLOAT stamina)
{
	m_stamina = stamina;
	if (m_stamina <= 0)
		m_stamina = 0;

	if (m_staminaBar)
		m_staminaBar->SetGauge(m_stamina);
}

void Player::ResetCooldown(char type)
{
	m_cooldownList[type] = false;
}

void Player::ResetAllCooldown()
{
	if (m_skillGauge) {
		m_skillCool = static_cast<FLOAT>(PlayerSetting::SKILL_COOLDOWN[(INT)m_type].count());
		m_skillGauge->SetMaxGauge(static_cast<FLOAT>(PlayerSetting::SKILL_COOLDOWN[(INT)m_type].count()));
		m_skillGauge->SetGauge(static_cast<FLOAT>(PlayerSetting::SKILL_COOLDOWN[(INT)m_type].count()));
	}
	if (m_ultimateGauge) {
		m_ultimateCool = static_cast<FLOAT>(PlayerSetting::ULTIMATE_COOLDOWN[(INT)m_type].count());
		m_ultimateGauge->SetMaxGauge(static_cast<FLOAT>(PlayerSetting::ULTIMATE_COOLDOWN[(INT)m_type].count()));
		m_ultimateGauge->SetGauge(static_cast<FLOAT>(PlayerSetting::ULTIMATE_COOLDOWN[(INT)m_type].count()));
	}
	for (size_t i = 0; i < ActionType::COUNT; ++i) {
		m_cooldownList[i] = false;
	}
}

void Player::ChangeAnimation(USHORT animation, bool doSend)
{
	if (m_currentAnimation == animation)
		return ;

	int start_num{};
	m_animationController->SetTrackSpeed(0, 1.f);
	switch (animation) {
	case ObjectAnimation::IDLE:
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;

	case ObjectAnimation::WALK:
		ChangeAnimationSettings(AnimationBlending::BLENDING, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		m_animationController->SetTrackSpeed(0, 1.7f);
		break;

	case ObjectAnimation::RUN:
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		m_animationController->SetTrackSpeed(0, 1.3f);
		break;

	case ObjectAnimation::ATTACK:
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;

	case ObjectAnimation::DEATH:
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;

	case PlayerAnimation::DASH:
		start_num = PlayerAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_LOOP,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case PlayerAnimation::SKILL:
		start_num = PlayerAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case PlayerAnimation::ULTIMATE:
		start_num = PlayerAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	case PlayerAnimation::ROLL:
		start_num = PlayerAnimation::ANIMATION_START;
		ChangeAnimationSettings(AnimationBlending::NORMAL, ANIMATION_TYPE_ONCE,
			ANIMATION_TYPE_LOOP, m_currentAnimation);
		break;
	}
	m_prevAnimation = m_currentAnimation;
	m_currentAnimation = animation;
	m_animationController->SetTrackAnimation(0, animation - start_num);

	if (doSend)
		CreateChangeAnimation(animation);
}

void Player::MoveOnStairs()
{
	XMFLOAT3 pos = GetPosition();
	float ratio{};
	if (pos.z >= RoomSetting::DOWNSIDE_STAIRS_BACK &&
		pos.z <= RoomSetting::DOWNSIDE_STAIRS_FRONT)
	{
		ratio = RoomSetting::DOWNSIDE_STAIRS_FRONT - pos.z;
		pos.y = RoomSetting::DEFAULT_HEIGHT -
			RoomSetting::DOWNSIDE_STAIRS_HEIGHT * ratio / 10.f;
	}
	else if (pos.z >= RoomSetting::TOPSIDE_STAIRS_BACK &&
		pos.z <= RoomSetting::TOPSIDE_STAIRS_FRONT)
	{
		ratio = pos.z - RoomSetting::TOPSIDE_STAIRS_BACK;
		pos.y = RoomSetting::DEFAULT_HEIGHT +
			RoomSetting::TOPSIDE_STAIRS_HEIGHT * ratio / 10.f;
	}
	else if (pos.z >= RoomSetting::DOWNSIDE_STAIRS_FRONT &&
		pos.z <= RoomSetting::TOPSIDE_STAIRS_BACK)
	{
		if (std::fabs(pos.y - RoomSetting::DEFAULT_HEIGHT) <= std::numeric_limits<float>::epsilon()) {
			return;
		}
		pos.y = RoomSetting::DEFAULT_HEIGHT;
	}
	SetPosition(pos);
}

void Player::CreateMovePacket()
{
#ifdef USE_NETWORK
	CS_PLAYER_MOVE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_PLAYER_MOVE;
	packet.pos = GetPosition();
	packet.velocity = GetVelocity();
	packet.yaw = GetYaw();
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateCooldownPacket(ActionType type)
{
#ifdef USE_NETWORK
	m_cooldownList[type] = true;

	CS_COOLDOWN_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_SET_COOLDOWN;
	packet.cooldown_type = type;
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateAttackPacket(ActionType attackType)
{
#ifdef USE_NETWORK
	m_cooldownList[attackType] = true;

	CS_ATTACK_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_ATTACK;
	packet.attack_type = attackType;
	packet.attack_time = chrono::system_clock::now();
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateInteractPacket()
{
#ifdef USE_NETWORK
	CS_INTERACT_OBJECT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_INTERACT_OBJECT;
	packet.interaction_type = m_interactableType;
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateChangeAnimation(USHORT animation)
{
#ifdef USE_NETWORK
	CS_CHANGE_ANIMATION_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CHANGE_ANIMATION;
	packet.animation = m_currentAnimation;
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateChangeStaminaPacket(bool value)
{
#ifdef USE_NETWORK
	CS_CHANGE_STAMINA_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CHANGE_STAMINA;
	packet.is_increase = value;
	SetBuffer(&packet, packet.size);
#endif
}

void Player::SetBuffer(void* mess, size_t size)
{
	char* c = reinterpret_cast<char*>(mess);
	memcpy((m_sendBuffer + m_bufSize), c, size);
	m_bufSize += static_cast<int>(size);
}

void Player::SendPacket()
{
#ifdef USE_NETWORK
	if (0 == m_bufSize)
		return;

	send(g_socket, m_sendBuffer, m_bufSize, 0);
	m_bufSize = 0;
#endif
}

Arrow::Arrow() : m_enable { false }, m_velocity { 0.f, 0.f, 0.f }
{
}

void Arrow::Update(FLOAT timeElapsed)
{
	if (m_enable) {
		GameObject::Update(timeElapsed);
		Move(Vector3::Mul(m_velocity, timeElapsed));

		// 중력적용
		m_velocity = Vector3::Add(m_velocity, XMFLOAT3(0.f, -10.f * timeElapsed, 0.f));

		if(m_velocity.y < 0)
			Rotate(0.f, -m_velocity.y / 3.f, 0.f);
	}
}

void Arrow::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (m_enable) GameObject::Render(commandList);
}

void Arrow::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView)
{
	if (m_enable) GameObject::Render(commandList, instanceBufferView);
}

void Arrow::Reset()
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_transformMatrix, XMMatrixIdentity());
	m_velocity = XMFLOAT3(0.f, 0.f, 0.f);
}

void Arrow::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	for (const auto & material : m_materials->m_materials) {
		material.UpdateShaderVariable(commandList);
	}
}

ArrowRain::ArrowRain() : m_enable{ false }, m_age{ 0.f },
	m_lifeTime{ (FLOAT)(chrono::duration_cast<chrono::seconds>(TriggerSetting::DURATION[static_cast<INT>(TriggerType::ARROW_RAIN)]).count())}
{
	for (auto& arrow : m_arrows) {
		arrow.first = make_shared<Arrow>();
		arrow.first->SetMesh("MeshArrow");
		arrow.first->SetMaterials("Archer_WeaponArrow");
		arrow.first->Rotate(0.f, 90.f, 0.f);
		arrow.first->SetEnable();
		arrow.second = Utiles::GetRandomFLOAT(0.f, ARROW_LIFECYCLE);
	}
	m_magicCircle = make_unique<GameObject>();
	m_magicCircle->SetMesh("MAGICCIRCLE");
	m_magicCircle->SetTexture("MAGICCIRCLE");
}

void ArrowRain::Update(FLOAT timeElapsed)
{
	if (m_enable) {
		m_age += timeElapsed;
		if (m_age >= m_lifeTime) {
			m_age = 0.f;
			m_enable = false;
			return;
		}
		for (auto& arrow : m_arrows) {
			arrow.second += timeElapsed;
			if (arrow.second >= ARROW_LIFECYCLE) {
				arrow.second = 0.f;
				auto position = arrow.first->GetPosition();
				arrow.first->SetPosition(Vector3::Add(position, { 0.f, MAX_ARROW_HEIGHT, 0.f }));
			}
			auto position = arrow.first->GetPosition();
			arrow.first->SetPosition(Vector3::Sub(position, { 0.f, MAX_ARROW_HEIGHT * timeElapsed / ARROW_LIFECYCLE, 0.f}));
		}
		m_magicCircle->Update(timeElapsed);
	}
}

void ArrowRain::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView)
{
	if (m_enable) {
		for (auto& arrow : m_arrows) {
			arrow.first->Render(commandList, instanceBufferView);
		}
	}
}

void ArrowRain::RenderMagicCircle(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (m_enable) {
		m_magicCircle->Render(commandList);
	}
}

void ArrowRain::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (m_arrows[0].first) m_arrows[0].first->UpdateShaderVariable(commandList);
}

void ArrowRain::SetPosition(const XMFLOAT3& position)
{
	GameObject::SetPosition(position);

	XMFLOAT3 extent = TriggerSetting::EXTENT[static_cast<INT>(TriggerType::ARROW_RAIN)];

	for (auto& arrow : m_arrows) {
		arrow.first->SetPosition(Vector3::Add(position, {
			Utiles::GetRandomFLOAT(-extent.x, extent.x),
			MAX_ARROW_HEIGHT - (MAX_ARROW_HEIGHT * arrow.second / ARROW_LIFECYCLE), 
			Utiles::GetRandomFLOAT(-extent.z, extent.z) }));
	}
	m_magicCircle->SetPosition(position);
}

array<pair<shared_ptr<Arrow>, FLOAT>, MAX_ARROWRAIN_ARROWS>& ArrowRain::GetArrows()
{
	return m_arrows;
}
