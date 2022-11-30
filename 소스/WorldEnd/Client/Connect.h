#pragma once
#include "stdafx.h"

#include "../Server/protocol.h"

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

class EXP_OVER {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	EXP_OVER()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	EXP_OVER(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};



class Connect
{
public:
	int m_clientID = 0;
	SOCKET m_c_socket;

public:
	Connect();
	~Connect();

	bool Init();
	bool ConnectTo();

	void SendPacket(void* packet);
 
};

