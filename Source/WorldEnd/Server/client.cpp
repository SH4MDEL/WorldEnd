#include "client.h"
#include "stdafx.h"
#include "object.h"
#include "Server.h"

ExpOver::ExpOver()
{
	_wsa_buf.len = BUF_SIZE;
	_wsa_buf.buf = _send_buf;
	_comp_type = OP_RECV;
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
}

ExpOver::ExpOver(char* packet)
{
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
	_wsa_buf.buf = _send_buf;
	_wsa_buf.len = packet[0];
	_comp_type = OP_SEND;
	memcpy(_send_buf, packet, packet[0]);
}

ExpOver::ExpOver(char* packet, INT packet_count)
{
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
	_wsa_buf.buf = _send_buf;
	_wsa_buf.len = packet[0] * packet_count;
	_comp_type = OP_SEND;
	memcpy(_send_buf, packet, static_cast<size_t>(packet[0] * packet_count));
}

ExpOver::ExpOver(char* packet1, INT count1, char* packet2, INT count2)
{
	ZeroMemory(&_wsa_over, sizeof(_wsa_over));
	_wsa_buf.buf = _send_buf;
	_wsa_buf.len = packet1[0] * count1 + packet2[0] * count2;
	_comp_type = OP_SEND;
	memcpy(_send_buf, packet1, static_cast<size_t>(packet1[0] * count1));
	memcpy(_send_buf + (packet1[0] * count1), packet2, static_cast<size_t>(packet2[0] * count2));
}

Client::Client() : m_socket{}, m_ready_check{ false }, m_remain_size{ 0 },
	m_recv_over{}, m_stamina{ PlayerSetting::PLAYER_MAX_STAMINA }, m_is_dash{ false },
	m_interactable{ false }
{
	m_name = "Player";
	SetPlayerType(PlayerType::WARRIOR);
	m_damage = 90.f;
	m_skill_ratio = 1.2f;

	m_max_hp = 100.f;
	m_hp = 100.f;
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
	if (SOCKET_ERROR == ret){
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num){
			//g_server.Disconnect(data.id);
			if (error_num == WSAECONNRESET)
				std::cout << "[" << m_id << " Client] Disconnect(do_recv)" << std::endl;
			else ErrorDisplay("do_recv");
		}
	}
}

void Client::DoSend(void* p)
{
	ExpOver* ex_over = new ExpOver{ reinterpret_cast<char*>(p) };
	int retval = WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, nullptr);
	if (SOCKET_ERROR == retval) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			ErrorDisplay("Send Error");
			
			if (WSAECONNRESET == error_num) {
				Server& server = Server::GetInstance();
				server.Disconnect(m_id);
			}
		}
	}
}

void Client::DoSend(void* p, INT packet_count)
{
	ExpOver* ex_over = new ExpOver{ reinterpret_cast<char*>(p), packet_count };
	int retval = WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, nullptr);
	if (SOCKET_ERROR == retval) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			ErrorDisplay("Send Error");

			if (WSAECONNRESET == error_num) {
				Server& server = Server::GetInstance();
				server.Disconnect(m_id);
			}
		}
	}
}

void Client::DoSend(void* p1, INT count1, void* p2, INT count2)
{
	ExpOver* ex_over = new ExpOver{ reinterpret_cast<char*>(p1), count1, reinterpret_cast<char*>(p2), count2 };
	int retval = WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, nullptr);
	if (SOCKET_ERROR == retval) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) {
			ErrorDisplay("Send Error");

			if (WSAECONNRESET == error_num) {
				Server& server = Server::GetInstance();
				server.Disconnect(m_id);
			}
		}
	}
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

void Client::SetWeaponCenter(const XMFLOAT3& center)
{
	m_weopon_bounding_box.Center = center;
}

void Client::SetWeaponOrientation(const XMFLOAT4& orientation)
{
	m_weopon_bounding_box.Orientation = orientation;
}

void Client::SetStamina(FLOAT stamina)
{
	m_stamina = stamina;
}

void Client::SetIsDash(bool val)
{
	m_is_dash = val;
}

void Client::SetInteractable(bool val)
{
	m_interactable = val;
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
	default:
		return 1.f;
	}

	return ratio;
}

PLAYER_DATA Client::GetPlayerData() const
{
	return PLAYER_DATA(m_id, m_position, m_velocity, m_yaw, m_hp);
}

void Client::ChangeStamina(FLOAT value)
{
	m_stamina += value;
	if (m_stamina <= 0.f)
		m_stamina = 0.f;
}


void Client::SetBoundingBox(PlayerType type)
{
	switch (type) {
	case PlayerType::WARRIOR:
		m_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weopon_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.13f, 0.03f, 0.68f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		break;

	case PlayerType::ARCHER:
		m_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weopon_bounding_box = BoundingOrientedBox{ m_position, XMFLOAT3{0.18f, 0.04f, 0.68f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		// 화살 바운드 박스로 교체 필요함, 활은 충돌하지 않을 것이기 때문
		break;
	}
}

Party::Party()
{
	for (size_t i = 0; i < MAX_INGAME_USER; ++i) {
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
		party = std::make_shared<Party>();
	}
		
}
