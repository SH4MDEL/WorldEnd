#include "player.h"
#include "camera.h"

Player::Player() : m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.5f }, m_hp{ 100.f }, m_maxHp{ 100.f }, m_id { -1 }
{

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
		AddVelocity(Vector3::Mul(GetFront(), timeElapsed * 10.0f));

#ifdef USE_NETWORK
		CS_PLAYER_MOVE_PACKET move_packet;
		move_packet.size = sizeof(move_packet);
		move_packet.type = CS_PACKET_PLAYER_MOVE;
		move_packet.pos = GetPosition();
		move_packet.velocity = GetVelocity();
		move_packet.yaw = GetYaw();

		send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
		cout << " x: " << move_packet.pos.x << " y: " << move_packet.pos.y <<
			" z: " << move_packet.pos.z << endl;
#endif // USE_NETWORK
	}
	if (GetAsyncKeyState('Q') & 0x8000)
	{
		SetPosition(XMFLOAT3{ GetPosition().x, GetPosition().y + 0.01f, GetPosition().z });
		cout << GetPosition().x << ", " << GetPosition().y << ", " << GetPosition().z << endl;

	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{

	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{

	}
	
	if (GetAsyncKeyState('E') & 0x8000) {

	}
}

void Player::OnProcessingClickMessage(LPARAM lParam)
{
	ChangeAnimation(ObjectAnimation::ATTACK);

#ifdef USE_NETWORK
	CS_ATTACK_PACKET attack_packet;
	attack_packet.size = sizeof(attack_packet);
	attack_packet.type = CS_PACKET_PLAYER_ATTACK;
	attack_packet.attack_type = AttackType::NORMAL;
	send(g_socket, reinterpret_cast<char*>(&attack_packet), sizeof(attack_packet), 0);
#endif
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

			if (fabs(track.GetPosition() - animation->GetLength()) <= numeric_limits<float>::epsilon()) {
				m_isAttackCheck = false;
				ChangeAnimation(ObjectAnimation::IDLE);
			}

			// 공격이 수행되는 프레임에 충돌 패킷 전송
#ifdef USE_NETWORK
			else if (m_type == PlayerType::WARRIOR && fabs(track.GetPosition() - 0.21f) <= 0.01f) {
				m_isAttackCheck = true;
				
				auto& obj = FindFrame("Warrior_WeaponSword");
				/*XMFLOAT4X4 world = GetWorldMatrix();
				XMFLOAT4X4 anim = obj->GetAnimationMatrix();
				XMFLOAT4X4 mat = Matrix::Mul(world, anim);
				XMFLOAT3 pos{ mat._41, mat._42, mat._43 };*/
				XMFLOAT3 front = GetFront();
				XMFLOAT3 pos = Vector3::Add(front, GetPosition());

				CS_WEAPON_COLLISION_PACKET packet;
				packet.size = sizeof(packet);
				packet.type = CS_PACKET_WEAPON_COLLISION;
				packet.x = pos.x;
				packet.y = pos.y;
				packet.z = pos.z;
				packet.attack_type = AttackType::NORMAL;
				packet.collision_type = CollisionType::MULTIPLE_TIMES;
				packet.end_time = chrono::system_clock::now();
				send(g_socket, reinterpret_cast<char*>(&packet), packet.size, 0);
			}
			else if (m_type == PlayerType::ARCHER && fabs(track.GetPosition() - 0.4f) <= 0.01f) {
				m_isAttackCheck = true;

				auto& obj = FindFrame("Archer_WeaponBow");
				XMFLOAT4X4 world = GetWorldMatrix();
				XMFLOAT4X4 anim = obj->GetAnimationMatrix();
				XMFLOAT4X4 mat = Matrix::Mul(world, anim);
				XMFLOAT3 pos{ mat._41, mat._42, mat._43 };

				CS_WEAPON_COLLISION_PACKET packet;
				packet.size = sizeof(packet);
				packet.type = CS_PACKET_WEAPON_COLLISION;
				packet.x = pos.x;
				packet.y = pos.y;
				packet.z = pos.z;
				packet.attack_type = AttackType::NORMAL;
				packet.collision_type = CollisionType::ONE_OFF;
				packet.end_time = chrono::system_clock::now();
				send(g_socket, reinterpret_cast<char*>(&packet), packet.size, 0);
			}
#endif
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

// ------------- 콜백 함수 -----------
void AttackCallbackHandler::Callback(void* callbackData, float trackPosition)
{
	Player* p = static_cast<Player*>(callbackData);
	p->GetAnimationController()->SetTrackEnable(0, true);
	p->GetAnimationController()->SetTrackEnable(2, false);
}
