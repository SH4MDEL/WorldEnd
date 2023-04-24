#include "Client.h"

void Client::DoSend(void* packet)
{
	int psize = reinterpret_cast<unsigned char*>(packet)[0];
	int ptype = reinterpret_cast<unsigned char*>(packet)[1];
	ExpOver* ex_over = new ExpOver;
	ex_over->event_type = OP_SEND;
	memcpy(ex_over->IOCP_buf, packet, psize);
	ZeroMemory(&ex_over->over, sizeof(ex_over->over));
	ex_over->wsabuf.buf = reinterpret_cast<CHAR*>(ex_over->IOCP_buf);
	ex_over->wsabuf.len = psize;
	int ret = WSASend(m_client_socket, &ex_over->wsabuf, 1, NULL, 0,
		&ex_over->over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			ErrorDisplay("Error in SendPacket:", err_no);
	}
}

void Client::DoRecv()
{
	DWORD recv_flag = 0;
	int ret = WSARecv(m_client_socket,&m_recv_over.wsabuf, 1,
		NULL, &recv_flag, &m_recv_over.over, NULL);
	if (SOCKET_ERROR == ret) {
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
		{
			ErrorDisplay("RECV ERROR", err_no);
			
		}
	}
}
