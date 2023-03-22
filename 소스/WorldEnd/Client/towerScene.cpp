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

void TowerScene::OnCreate(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature)
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

	// 카메라 버퍼 포인터
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
}

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	CreateShaderVariable(device, commandlist);

	// 플레이어 생성
	m_player = make_shared<Player>();
	m_player->SetType(PlayerType::WARRIOR);
	LoadPlayerFromFile(m_player);

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

	// 그림자 맵 생성
	m_shadow = make_unique<Shadow>(device, 4096 << 1, 4096 << 1);

	// 씬 로드
	LoadSceneFromFile(TEXT("./Resource/Scene/TowerScene.bin"), TEXT("TowerScene"));

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

	InitServer();
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

void TowerScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);
	m_shaders.at("ANIMATION")->Render(commandList);
	m_shaders.at("OBJECT")->Render(commandList);
	m_shaders.at("SKYBOX")->Render(commandList);
	m_shaders.at("HPBAR")->Render(commandList);
	//m_shaders.at("DEBUG")->Render(commandList);
}

void TowerScene::RenderShadow(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ID3D12DescriptorHeap* ppHeaps[] = { m_shadow->GetSrvDiscriptorHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(8, m_shadow->GetGpuSrv());

	commandList->RSSetViewports(1, &m_shadow->GetViewport());
	commandList->RSSetScissorRects(1, &m_shadow->GetScissorRect());

	// DEPTH_WRITE로 바꾼다.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadow->GetShadowMap().Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 후면 버퍼와 깊이 버퍼를 지운다.
	commandList->ClearDepthStencilView(m_shadow->GetCpuDsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

	// 장면을 깊이 버퍼에만 렌더링할 것이므로 렌더 타겟은 nullptr로 설정한다.
	// 이처럼 nullptr 렌더 타겟을 설정하면 색상 쓰기가 비활성화된다.
	// 반드시 활성 PSO의 렌더 타겟 개수도 0으로 지정해야 함을 주의해야 한다.
	commandList->OMSetRenderTargets(0, nullptr, false, &m_shadow->GetCpuDsv());

	m_shaders["ANIMATION"]->Render(commandList, m_shaders["ANIMATIONSHADOW"]);
	m_shaders["OBJECT"]->Render(commandList, m_shaders["SHADOW"]);

	// 셰이더에서 텍스처를 읽을 수 있도록 다시 GENERIC_READ로 바꾼다.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadow->GetShadowMap().Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void TowerScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
}

void TowerScene::LoadSceneFromFile(wstring fileName, wstring sceneName)
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

		LoadObjectFromFile(strPath, object);

		XMFLOAT4X4 worldMatrix;
		in.read((CHAR*)(&worldMatrix), sizeof(XMFLOAT4X4));
		object->SetWorldMatrix(worldMatrix);

		m_object.push_back(object);
		m_shaders["OBJECT"]->SetObject(object);
	}
}

void TowerScene::LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object)
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

void TowerScene::LoadPlayerFromFile(const shared_ptr<Player>& player)
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
	
	LoadObjectFromFile(filePath, player);

	player->SetAnimationSet(m_animationSets[animationSet]);
	player->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	player->GetAnimationController()->SetTrackEnable(1, false);
	player->GetAnimationController()->SetTrackEnable(2, false);
}

void TowerScene::LoadMonsterFromFile(const shared_ptr<Monster>& monster)
{
	wstring filePath{};
	string animationSet{};

	switch (monster->GetType()) {
	case MonsterType::WARRIOR:
		filePath = TEXT("./Resource/Model/Undead_Warrior.bin");
		animationSet = "Undead_WarriorAnimation";
		break;

	case MonsterType::ARCHER:
		filePath = TEXT("./Resource/Model/Undead_Archer.bin");
		animationSet = "Undead_ArcherAnimation";
		break;

	case MonsterType::WIZARD:
		filePath = TEXT("./Resource/Model/Undead_Wizard.bin");
		animationSet = "Undead_WizardAnimation";
		break;
	}

	LoadObjectFromFile(filePath, monster);

	monster->SetAnimationSet(m_animationSets[animationSet]);
	monster->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	monster->GetAnimationController()->SetTrackEnable(1, false);
	monster->GetAnimationController()->SetTrackEnable(2, false);
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
	login_packet.player_type = m_player->GetType();
	memcpy(login_packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);

	g_networkThread = thread{ &TowerScene::RecvPacket, this};
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
	//cout << "[Process Packet] Packet Type: " << (int)ptr[1] << endl;//test

	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
		RecvLoginOkPacket(ptr);
		break;
	case SC_PACKET_ADD_OBJECT:
		RecvAddObjectPacket(ptr);
		break;
	case SC_PACKET_REMOVE_OBJECT:
		RecvRemoveObjectPacket(ptr);
		break;
	case SC_PACKET_UPDATE_CLIENT:
		RecvUpdateClient(ptr);
		break;
	case SC_PACKET_ADD_MONSTER:
		RecvAddMonsterPacket(ptr);
		break;
	case SC_PACKET_UPDATE_MONSTER:
		RecvUpdateMonster(ptr);
		break;
	case SC_PACKET_CHANGE_MONSTER_BEHAVIOR:
		RecvChangeMonsterBehavior(ptr);
		break;
	case SC_PACKET_CHANGE_ANIMATION:
		RecvChangeAnimation(ptr);
		break;
	case SC_PACKET_RESET_COOLTIME:
		RecvResetCooltime(ptr);
		break;
	}
}



void TowerScene::PacketReassembly(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t make_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];
	ZeroMemory(packet_buffer, BUF_SIZE);

	while (io_byte != 0) {
		if (make_packet_size == 0)
			make_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= make_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, make_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
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
	SC_LOGIN_OK_PACKET* packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(ptr);
	m_player->SetID(packet->player_data.id);
}

void TowerScene::RecvAddObjectPacket(char* ptr)
{
	SC_ADD_OBJECT_PACKET* packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);

	PLAYER_DATA player_data{};

	player_data.hp = packet->player_data.hp;
	player_data.id = packet->player_data.id;
	player_data.pos = packet->player_data.pos;

	auto multiPlayer = make_shared<Player>();
	multiPlayer->SetType(packet->player_type);
	LoadPlayerFromFile(multiPlayer);

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

void TowerScene::RecvRemoveObjectPacket(char* ptr)
{
	SC_REMOVE_OBJECT_PACKET* packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);

	m_multiPlayers[packet->id]->SetPosition(XMFLOAT3(10000.f, 10000.f, 10000.f));
	cout << "Erase" << endl;
}

void TowerScene::RecvUpdateClient(char* ptr)
{
	SC_UPDATE_CLIENT_PACKET* packet = reinterpret_cast<SC_UPDATE_CLIENT_PACKET*>(ptr);

	for (int i = 0; i < MAX_INGAME_USER; ++i) {
		if (-1 == packet->data[i].id) {
			continue;
		}

		if (packet->data[i].id == m_player->GetID()) {
			m_player->SetPosition(packet->data[i].pos);
			continue;
		}
		else {
			// towerScene의 multiPlayer를 업데이트 해도 shader의 multiPlayer도 업데이트 됨.
			XMFLOAT3 playerPosition = packet->data[i].pos;
			auto& player = m_multiPlayers[packet->data[i].id];
			player->SetPosition(playerPosition);
			player->SetVelocity(packet->data[i].velocity);
			player->Rotate(0.f, 0.f, packet->data[i].yaw - player->GetYaw());
			player->SetHp(packet->data[i].hp);
		}
	}
}

void TowerScene::RecvAddMonsterPacket(char* ptr)
{
	SC_ADD_MONSTER_PACKET* packet = reinterpret_cast<SC_ADD_MONSTER_PACKET*>(ptr);

	MONSTER_DATA monster_data{};

	monster_data.id = packet->monster_data.id;
	monster_data.pos = packet->monster_data.pos;
	monster_data.hp = packet->monster_data.hp;

	if (monster_data.id == -1)
		return;

	auto monster = make_shared<Monster>();
	monster->SetType(packet->monster_type);
	LoadMonsterFromFile(monster);
	monster->ChangeAnimation(ObjectAnimation::WALK);

	monster->SetPosition(XMFLOAT3{ monster_data.pos.x, monster_data.pos.y, monster_data.pos.z });
	m_monsters.insert({ static_cast<INT>(monster_data.id), monster });

	auto hpBar = make_shared<HpBar>();
	hpBar->SetMesh(m_meshs["HPBAR"]);
	hpBar->SetTexture(m_textures["HPBAR"]);
	m_shaders["HPBAR"]->SetObject(hpBar);
	monster->SetHpBar(hpBar);

	m_shaders["ANIMATION"]->SetMonster(monster_data.id, monster);
}

void TowerScene::RecvUpdateMonster(char* ptr)
{
	SC_UPDATE_MONSTER_PACKET* packet = reinterpret_cast<SC_UPDATE_MONSTER_PACKET*>(ptr);

	if (packet->monster_data.id < 0) return;

	auto& monster = m_monsters[packet->monster_data.id];
	monster->SetPosition(packet->monster_data.pos);
	monster->SetHp(packet->monster_data.hp);

	FLOAT newYaw = packet->monster_data.yaw;
	FLOAT oldYaw = monster->GetYaw();
	monster->Rotate(0.f, 0.f, newYaw - oldYaw);
}

void TowerScene::RecvChangeMonsterBehavior(char* ptr)
{
	SC_CHANGE_MONSTER_BEHAVIOR_PACKET* packet =
		reinterpret_cast<SC_CHANGE_MONSTER_BEHAVIOR_PACKET*>(ptr);

	auto& monster = m_monsters[packet->id];
	monster->ChangeAnimation(packet->animation);
	//monster->ChangeBehavior(packet->behavior);
}

void TowerScene::RecvResetCooltime(char* ptr)
{
	SC_RESET_COOLTIME_PACKET* packet = reinterpret_cast<SC_RESET_COOLTIME_PACKET*>(ptr);
	m_player->ResetCooltime(packet->cooltime_type);
}

void TowerScene::RecvChangeAnimation(char* ptr)
{
	SC_CHANGE_ANIMATION_PACKET* packet = reinterpret_cast<SC_CHANGE_ANIMATION_PACKET*>(ptr);

	m_multiPlayers[packet->id]->ChangeAnimation(packet->animation_type, false);
}
