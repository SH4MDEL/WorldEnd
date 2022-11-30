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

int m_clientID = 0;
SOCKET m_c_socket;

void CALLBACK SendCallBack(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flag);
bool Init();
bool ConnectTo();

void SendPacket(void* packet);

bool Init()
{
	// was start
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		cout << "WSA START ERROR!!" << endl;
		return false;
	}

	// socket
	m_c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (m_c_socket == INVALID_SOCKET) {
		cout << "SOCKET INIT ERROR!!" << endl;
		return true;
	}


}


bool ConnectTo()
{
	// connect to ipAddr
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVERPORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);
	int val = connect(m_c_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (val == SOCKET_ERROR) return false;

	// login ok
	return true;
}

void SendPacket(void* packet)
{
	EXP_OVER* s_data = new EXP_OVER{ reinterpret_cast<char*>(packet) };
	cout << "Send" << endl;
	int ret = WSASend(m_c_socket, &s_data->_wsabuf, 1, 0, 0, &s_data->_over, SendCallBack);
	if (ret != 0) {
		cout << "Send Error - " << ret << endl;
		cout << GetLastError() << endl;
	}

}

void SendCallBack(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flag)
{
	cout << "Send Call back" << endl;
	delete over;
	return;
}







class Connect
{
public:
	int m_clientID = 0;
	SOCKET m_c_socket;


public:
	Connect();
	~Connect();

	/*bool Init();
	bool ConnectTo();

	void SendPacket(void* packet);*/

	
 
};

