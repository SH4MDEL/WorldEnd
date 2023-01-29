#include "towerScene.h"

TowerScene::TowerScene() : m_left_other_player_id{-1}, m_right_other_player_id{-1}
{
#ifdef USE_NETWORK

	wcout.imbue(locale("korean"));
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		cout << "WSA START ERROR" << endl;
	}

	// socket 생성
	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (g_socket == INVALID_SOCKET) {
		cout << "SOCKET INIT ERROR!" << endl;
	}

	// connect
	SOCKADDR_IN server_address{};
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, g_serverIP.c_str(), &(server_address.sin_addr.s_addr));

	connect(g_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address));

	g_networkThread = thread{ &TowerScene::RecvPacket, this };
#endif
}

TowerScene::~TowerScene()
{
}

void TowerScene::OnCreate()
{
}

void TowerScene::OnDestroy()
{
}

void TowerScene::ReleaseUploadBuffer() {}

void TowerScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const
{
	SetCursor(NULL);
	RECT rect; GetWindowRect(hWnd, &rect);
	POINT prevPosition{ rect.left + width / 2, rect.top + height / 2 };

	POINT nextPosition; GetCursorPos(&nextPosition);

	int dx = nextPosition.x - prevPosition.x;
	int dy = nextPosition.y - prevPosition.y;
	m_player->Rotate(dy * 5.0f * deltaTime, dx * 5.0f * deltaTime, 0.0f);
	SetCursorPos(prevPosition.x, prevPosition.y);
}

void TowerScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const
{
#ifdef USE_NETWORK
	char packet_direction = 0;

	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * 10.0f));
		CS_PLAYER_MOVE_PACKET move_packet;
		move_packet.size = sizeof(move_packet);
		move_packet.type = CS_PACKET_PLAYER_MOVE;
		move_packet.pos = m_player->GetPosition();
		move_packet.velocity = m_player->GetVelocity();
		//move_packet.yaw = m_yaw;
		send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
		cout << " x: " << m_player->GetPosition().x << " y: " << m_player->GetPosition().y <<  
			" z: " << m_player->GetPosition().z << endl;
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * -10.0f));
		CS_PLAYER_MOVE_PACKET move_packet;
		move_packet.size = sizeof(move_packet);
		move_packet.type = CS_PACKET_PLAYER_MOVE;
		move_packet.pos = m_player->GetPosition();
		move_packet.velocity = m_player->GetVelocity();
		//move_packet.yaw = m_yaw;
		send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
		cout << " x: " << m_player->GetPosition().x << " y: " << m_player->GetPosition().y <<
			" z: " << m_player->GetPosition().z << endl;
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * -10.0f));
		CS_PLAYER_MOVE_PACKET move_packet;
		move_packet.size = sizeof(move_packet);
		move_packet.type = CS_PACKET_PLAYER_MOVE;
		move_packet.pos = m_player->GetPosition();
		move_packet.velocity = m_player->GetVelocity();
		//move_packet.yaw = m_yaw;
		send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
		cout << " x: " << m_player->GetPosition().x << " y: " << m_player->GetPosition().y <<
			" z: " << m_player->GetPosition().z << endl;
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
	    m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * 10.0f));
		CS_PLAYER_MOVE_PACKET move_packet;
		move_packet.size = sizeof(move_packet);
		move_packet.type = CS_PACKET_PLAYER_MOVE;
		move_packet.pos = m_player->GetPosition();
		move_packet.velocity = m_player->GetVelocity();
		//move_packet.yaw = m_yaw;
		send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
		cout << " x: " << m_player->GetPosition().x << " y: " << m_player->GetPosition().y <<
			" z: " << m_player->GetPosition().z << endl;
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetUp(), timeElapsed * 10.0f));
	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetUp(), timeElapsed * -10.0f));
	}


#endif // USE_NETWORK

	
#ifndef USE_NETWORK
	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * 10.0f));
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * -10.0f));
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * -10.0f));
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * 10.0f));
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetUp(), timeElapsed * 10.0f));
	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetUp(), timeElapsed * -10.0f));
	}
#endif // USE_NETWORK

}

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
{
	// 플레이어 생성
	LoadObjectFromFile(device, commandlist, TEXT("./Resource/Model/Warrior.bin"));

	m_player->CreateAnimationController(device, commandlist, 2);
	m_player->SetAnimationSet(m_animations["WarriorAnimation"]);
	m_player->SetAnimationOnTrack(0, 0);
	m_player->SetAnimationOnTrack(1, 1);
	m_player->GetAnimationController()->SetTrackEnable(1, false);

	m_player->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
	m_shaders["PLAYER"]->SetPlayer(m_player);

	// 카메라 생성
	m_camera = make_shared<ThirdPersonCamera>();
	m_camera->CreateShaderVariable(device, commandlist);
	m_camera->SetEye(XMFLOAT3{ 0.0f, 0.0f, 0.0f });
	m_camera->SetAt(XMFLOAT3{ 0.0f, 0.0f, 1.0f });
	m_camera->SetUp(XMFLOAT3{ 0.0f, 1.0f, 0.0f });
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, aspectRatio, 0.1f, 1000.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 지형 생성
	auto field{ make_shared<Field>(device, commandlist, 51, 51, 0, 51, 51, 0, XMFLOAT3{ 1.f, 1.f, 1.f }) };
	field->SetPosition(XMFLOAT3{ -25.0f, 0.f, -25.0f });
	field->SetTexture(m_textures["FIELD"]);
	static_pointer_cast<DetailShader>(m_shaders["FIELD"])->SetField(field);

	// 펜스 생성
	auto fence{ make_shared<Fence>(device, commandlist, 50, 50, 2, 2) };
	fence->SetTexture(m_textures["FENCE"]);
	m_shaders["FENCE"]->GetGameObjects().push_back(fence);

	// 스카이박스 생성
	auto skybox{ make_shared<Skybox>(device, commandlist, 20.0f, 20.0f, 20.0f) };
	skybox->SetTexture(m_textures["SKYBOX"]);
	m_shaders["SKYBOX"]->GetGameObjects().push_back(skybox);

	// 오브젝트 설정	
	m_object.push_back(skybox);
	m_object.push_back(field);
	m_object.push_back(fence);
}

void TowerScene::Update(FLOAT timeElapsed)
{
	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetGameObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shaders)
		shader.second->Update(timeElapsed);

	CheckBorderLimit();
}

void TowerScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	m_shaders.at("PLAYER")->Render(commandList);
	m_shaders.at("SKYBOX")->Render(commandList);
	m_shaders.at("FIELD")->Render(commandList);
	m_shaders.at("FENCE")->Render(commandList);
}

void TowerScene::LoadObjectFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {
			auto object = make_shared<Player>();
			object->LoadObject(device, commandList, in);
			m_player = object;
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void TowerScene::CheckBorderLimit()
{
	XMFLOAT3 pos = m_player->GetPosition();
	if (pos.x > 25.f) {
		m_player->SetPosition(XMFLOAT3{ 25.f, pos.y, pos.z });
	}
	if (pos.x < -25.f) {
		m_player->SetPosition(XMFLOAT3{ -25.f, pos.y, pos.z });
	}
	if (pos.z > 25.f) {
		m_player->SetPosition(XMFLOAT3{ pos.x, pos.y, 25.f });
	}
	if (pos.z < -25.f) {
		m_player->SetPosition(XMFLOAT3{ pos.x, pos.y, -25.f });
	}
}

void TowerScene::SendPlayerData()
{
#ifdef USE_NETWORK
	CS_PLAYER_MOVE_PACKET move_packet;
	move_packet.size = sizeof(move_packet);
	move_packet.type = CS_PACKET_PLAYER_MOVE;
	move_packet.pos = m_player->GetPosition();
	move_packet.velocity = m_player->GetVelocity();
	//move_packet.yaw = m_yaw;
	send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
#endif
}

void TowerScene::RecvPacket()
{
	constexpr char name[10] = "HSC\0";
	CS_LOGIN_PACKET login_packet{};
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	memcpy(login_packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);
	cout << login_packet.name << endl;
	//while (!m_isReadyToPlay && !m_isLogout)
	ProcessPacket();

}

void TowerScene::ProcessPacket()
{
	char buf[2]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recv_byte{ 0 }, recv_flag{ 0 };
	if (WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr) == SOCKET_ERROR)
		ErrorDisplay("RecvSizeType");

	UCHAR size{ static_cast<UCHAR>(buf[0]) };
	UCHAR type{ static_cast<UCHAR>(buf[1]) };

	switch (type)
	{
	case SC_PACKET_LOGIN_OK:
		RecvLoginOkPacket();
		break;
	case SC_PACKET_UPDATE_CLIENT:
		RecvUpdateClient();
		break;
	}

}

void TowerScene::RecvLoginOkPacket()
{

	// 플레이어정보 + 닉네임 
	char buf[sizeof(PlayerData) + NAME_SIZE]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recv_byte{}, recv_flag{};
	WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr);

	if (!m_player) return;
	cout << "넘어 왔나" << endl;
	PlayerData pl_data{};
	char name[NAME_SIZE]{};
	memcpy(&pl_data, buf, sizeof(pl_data));
	memcpy(&name, &buf[sizeof(PlayerData)], sizeof(name));

	// 다른 플레이어가 들어오면 옆에 위치시키게
	for (auto& p : m_multiPlayers)
	{
		if (p) continue;
		//p = make_shared<Player>(TRUE);
		p->SetId(static_cast<int>(pl_data.id));

		if (m_left_other_player_id == -1)
		{
			m_left_other_player_id = static_cast<int>(pl_data.id);
			//p->LoadGeometry(device, commandlist, TEXT("./Resource/Model/Warrior.bin"));
			m_shaders["PLAYER"]->SetPlayer(m_player);
			p->Move(XMFLOAT3{ 3.0f, 0.0f, -3.0f });
		}
		else if (m_right_other_player_id == -1)
		{
			m_right_other_player_id = static_cast<int>(pl_data.id);
			//p->LoadGeometry(device, commandlist, TEXT("./Resource/Model/Warrior.bin"));
			m_shaders["PLAYER"]->SetPlayer(m_player);
			p->Move(XMFLOAT3{ -3.0f, 0.0f, -3.0f });
		}
		break;
	}

}

void TowerScene::RecvUpdateClient()
{
	char subBuf[sizeof(PlayerData) * MAX_USER]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	// 모든 플레이어의 데이터
	array<PlayerData, MAX_USER> data;
	memcpy(&data, subBuf, sizeof(PlayerData) * MAX_USER);

	// 멀티플레이어 업데이트
	unique_lock<mutex> lock{ g_mutex };
	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		for (auto& d : data)
		{
			if (!d.active_check) continue;
			if (p->GetId() != d.id) continue;
		}
	}
}



