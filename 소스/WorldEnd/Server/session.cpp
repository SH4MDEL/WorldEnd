#include "session.h"
#include "stdafx.h"

Session::Session() : m_socket{}, m_player_data{ 0, false , {},{},{} , 100}, m_ready_check{FALSE},
m_player_type{ PlayerType::SWORD }, m_state{ STATE::ST_FREE }, m_prev_size{ 0 }
{
	strcpy_s(m_name, "Player\0");
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

DirectX::BoundingOrientedBox Session::GetBoundingBox() const
{
	BoundingOrientedBox result{};
	m_boundingbox.Transform(result, XMLoadFloat4x4(&m_worldMatrix));
	return result;
	return DirectX::BoundingOrientedBox();
}
