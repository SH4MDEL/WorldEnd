#include "player.h"
#include "camera.h"

Player::Player() : m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.5f }, m_id{-1}
{

}

void Player::Update(FLOAT timeElapsed)
{
	GameObject::Update(timeElapsed);

	Move(m_velocity);
	if (m_animationController) {
		float length = fabs(m_velocity.x) + fabs(m_velocity.z);
		if (length <= FLT_EPSILON) {
			m_animationController->SetTrackEnable(0, true);
			m_animationController->SetTrackEnable(1, false);
			m_animationController->SetTrackPosition(1, 0.0f);
		}
		else {
			m_animationController->SetTrackEnable(0, false);
			m_animationController->SetTrackEnable(1, true);
		}
	}

	ApplyFriction(timeElapsed);
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// ȸ���� ����
	if (m_roll + roll > MAX_ROLL)
		roll = MAX_ROLL - m_roll;
	else if (m_roll + roll < MIN_ROLL)
		roll = MIN_ROLL - m_roll;

	// ȸ���� �ջ�
	m_roll += roll; m_pitch += pitch; m_yaw += yaw;

	// ī�޶�� x,y������ ȸ���� �� �ִ�.
	// GameObject::Rotate���� �÷��̾��� ���� x,y,z���� �����ϹǷ� ���� ȣ���ؾ��Ѵ�.
	m_camera->Rotate(roll, pitch, 0.0f);

	// �÷��̾�� y�����θ� ȸ���� �� �ִ�.
	GameObject::Rotate(0.0f, pitch, 0.0f);
}

void Player::ApplyFriction(FLOAT deltaTime)
{
	m_velocity = Vector3::Mul(m_velocity, 1 / m_friction * deltaTime);
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

INT Player::GetId() const
{
	return m_id;
}
