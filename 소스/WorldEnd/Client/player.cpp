#include "player.h"
#include "camera.h"

Player::Player() : m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.5f }
{
	m_gravity = { 0.0f, -1.0f, 0.0f };
	m_jumpPower = m_gravity.y * -2.0f;
	m_jumpVelocity = { 0.0f, 0.0f, 0.0f };
	m_jumpDelta = -1.0f;

	// 현재 지형정보 X
	SetObjectContext(nullptr);
}

void Player::Update(FLOAT timeElapsed)
{
	m_velocity = Vector3::Add(m_velocity, Vector3::Mul(m_gravity, timeElapsed));

	m_velocity = Vector3::Add(m_velocity, Vector3::Mul(m_jumpVelocity, timeElapsed));
	// 점프 속도를 얼마나 감소시킬지
	m_jumpVelocity.y += (m_jumpDelta * timeElapsed * 5.0f);

	GameObject::Update(timeElapsed);

	Move(m_velocity);
	
	// 점프상태일때는 관성 적용, 점프 아닐때만 마찰력 적용
	if (JUMP != m_curState)
		ApplyFriction(timeElapsed);

	ObjectUpdateCallBack(timeElapsed);

	ChangeState();

	// 디버그용 출력
	cout << GetCurrentStateString() << endl;
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전각 제한
	if (m_roll + roll > MAX_ROLL)
		roll = MAX_ROLL - m_roll;
	else if (m_roll + roll < MIN_ROLL)
		roll = MIN_ROLL - m_roll;

	// 회전각 합산
	m_roll += roll; m_pitch += pitch; m_yaw += yaw;

	// 카메라는 x,y축으로 회전할 수 있다.
	// GameObject::Rotate에서 플레이어의 로컬 x,y,z축을 변경하므로 먼저 호출해야한다.
	m_camera->Rotate(roll, pitch, 0.0f);

	// 플레이어는 y축으로만 회전할 수 있다.
	GameObject::Rotate(0.0f, pitch, 0.0f);
}

void Player::ObjectUpdateCallBack(float timeElapsed)
{
	// 지형 정보 O
	if (m_updateContext) {

	}

	// 지형 정보 X	-> 현재 바닥이 0.0f 에 있다고 가정
	else {
		XMFLOAT3 pos = GetPosition();

		// 바닥에 닿으면
		if (pos.y < 0.0f) {
			XMFLOAT3 velocity = GetVelocity();
			velocity.y = 0.0f;
			SetVelocity(velocity);

			pos.y = 0.0f;
			SetPosition(pos);

			// 점프 이전의 상태로 변경함
			if (JUMP == m_curState) {
				SetState(m_prevState);
				m_jumpVelocity = { 0.0f, 0.0f, 0.0f };
			}
		}
	}
}

void Player::ApplyFriction(FLOAT deltaTime)
{
	m_velocity = Vector3::Mul(m_velocity, 1 / m_friction * deltaTime);
}

void Player::AddVelocity(const XMFLOAT3& increase)
{
	m_velocity = Vector3::Add(m_velocity, increase);

	// 최대 속도에 걸린다면 해당 비율로 축소시킴
	FLOAT length{ Vector3::Length(m_velocity) };
	if (length > m_maxVelocity)
	{
		FLOAT ratio{ m_maxVelocity / length };
		m_velocity = Vector3::Mul(m_velocity, ratio);
	}
}

void Player::ChangeState()
{
	if (m_curState == WALK) {
		if (abs(m_velocity.x) < FLT_EPSILON && abs(m_velocity.z) < FLT_EPSILON)
			SetState(IDLE);
	}
	
}
