#include "Connect.h"

Connect::Connect()
{

}

Connect::~Connect()
{
	closesocket(m_c_socket);
	WSACleanup();
}

bool Connect::Init()
{
	// was start
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		cout << "WSA START ERROR!!" << endl;
		return false;
	}

	// socket
	m_c_socket = socket(AF_INET, SOCK_STREAM,0);
	if (m_c_socket == INVALID_SOCKET) {
		cout << "SOCKET INIT ERROR!!" << endl;
		return false;
	}

	return true;
}


bool Connect::ConnectTo()
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
