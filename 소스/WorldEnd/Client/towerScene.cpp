#include "towerScene.h"

TowerScene::TowerScene()
{

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

	if (m_camera) m_camera->Rotate(0.f, dy * 5.0f * deltaTime, dx * 5.0f * deltaTime);
	SetCursorPos(prevPosition.x, prevPosition.y);
}

void TowerScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const
{
	m_player->OnProcessingKeyboardMessage(timeElapsed);
}

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
{
	// 플레이어 생성
	m_player = make_shared<Player>();
	LoadObjectFromFile(TEXT("./Resource/Model/Archer.bin"), m_player);
	m_player->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
	m_shaders["PLAYER"]->SetPlayer(m_player);

	// 카메라 생성
	m_camera = make_shared<ThirdPersonCamera>();
	m_camera->CreateShaderVariable(device, commandlist);
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
	m_shaders["FENCE"]->SetObject(fence);

	// 스카이박스 생성
	auto skybox{ make_shared<Skybox>(device, commandlist, 20.0f, 20.0f, 20.0f) };
	skybox->SetTexture(m_textures["SKYBOX"]);
	m_shaders["SKYBOX"]->SetObject(skybox);

	// 오브젝트 설정	
	m_object.push_back(skybox);
	m_object.push_back(field);
	m_object.push_back(fence);

	InitServer();
}

void TowerScene::Update(FLOAT timeElapsed)
{
	
	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
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

void TowerScene::LoadObjectFromFile(wstring fileName, shared_ptr<GameObject> object)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {
			object->LoadObject(in);
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

void TowerScene::InitServer()
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

	// connectwadw
	SOCKADDR_IN server_address{};
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, g_serverIP.c_str(), &(server_address.sin_addr.s_addr));

	connect(g_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address));

	constexpr char name[10] = "HSC\0";
	CS_LOGIN_PACKET login_packet{};
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	memcpy(login_packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);

	g_networkThread = thread{ &TowerScene::RecvPacket, this };
	g_networkThread.detach();

#endif
}

void TowerScene::SendPlayerData()
{
#ifdef USE_NETWORK
	CS_PLAYER_MOVE_PACKET move_packet;
	move_packet.size = sizeof(move_packet);
	move_packet.type = CS_PACKET_PLAYER_MOVE;
	move_packet.pos = m_player->GetPosition();
	move_packet.velocity = m_player->GetVelocity();
	move_packet.yaw = m_player->GetYaw();
	send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
#endif
}

void TowerScene::RecvPacket()
{

	while (true) {

		char buf[BUF_SIZE] = {0};
		WSABUF wsabuf{ BUF_SIZE, buf };
		DWORD recv_byte{ 0 }, recv_flag{ 0 };

		if (WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr) == SOCKET_ERROR)
			ErrorDisplay("RecvSizeType");

		if(recv_byte > 0)
			PacketReassembly(wsabuf.buf, recv_byte);

	}

}


void TowerScene::ProcessPacket(char* ptr)
{
		
		cout << "[Process Packet] Packet Type: " << (int)ptr[1] << endl;//test

		
		switch (ptr[1])
		{
		case SC_PACKET_LOGIN_OK:
			RecvLoginOkPacket(ptr);
			break;
		case SC_PACKET_ADD_PLAYER:
			RecvAddPlayerPacket(ptr);
			break;
		case SC_PACKET_UPDATE_CLIENT:
			RecvUpdateClient(ptr);
			break;
		

	}
}



void TowerScene::PacketReassembly(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t make_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (io_byte != 0) {
		if (make_packet_size == 0)
			make_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= make_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, make_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ZeroMemory(packet_buffer, BUF_SIZE);
			ptr += make_packet_size - saved_packet_size;
			io_byte -= make_packet_size - saved_packet_size;
			cout << "io byte - " << io_byte << endl;
			make_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}

}

void TowerScene::RecvLoginOkPacket(char* ptr)
{
	SC_LOGIN_OK_PACKET* login_packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(ptr);
	m_player->SetID(login_packet->player_data.id);
}

void TowerScene::RecvAddPlayerPacket(char* ptr)
{
	SC_ADD_PLAYER_PACKET* add_pl_packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(ptr);

	PlayerData playerData{};

	playerData.hp = add_pl_packet->player_data.hp;
	playerData.id = add_pl_packet->player_data.id;
	playerData.pos = add_pl_packet->player_data.pos;

	auto multiPlayer = make_shared<Player>();
	LoadObjectFromFile(TEXT("./Resource/Model/Archer.bin"), multiPlayer);
	multiPlayer->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
	m_multiPlayers.insert({ playerData.id, multiPlayer });
	m_shaders["PLAYER"]->SetMultiPlayer(playerData.id, multiPlayer);
	cout << "add player" << static_cast<int>(playerData.id) << endl;

}

void TowerScene::RecvUpdateClient(char* ptr)
{
	SC_UPDATE_CLIENT_PACKET* update_packet = reinterpret_cast<SC_UPDATE_CLIENT_PACKET*>(ptr);

	unique_lock<mutex> lock{ g_mutex };
	for (int i = 0; i < MAX_USER; ++i) {
		if (update_packet->data[i].id == -1) {
			continue;
		}
		if (update_packet->data[i].id == m_player->GetID()) {
			// 클라이언트 내에서 자체적으로 진행하던 업데이트 폐기하고
			// 플레이어 데이터 업데이트
			continue;
		}
		if (update_packet->data[i].active_check) {
			// towerScene의 multiPlayer를 업데이트 해도 shader의 multiPlayer도 업데이트 됨.
			m_multiPlayers[update_packet->data[i].id]->SetPosition(update_packet->data[i].pos);
			m_multiPlayers[update_packet->data[i].id]->SetVelocity(update_packet->data[i].velocity);
			//m_multiPlayers[playerData.id]->Rotate(0.f, 0.f, playerData.yaw - m_multiPlayers[playerData.id]->GetYaw());
		}
	}

}



