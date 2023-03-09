#include "session.h"
#include "stdafx.h"

Session::Session() : m_socket{}, m_player_data{ 0, false , {},{},{} , 100}, m_ready_check{FALSE},
	m_state{ STATE::ST_FREE }, m_prev_size{ 0 }, m_exist_check{TRUE}
{
	strcpy_s(m_name, "Player\0");
	SetPlayerType(PlayerType::WARRIOR);
}

Session::~Session()
{
}

void Session::DoRecv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&m_recv_over._wsa_over, sizeof(m_recv_over._wsa_over));
	m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(m_recv_over._send_buf + m_prev_size);
	m_recv_over._wsa_buf.len = sizeof(m_recv_over._send_buf) - m_prev_size;
	int ret = WSARecv(m_socket, &m_recv_over._wsa_buf, 1, 0, &recv_flag, &m_recv_over._wsa_over, NULL);
	if (SOCKET_ERROR == ret)
	{
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
		{
			//g_server.Disconnect(data.id);
			if (error_num == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(m_player_data.id) << " Session] Disconnect(do_recv)" << std::endl;
			else ErrorDisplay("do_recv");
		}
	}
}

void Session::DoSend()
{
}

void Session::SetPlayerType(PlayerType type)
{
	m_player_type = type;
	SetBoundingBox(type);
}

void Session::SetBoundingBox(PlayerType type)
{
	switch (type) {
	case PlayerType::ARCHER:
		// 바운드 박스는 현재 궁수의 몸통을 기준으로 생성함
		//m_bounding_box = BoundingOrientedBox{ m_player_data.pos, XMFLOAT3{0.65f, 0.37f, 0.65f}, q };
		m_bounding_box = BoundingOrientedBox{ m_player_data.pos, XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f } };
		m_weopon_bounding_box = BoundingOrientedBox{ m_player_data.pos, XMFLOAT3{0.18f, 0.04f, 0.68f}, q };
		break;

	case PlayerType::WARRIOR:
		//m_bounding_box = BoundingOrientedBox{ m_player_data.pos, XMFLOAT3{0.65f, 0.17f, 0.7f}, q };
		m_bounding_box = BoundingOrientedBox{ m_player_data.pos, XMFLOAT3{0.37f, 0.65f, 0.37f}, q };
		m_weopon_bounding_box = BoundingOrientedBox{ m_player_data.pos, XMFLOAT3{0.13f, 0.03f, 0.68f}, q };
		break;
	}
}

DirectX::BoundingOrientedBox Session::GetBoundingBox() const
{
	return m_bounding_box;
}
