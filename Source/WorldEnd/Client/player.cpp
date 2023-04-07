#include "player.h"
#include "camera.h"
#include "particleSystem.h"

Player::Player() : m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.5f }, 
	m_hp{ 100.f }, m_maxHp{ 100.f }, m_stamina{ PlayerSetting::PLAYER_MAX_STAMINA }, m_maxStamina{ PlayerSetting::PLAYER_MAX_STAMINA }, 
	m_id{ -1 }, m_cooltimeList{ false, }, m_dashed{ false }, m_moveSpeed{ PlayerSetting::PLAYER_WALK_SPEED },
	m_interactable{ false }, m_interactableType{ InteractableType::NONE }, m_bufSize{ 0 }
{
}

Player::~Player()
{
	m_hpBar.reset();
}

void Player::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
	if (GetAsyncKeyState('Q') & 0x8000) {
		if (!m_cooltimeList[CooltimeType::ULTIMATE]) {

			ChangeAnimation(PlayerAnimation::ULTIMATE);

			XMFLOAT3 pos = Vector3::Add(Vector3::Mul(m_front, 0.8f), GetPosition());
			CreateAttackPacket(pos, AttackType::ULTIMATE, CollisionType::MULTIPLE_TIMES,
				chrono::system_clock::now() + PlayerSetting::WARRIOR_ULTIMATE_COLLISION_TIME,
				CooltimeType::ULTIMATE);

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
		XMFLOAT3 eye = m_camera->GetEye();
		XMFLOAT3 direction{ Vector3::Normalize(Vector3::Sub(GetPosition(), eye)) };
		if (GetAsyncKeyState('W') & 0x8000) {
			XMFLOAT3 front{ Vector3::Normalize(Vector3::Sub(GetPosition(), eye)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), front) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), front) };
			if (clockwise.y >= 0) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		if (GetAsyncKeyState('A') & 0x8000) {
			XMFLOAT3 left{ Vector3::Normalize(Vector3::Cross(direction, GetUp())) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), left) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), left) };
			if (clockwise.y >= 0) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		if (GetAsyncKeyState('S') & 0x8000) {
			XMFLOAT3 back{ Vector3::Normalize(Vector3::Sub(eye, GetPosition())) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), back) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), back) };
			if (clockwise.y >= 0) {
				Rotate(0.f, 0.f, timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
			else {
				Rotate(0.f, 0.f, -timeElapsed * XMConvertToDegrees(angle.y) * 10.f);
			}
		}
		if (GetAsyncKeyState('D') & 0x8000) {
			XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(GetUp(), direction)) };
			XMFLOAT3 angle{ Vector3::Angle(GetFront(), right) };
			XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), right) };
			if (clockwise.y >= 0) {
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
		if (!m_cooltimeList[CooltimeType::ROLL]) {
			if (m_stamina >= PlayerSetting::MINIMUM_ROLL_STAMINA) {

				ChangeAnimation(PlayerAnimation::ROLL);
				m_moveSpeed = PlayerSetting::PLAYER_ROLL_SPEED;

				CreateCooltimePacket(CooltimeType::ROLL);
				CreateChangeStaminaPacket(false);

				SendPacket();
			}
		}
	}
	
	if (!m_dashed && GetAsyncKeyState(VK_SHIFT) & 0x8000) {
		if (!m_cooltimeList[CooltimeType::DASH]) {

			if (m_stamina >= PlayerSetting::MINIMUM_DASH_STAMINA) {
				m_dashed = true;
				ChangeAnimation(PlayerAnimation::DASH);
				m_moveSpeed = PlayerSetting::PLAYER_DASH_SPEED;
				m_startDash = chrono::system_clock::now();

				CreateCooltimePacket(CooltimeType::DASH);
				CreateChangeStaminaPacket(false);

				SendPacket();
			}
		}
	}
	if (m_dashed && GetAsyncKeyState(VK_SHIFT) == 0x0000) {
		m_dashed = false;
		CreateChangeStaminaPacket(true);
		SendPacket();
	}
	
	if (GetAsyncKeyState('E') & 0x8000) {
		if (!m_cooltimeList[CooltimeType::SKILL]) {

			ChangeAnimation(PlayerAnimation::SKILL);


			XMFLOAT3 pos = Vector3::Add(Vector3::Mul(m_front, 0.8f), GetPosition());
			CreateAttackPacket(pos, AttackType::SKILL, CollisionType::MULTIPLE_TIMES,
				chrono::system_clock::now() + PlayerSetting::WARRIOR_SKILL_COLLISION_TIME,
				CooltimeType::SKILL);

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
	if (m_cooltimeList[CooltimeType::NORMAL_ATTACK])
		return;

	ChangeAnimation(ObjectAnimation::ATTACK);

	if (PlayerType::WARRIOR == m_type) {
		XMFLOAT3 pos = Vector3::Add(Vector3::Mul(m_front, 0.8f), GetPosition());
		CreateAttackPacket(pos, AttackType::NORMAL, CollisionType::MULTIPLE_TIMES,
			chrono::system_clock::now() + PlayerSetting::WARRIOR_ATTACK_COLLISION_TIME,
			CooltimeType::NORMAL_ATTACK);
	}
	else if (PlayerType::ARCHER == m_type) {

	}
	SendPacket();
}

void Player::Update(FLOAT timeElapsed)
{
	AnimationObject::Update(timeElapsed);

	// 애니메이션 상태 머신에 들어갈 내용
	// 상태 전환을 하는 함수를 작성하고 해당 함수를 호출하도록 해야 함
	if (m_animationController) {
		if (ObjectAnimation::ATTACK == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			// 공격 애니메이션 종료 시 IDLE 로 변경
			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::IDLE);
				SendPacket();
			}
		}
		else if (PlayerAnimation::SKILL == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			// 스킬 애니메이션 종료 시 IDLE 로 변경
			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				if (m_dashed && (PlayerAnimation::RUN == m_prevAnimation))
					ChangeAnimation(PlayerAnimation::RUN);
				else
					ChangeAnimation(ObjectAnimation::IDLE);

				SendPacket();
			}
		}
		else if (PlayerAnimation::ULTIMATE == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			// 스킬 애니메이션 종료 시 IDLE 로 변경
			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				if (m_dashed && (PlayerAnimation::RUN == m_prevAnimation))
					ChangeAnimation(PlayerAnimation::RUN);
				else
					ChangeAnimation(ObjectAnimation::IDLE);

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
					ChangeAnimation(PlayerAnimation::RUN);
					m_moveSpeed = PlayerSetting::PLAYER_RUN_SPEED;
				}
				else {
					ChangeAnimation(ObjectAnimation::IDLE);
					m_moveSpeed = PlayerSetting::PLAYER_WALK_SPEED;
					
					CreateChangeStaminaPacket(true);
				}
			}
			SendPacket();
		}
		else if (PlayerAnimation::DASH == m_currentAnimation) {
			auto& track = m_animationController->GetAnimationTrack(0);
			auto& animation = m_animationController->GetAnimationSet()->GetAnimations()[track.GetAnimation()];

			
			if (chrono::system_clock::now() > m_startDash + PlayerSetting::PLAYER_DASH_DURATION) {
				ChangeAnimation(ObjectAnimation::RUN);
				m_moveSpeed = PlayerSetting::PLAYER_RUN_SPEED;

				m_animationController->SetTrackEnable(1, false);
				m_animationController->SetTrackWeight(0, 1.f);
				m_animationController->SetBlendingMode(AnimationBlending::NORMAL);

				SendPacket();
			}
			else if (chrono::system_clock::now() > m_startDash + 50ms) {
				m_animationController->SetTrackEnable(1, true);
			}
			else if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::WALK);
				m_moveSpeed = PlayerSetting::PLAYER_WALK_SPEED;
				SendPacket();
			}
		}
		else {
			float length = fabs(m_velocity.x) + fabs(m_velocity.z);
			if (length <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::IDLE);
				SendPacket();
			}
			else {
				if (m_dashed) {
					if (m_stamina <= numeric_limits<float>::epsilon()) {
						m_dashed = false;
						m_moveSpeed = PlayerSetting::PLAYER_WALK_SPEED;

						ChangeAnimation(ObjectAnimation::WALK);
						CreateChangeStaminaPacket(true);
						SendPacket();
					}
					else {
						ChangeAnimation(ObjectAnimation::RUN);
						SendPacket();
					}
				}
				else {
					ChangeAnimation(ObjectAnimation::WALK);
					m_moveSpeed = PlayerSetting::PLAYER_WALK_SPEED;
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
	}
#ifndef USE_NETWORK
	Move(m_velocity);
#endif // !USE_NETWORK

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

void Player::ResetCooltime(char type)
{
	m_cooltimeList[type] = false;
}

bool Player::ChangeAnimation(USHORT animation)
{
	if (!AnimationObject::ChangeAnimation(animation))
		return false;
#ifdef USE_NETWORK
	CS_CHANGE_ANIMATION_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CHANGE_ANIMATION;
	packet.animation_type = m_currentAnimation;
	SetBuffer(&packet, packet.size);
	return true;
#endif
}

void Player::ChangeAnimation(USHORT animation, bool other)
{
	AnimationObject::ChangeAnimation(animation);
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

void Player::CreateCooltimePacket(CooltimeType type)
{
#ifdef USE_NETWORK
	m_cooltimeList[type] = true;

	CS_COOLTIME_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_SET_COOLTIME;
	packet.cooltime_type = type;
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateAttackPacket(const XMFLOAT3& pos, AttackType attackType,
	CollisionType collisionType, chrono::system_clock::time_point eventTime,
	CooltimeType cooltimeType)
{
#ifdef USE_NETWORK
	m_cooltimeList[cooltimeType] = true;

	CS_ATTACK_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_ATTACK;
	packet.position = pos;
	packet.attack_type = attackType;
	packet.collision_type = collisionType;
	packet.event_time = eventTime;
	packet.cooltime_type = cooltimeType;
	SetBuffer(&packet, packet.size);
#endif
}

void Player::CreateInteractPacket()
{
#ifdef USE_NETWORK
	CS_INTERACT_OBJECT_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_INTERACT_OBJECT;
	packet.interactable_type = m_interactableType;
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
	memcpy(m_sendBuffer + static_cast<char>(m_bufSize), c, size);
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
