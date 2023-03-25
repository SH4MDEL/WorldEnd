#include "player.h"
#include "camera.h"
#include "particleSystem.h"

Player::Player() : m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.5f }, 
	m_hp{ 100.f }, m_maxHp{ 100.f }, m_id{ -1 }, m_cooltimeList{ false, }
{

}

Player::~Player()
{
	m_hpBar.reset();
}

void Player::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
	XMFLOAT3 eye = m_camera->GetEye();
	XMFLOAT3 direction{ Vector3::Normalize(Vector3::Sub(GetPosition(), eye)) };
	if (GetAsyncKeyState('W') & 0x8000)
	{
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
	if (GetAsyncKeyState('A') & 0x8000)
	{
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
	if (GetAsyncKeyState('S') & 0x8000)
	{
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
	if (GetAsyncKeyState('D') & 0x8000)
	{
		XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(GetUp(), direction)) };
		XMFLOAT3 angle{ Vector3::Angle(GetFront(), right) };
		XMFLOAT3 clockwise{ Vector3::Cross(GetFront(), right)};
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
		AddVelocity(Vector3::Mul(GetFront(), timeElapsed * PlayerSetting::PLAYER_RUN_SPEED));

#ifdef USE_NETWORK
		CS_PLAYER_MOVE_PACKET move_packet;
		move_packet.size = sizeof(move_packet);
		move_packet.type = CS_PACKET_PLAYER_MOVE;
		move_packet.pos = GetPosition();
		move_packet.velocity = GetVelocity();
		move_packet.yaw = GetYaw();

		send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
#endif // USE_NETWORK
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		if (m_cooltimeList[CooltimeType::ULTIMATE])
			return;

		cout << "Ultimate" << endl;
		// 임시 위치
		XMFLOAT3 pos = GetPosition();
		SendAttackPacket(pos, AttackType::ULTIMATE, CollisionType::MULTIPLE_TIMES,
			chrono::system_clock::now() + PlayerSetting::WARRIOR_ULTIMATE_COLLISION_TIME,
			CooltimeType::ULTIMATE);
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{

	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{

	}
	
	if (GetAsyncKeyState('E') & 0x8000) {
		if (m_cooltimeList[CooltimeType::SKILL])
			return;

		cout << "Skill" << endl;
		// 임시 위치
		XMFLOAT3 pos = GetPosition();
		SendAttackPacket(pos, AttackType::SKILL, CollisionType::MULTIPLE_TIMES,
			chrono::system_clock::now() + PlayerSetting::WARRIOR_SKILL_COLLISION_TIME,
			CooltimeType::SKILL);
	}
}

void Player::OnProcessingClickMessage(LPARAM lParam)
{
	if (m_cooltimeList[CooltimeType::NORMAL_ATTACK])
		return;

	ChangeAnimation(ObjectAnimation::ATTACK);

	if (PlayerType::WARRIOR == m_type) {
		XMFLOAT3 pos = Vector3::Add(Vector3::Mul(m_front, 0.8f), GetPosition());
		SendAttackPacket(pos, AttackType::NORMAL, CollisionType::MULTIPLE_TIMES,
			chrono::system_clock::now() + PlayerSetting::WARRIOR_ATTACK_COLLISION_TIME,
			CooltimeType::NORMAL_ATTACK);
	}
	else if (PlayerType::ARCHER == m_type) {

	}
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
			}
		}
		else {
			float length = fabs(m_velocity.x) + fabs(m_velocity.z);
			if (length <= numeric_limits<float>::epsilon()) {
				ChangeAnimation(ObjectAnimation::IDLE);
			}
			else {
				ChangeAnimation(ObjectAnimation::WALK);
			}
		}
	}

	static FLOAT dummy = 50.f;
	if (dummy > 100.f) {
		dummy -= 100.f;
	}
	else {
		dummy += timeElapsed * 10.f;
	}

	if (m_hpBar) {
		m_hpBar->SetMaxHp(m_maxHp);
		m_hpBar->SetHp(dummy);
		XMFLOAT3 hpBarPosition = GetPosition();
		hpBarPosition.y += 1.8f;
		m_hpBar->SetPosition(hpBarPosition);
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

	// �ִ� �ӵ��� �ɸ��ٸ� �ش� ������ ��ҽ�Ŵ
	FLOAT length{ Vector3::Length(m_velocity) };
	if (length > m_maxVelocity)
	{
		FLOAT ratio{ m_maxVelocity / length };
		m_velocity = Vector3::Mul(m_velocity, ratio);
	}
}

void Player::ResetCooltime(CooltimeType type)
{
	m_cooltimeList[type] = false;
}

bool Player::ChangeAnimation(int animation)
{
	if (!AnimationObject::ChangeAnimation(animation))
		return false;
#ifdef USE_NETWORK
	CS_CHANGE_ANIMATION_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CHANGE_ANIMATION;
	packet.animation_type = m_currentAnimation;
	send(g_socket, reinterpret_cast<char*>(&packet), packet.size, 0);
#endif
	return true;
}

void Player::ChangeAnimation(int animation, bool other)
{
	AnimationObject::ChangeAnimation(animation);
}

void Player::SendCooltimePacket(CooltimeType type)
{
#ifdef USE_NETWORK
	m_cooltimeList[type] = true;

	CS_COOLTIME_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_SET_COOLTIME;
	packet.cooltime_type = type;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void Player::SendAttackPacket(const XMFLOAT3& pos, AttackType attackType,
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
	send(g_socket, reinterpret_cast<char*>(&packet), packet.size, 0);
#endif
}

// ------------- 콜백 함수 -----------
void AttackCallbackHandler::Callback(void* callbackData, float trackPosition)
{
	Player* p = static_cast<Player*>(callbackData);
	p->GetAnimationController()->SetTrackEnable(0, true);
	p->GetAnimationController()->SetTrackEnable(2, false);
}
