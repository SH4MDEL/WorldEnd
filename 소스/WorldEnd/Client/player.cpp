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
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{

	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{

	}
	
	if (GetAsyncKeyState('E') & 0x8000) {
		char packet_direction{};
		packet_direction += INPUT_KEY_E;
		CS_ATTACK_PACKET attack_packet;
		attack_packet.size = sizeof(attack_packet);
		attack_packet.type = CS_PACKET_PLAYER_ATTACK;
		attack_packet.key = packet_direction;

		send(g_socket, reinterpret_cast<char*>(&attack_packet), sizeof(attack_packet), 0);
	}
}

void Player::Update(FLOAT timeElapsed)
{
	GameObject::Update(timeElapsed);

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

//#ifndef USE_NETWORK
	Move(m_velocity);
//#endif // !USE_NETWORK

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

	// 최대 속도에 걸린다면 해당 비율로 축소시킴
	FLOAT length{ Vector3::Length(m_velocity) };
	if (length > m_maxVelocity)
	{
		FLOAT ratio{ m_maxVelocity / length };
		m_velocity = Vector3::Mul(m_velocity, ratio);
	}
}
