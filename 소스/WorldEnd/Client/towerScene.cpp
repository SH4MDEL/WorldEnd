#include "towerScene.h"

TowerScene::TowerScene() : 
	m_NDCspace( 0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, -0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, 0.0f, 1.0f)
{}

TowerScene::~TowerScene()
{

}

void TowerScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature)
{
	BuildObjects(device, commandList, rootSignature);
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

void TowerScene::OnProcessingClickMessage(LPARAM lParam) const
{
	if (m_player) m_player->OnProcessingClickMessage(lParam);
}

void TowerScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const
{
	if (m_player) m_player->OnProcessingKeyboardMessage(timeElapsed);
}

void TowerScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_sceneBuffer)));

	m_sceneBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_sceneBufferPointer));
}

void TowerScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	BoundingSphere sceneBounds{ XMFLOAT3{ 0.0f, 0.0f, 10.0f }, 60.f };

	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&m_lightSystem->m_lights[0].m_direction);
	XMVECTOR lightPos{ -2.0f * sceneBounds.Radius * lightDir };
	XMVECTOR targetPos = XMLoadFloat3(&sceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, m_lightView));

	float l = sphereCenterLS.x - sceneBounds.Radius;
	float b = sphereCenterLS.y - sceneBounds.Radius;
	float n = sphereCenterLS.z - sceneBounds.Radius;
	float r = sphereCenterLS.x + sceneBounds.Radius;
	float t = sphereCenterLS.y + sceneBounds.Radius;
	float f = sphereCenterLS.z + sceneBounds.Radius;

	m_lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	::memcpy(&m_sceneBufferPointer->lightView, &XMMatrixTranspose(m_lightView), sizeof(XMFLOAT4X4));
	::memcpy(&m_sceneBufferPointer->lightProj, &XMMatrixTranspose(m_lightProj), sizeof(XMFLOAT4X4));
	::memcpy(&m_sceneBufferPointer->NDCspace, &XMMatrixTranspose(m_NDCspace), sizeof(XMFLOAT4X4));

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_sceneBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(4, virtualAddress);

	//if (m_camera) m_camera->UpdateShaderVariable(commandList);
	//if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);
}

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	CreateShaderVariable(device, commandlist);

	// 플레이어 생성
	m_player = make_shared<Player>();
	m_player->SetType(PlayerType::WARRIOR);
	LoadPlayerFromFile(device, commandlist, m_player);

	m_player->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
	m_shaders["ANIMATION"]->SetPlayer(m_player);

	// 체력 바 생성
	auto hpBar = make_shared<HpBar>();
	hpBar->SetMesh(m_meshs["HPBAR"]);
	hpBar->SetTexture(m_textures["HPBAR"]);
	m_shaders["HPBAR"]->SetObject(hpBar);
	m_player->SetHpBar(hpBar);

	// 카메라 생성
	m_camera = make_shared<ThirdPersonCamera>();
	m_camera->CreateShaderVariable(device, commandlist);
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 100.0f));
	m_camera->SetProjMatrix(projMatrix);

	m_shadow = make_shared<Shadow>(device, 4096 << 1, 4096 << 1);

	// 씬 로드
	LoadSceneFromFile(device, commandlist, TEXT("./Resource/Scene/TowerScene.bin"), TEXT("TowerScene"));

	// 스카이 박스 생성
	auto skybox{ make_shared<GameObject>() };
	skybox->SetMesh(m_meshs["SKYBOX"]);
	skybox->SetTexture(m_textures["SKYBOX"]);
	m_shaders["SKYBOX"]->SetObject(skybox);

	// 디버그 오브젝트 생성
	auto debugObject{ make_shared<GameObject>() };
	debugObject->SetMesh(m_meshs["DEBUG"]);
	m_shaders["DEBUG"]->SetObject(debugObject);

	// 오브젝트 설정	
	m_object.push_back(skybox);

	// 조명 생성
	CreateLight(device, commandlist);

	InitServer(device, commandlist);
}

void TowerScene::CreateLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	m_lightSystem = make_shared<LightSystem>();
	m_lightSystem->m_globalAmbient = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.f };
	m_lightSystem->m_numLight = 8;

	m_lightSystem->m_lights[0].m_type = DIRECTIONAL_LIGHT;
	m_lightSystem->m_lights[0].m_ambient = XMFLOAT4{ 0.3f, 0.3f, 0.3f, 1.f };
	m_lightSystem->m_lights[0].m_diffuse = XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.f };
	m_lightSystem->m_lights[0].m_specular = XMFLOAT4{ 0.4f, 0.4f, 0.4f, 0.f };
	m_lightSystem->m_lights[0].m_direction = XMFLOAT3{ 1.f, -1.f, 1.f };
	m_lightSystem->m_lights[0].m_enable = true;

	m_lightSystem->m_lights[1].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[1].m_range = 5.f;
	m_lightSystem->m_lights[1].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[1].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[1].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[1].m_position = XMFLOAT3(-2.943f, -2.468f, -45.335f);
	m_lightSystem->m_lights[1].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[1].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[1].m_falloff = 3.f;
	m_lightSystem->m_lights[1].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[1].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[1].m_enable = true;

	m_lightSystem->m_lights[2].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[2].m_range = 5.f;
	m_lightSystem->m_lights[2].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[2].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[2].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[2].m_position = XMFLOAT3(-2.963f, -2.468f, -33.575f);
	m_lightSystem->m_lights[2].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[2].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[2].m_falloff = 3.f;
	m_lightSystem->m_lights[2].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[2].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[2].m_enable = true;

	m_lightSystem->m_lights[3].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[3].m_range = 5.f;
	m_lightSystem->m_lights[3].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[3].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[3].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[3].m_position = XMFLOAT3(-2.755f, -2.468f, -21.307f);
	m_lightSystem->m_lights[3].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[3].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[3].m_falloff = 3.f;
	m_lightSystem->m_lights[3].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[3].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[3].m_enable = true;

	m_lightSystem->m_lights[4].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[4].m_range = 5.f;
	m_lightSystem->m_lights[4].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[4].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[4].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[4].m_position = XMFLOAT3(-4.833f, 1.952f, 8.224f);
	m_lightSystem->m_lights[4].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[4].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[4].m_falloff = 3.f;
	m_lightSystem->m_lights[4].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[4].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[4].m_enable = true;

	m_lightSystem->m_lights[5].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[5].m_range = 5.f;
	m_lightSystem->m_lights[5].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[5].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[5].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[5].m_position = XMFLOAT3(4.796f, 1.952f, 8.044f);
	m_lightSystem->m_lights[5].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[5].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[5].m_falloff = 3.f;
	m_lightSystem->m_lights[5].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[5].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[5].m_enable = true;

	m_lightSystem->m_lights[6].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[6].m_range = 5.f;
	m_lightSystem->m_lights[6].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[6].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[6].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[6].m_position = XMFLOAT3(-4.433f, 1.952f, 39.324f);
	m_lightSystem->m_lights[6].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[6].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[6].m_falloff = 3.f;
	m_lightSystem->m_lights[6].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[6].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[6].m_enable = true;

	m_lightSystem->m_lights[7].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[7].m_range = 5.f;
	m_lightSystem->m_lights[7].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[7].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[7].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[7].m_position = XMFLOAT3(3.116f, 1.952f, 39.184f);
	m_lightSystem->m_lights[7].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[7].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[7].m_falloff = 3.f;
	m_lightSystem->m_lights[7].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[7].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[7].m_enable = true;

	m_lightSystem->CreateShaderVariable(device, commandlist);
}

void TowerScene::Update(FLOAT timeElapsed)
{
	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shaders)
		shader.second->Update(timeElapsed);
}

void TowerScene::Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);

	switch (threadIndex)
	{
	case 0:
	{
		m_shaders.at("ANIMATION")->Render(device, commandList);
		break;
	}
	case 1:
	{
		m_shaders.at("OBJECT")->Render(device, commandList);
		break;
	}
	case 2:
	{
		m_shaders.at("SKYBOX")->Render(device, commandList);
		m_shaders.at("HPBAR")->Render(device, commandList);
		//m_shaders.at("DEBUG")->Render(device, commandList);
		break;
	}
	}
}


void TowerScene::RenderShadow(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex)
{

	switch (threadIndex)
	{
	case 0:
	{
		m_shaders["ANIMATION"]->Render(device, commandList, m_shaders["ANIMATIONSHADOW"]);
		break;
	}
	case 1:
	{
		m_shaders["OBJECT"]->Render(device, commandList, m_shaders["SHADOW"]);
		break;
	}
	case 2:
	{
		break;
	}
	}
}

void TowerScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
}

void TowerScene::LoadSceneFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, wstring fileName, wstring sceneName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;
	in.read((CHAR*)(&strLength), sizeof(BYTE));
	string strToken(strLength, '\0');
	in.read(&strToken[0], sizeof(CHAR) * strLength);
	INT objectCount;
	in.read((CHAR*)(&objectCount), sizeof(INT));

	for (int i = 0; i < objectCount; ++i) {
		auto object = make_shared<GameObject>();

		in.read((CHAR*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(CHAR) * strLength);

		in.read((CHAR*)(&strLength), sizeof(BYTE));
		string objectName(strLength, '\0');
		in.read(&objectName[0], sizeof(CHAR) * strLength);

		wstring wstrToken = L"";
		wstrToken.assign(objectName.begin(), objectName.end());
		wstring strPath = L"./Resource/Model/" + sceneName + L"/" + wstrToken + L".bin";

		LoadObjectFromFile(device, commandlist, strPath, object);

		XMFLOAT4X4 worldMatrix;
		in.read((CHAR*)(&worldMatrix), sizeof(XMFLOAT4X4));
		object->SetWorldMatrix(worldMatrix);

		m_object.push_back(object);
		m_shaders["OBJECT"]->SetObject(object);
	}
}

void TowerScene::LoadObjectFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName, const shared_ptr<GameObject>& object)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {
			object->LoadObject(device, commandList, in);
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void TowerScene::LoadPlayerFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Player>& player)
{
	wstring filePath{};
	string animationSet{};

	switch (player->GetType()) {
	case PlayerType::WARRIOR:
		filePath = TEXT("./Resource/Model/Warrior.bin");
		animationSet = "WarriorAnimation";
		break;

	case PlayerType::ARCHER:
		filePath = TEXT("./Resource/Model/Archer.bin");
		animationSet = "ArcherAnimation";
		break;
	}
	
	LoadObjectFromFile(device, commandList, filePath, player);

	player->SetAnimationSet(m_animationSets[animationSet]);
	player->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	player->GetAnimationController()->SetTrackEnable(1, false);
	player->GetAnimationController()->SetTrackEnable(2, false);
}


void TowerScene::InitServer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
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

	constexpr char name[10] = "HSC\0";
	CS_LOGIN_PACKET login_packet{};
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	memcpy(login_packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);

	g_networkThread = thread{ &TowerScene::RecvPacket, this, device, commandList};
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

void TowerScene::RecvPacket(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	while (true) {

		char buf[BUF_SIZE] = {0};
		WSABUF wsabuf{ BUF_SIZE, buf };
		DWORD recv_byte{ 0 }, recv_flag{ 0 };

		if (WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr) == SOCKET_ERROR)
			ErrorDisplay("RecvSizeType");

		if(recv_byte > 0)
			PacketReassembly(device, commandList, wsabuf.buf, recv_byte);

	}
}


void TowerScene::ProcessPacket(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, char* ptr)
{
	cout << "[Process Packet] Packet Type: " << (int)ptr[1] << endl;//test

	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
		RecvLoginOkPacket(ptr);
		break;
	case SC_PACKET_ADD_PLAYER:
		RecvAddPlayerPacket(device, commandList, ptr);
		break;
	case SC_PACKET_UPDATE_CLIENT:
		RecvUpdateClient(ptr);
		break;
	case SC_PACKET_PLAYER_ATTACK:
		RecvAttackPacket(ptr);
		break;
	case SC_PACKET_ADD_MONSTER:
		RecvAddMonsterPacket(ptr);
		break;
	case SC_PACKET_UPDATE_MONSTER:
		RecvUpdateMonster(ptr);
		break;
	}
}



void TowerScene::PacketReassembly(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, char* net_buf, size_t io_byte)
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
			ProcessPacket(device, commandList, packet_buffer);
			ZeroMemory(packet_buffer, BUF_SIZE);
			ptr += make_packet_size - saved_packet_size;
			io_byte -= make_packet_size - saved_packet_size;
			//cout << "io byte - " << io_byte << endl;
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

void TowerScene::RecvAddPlayerPacket(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, char* ptr)
{
	SC_ADD_PLAYER_PACKET* add_pl_packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(ptr);

	PlayerData player_data{};

	player_data.hp = add_pl_packet->player_data.hp;
	player_data.id = add_pl_packet->player_data.id;
	player_data.pos = add_pl_packet->player_data.pos;

	auto multiPlayer = make_shared<Player>();
	multiPlayer->SetType(PlayerType::ARCHER);
	LoadPlayerFromFile(device, commandList, multiPlayer);

	multiPlayer->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });

	m_multiPlayers.insert({ player_data.id, multiPlayer });

	auto hpBar = make_shared<HpBar>();
	hpBar->SetMesh(m_meshs["HPBAR"]);
	hpBar->SetTexture(m_textures["HPBAR"]);
	m_shaders["HPBAR"]->SetObject(hpBar);
	multiPlayer->SetHpBar(hpBar);

	m_shaders["ANIMATION"]->SetMultiPlayer(player_data.id, multiPlayer);
	cout << "add player" << static_cast<int>(player_data.id) << endl;

}

void TowerScene::RecvUpdateClient(char* ptr)
{
	SC_UPDATE_CLIENT_PACKET* update_packet = reinterpret_cast<SC_UPDATE_CLIENT_PACKET*>(ptr);

	for (int i = 0; i < MAX_USER; ++i) {
		if (update_packet->data[i].id == -1) {
			continue;
		}
		if (update_packet->data[i].id == m_player->GetID()) {
			m_player->SetPosition(update_packet->data[i].pos);
			continue;
		}
		if (update_packet->data[i].active_check) {
			// towerScene의 multiPlayer를 업데이트 해도 shader의 multiPlayer도 업데이트 됨.
			XMFLOAT3 playerPosition = update_packet->data[i].pos;			
			m_multiPlayers[update_packet->data[i].id]->SetPosition(playerPosition);
			m_multiPlayers[update_packet->data[i].id]->SetVelocity(update_packet->data[i].velocity);
			m_multiPlayers[update_packet->data[i].id]->Rotate(0.f, 0.f, update_packet->data[i].yaw - m_multiPlayers[update_packet->data[i].id]->GetYaw());
			m_multiPlayers[update_packet->data[i].id]->SetHp(update_packet->data[i].hp);
		}
	}
}

void TowerScene::RecvAttackPacket(char* ptr)
{
	SC_ATTACK_PACKET* attack_packet = reinterpret_cast<SC_ATTACK_PACKET*>(ptr);
	m_player->SetKey(attack_packet->key);
}

void TowerScene::RecvAddMonsterPacket(char* ptr)
{
	SC_ADD_MONSTER_PACKET* add_monster_packet = reinterpret_cast<SC_ADD_MONSTER_PACKET*>(ptr);

	MonsterData monster_data{};

	//monster_data.hp = add_monster_packet->monster_data.hp;
	monster_data.id = add_monster_packet->monster_data.id;
	monster_data.pos = add_monster_packet->monster_data.pos;

	auto monster = make_shared<Monster>();
	switch (add_monster_packet->monster_type)
	{
	case MonsterType::WARRIOR:
		LoadObjectFromFile(g_GameFramework.GetDevice(), g_GameFramework.GetCommandList(), TEXT("./Resource/Model/Undead_Warrior.bin"), monster);
		break;
	case MonsterType::ARCHER:
		LoadObjectFromFile(g_GameFramework.GetDevice(), g_GameFramework.GetCommandList(), TEXT("./Resource/Model/Undead_Archer.bin"), monster);
		break;
	case MonsterType::WIZARD:
		LoadObjectFromFile(g_GameFramework.GetDevice(), g_GameFramework.GetCommandList(), TEXT("./Resource/Model/Undead_Wizard.bin"), monster);
		break;
	}
	monster->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
	m_monsters.insert({ monster_data.id, monster });

	auto hpBar = make_shared<HpBar>();
	hpBar->SetMesh(m_meshs["HPBAR"]);
	hpBar->SetTexture(m_textures["HPBAR"]);
	m_shaders["HPBAR"]->SetObject(hpBar);
	monster->SetHpBar(hpBar);

	m_shaders["OBJECT"]->SetObject(monster);
	cout << "add monster" << static_cast<int>(monster_data.id) << endl;
}

void TowerScene::RecvUpdateMonster(char* ptr)
{
	SC_MONSTER_UPDATE_PACKET* monster_packet = reinterpret_cast<SC_MONSTER_UPDATE_PACKET*>(ptr);

	if (monster_packet->monster_data.id < 0) return;

	m_monsters[monster_packet->monster_data.id]->SetPosition(monster_packet->monster_data.pos);

	/*cout << "monster id - " << (int)monster_packet->monster_data.id << endl;
	cout << "monster pos (x: " << monster_packet->monster_data.pos.x <<
			" y: " << monster_packet->monster_data.pos.y <<
			" z: " << monster_packet->monster_data.pos.z << ")" << endl;*/

	
}