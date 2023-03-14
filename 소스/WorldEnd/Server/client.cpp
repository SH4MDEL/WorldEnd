#include "client.h"
#include "stdafx.h"
#include "object.h"

Client::Client() : m_socket{}, m_ready_check{ false }, m_remain_size{ 0 },
	m_recv_over{}
{
	m_name = "Player";
	SetPlayerType(PlayerType::WARRIOR);
	m_damage = 10;
	m_skill_ratio = 1.2f;
}

Client::~Client()
{
}

void Client::DoRecv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&m_recv_over._wsa_over, sizeof(m_recv_over._wsa_over));
	m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(m_recv_over._send_buf + m_remain_size);
	m_recv_over._wsa_buf.len = sizeof(m_recv_over._send_buf) - m_remain_size;
	int ret = WSARecv(m_socket, &m_recv_over._wsa_buf, 1, 0, &recv_flag, &m_recv_over._wsa_over, NULL);
	if (SOCKET_ERROR == ret)
	{
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
		{
			//g_server.Disconnect(data.id);
			if (error_num == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(m_id) << " Client] Disconnect(do_recv)" << std::endl;
			else ErrorDisplay("do_recv");
		}
	}
}

void Client::DoSend()
{
}

void Client::SetPlayerType(PlayerType type)
{
	m_player_type = type;
	SetBoundingBox(type);
}

void Client::SetSkillRatio(AttackType type, FLOAT ratio)
{
	switch (type) {
	case AttackType::SKILL:
		m_skill_ratio = ratio;
		break;
	case AttackType::ULTIMATE:
		m_ultimate_ratio = ratio;
		break;
	}
}

FLOAT Client::GetSkillRatio(AttackType type) const
{
	FLOAT ratio{};
	switch (type) {
	case AttackType::SKILL:
		ratio = m_skill_ratio;
		break;
	case AttackType::ULTIMATE:
		ratio = m_ultimate_ratio;
		break;
	}

	return ratio;
}

PLAYER_DATA Client::GetPlayerData() const
{
	return PLAYER_DATA(m_id, m_position, m_velocity, m_yaw, m_hp);
}


void Client::SetBoundingBox(PlayerType type)
{
	switch (type) {
	case PlayerType::ARCHER:
		m_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weopon_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.18f, 0.04f, 0.68f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		break;

	case PlayerType::WARRIOR:
		m_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weopon_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.13f, 0.03f, 0.68f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		// 화살 바운드 박스로 교체 필요함, 활은 충돌하지 않을 것이기 때문
		break;
	}
}

Party::Party()
{
	for (size_t i = 0; i < MAX_IN_GAME_USER; ++i) {
		m_members[i] = -1;
	}
}

bool Party::Join(INT player_id)
{
	// 여러 쓰레드에서 접근 할수 있으니 lock이 필요해 보임
	for (INT& id : m_members) {
		if (-1 == id) {
			id = player_id;
			return true;
		}
	}
	return false;
}

PartyManager::PartyManager()
{
	for (auto& party : m_parties) {
		party = make_shared<Party>();
	}
		
}
