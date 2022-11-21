#pragma once
#include "stdafx.h"


class Connect
{
	int m_clientID = 0;
	SOCKET m_c_socket;

public:
	Connect();
	~Connect();

	bool Init();
	bool ConnectTo();
 
};

