#include "scene.h"
#include "Connect.h"

Scene::Scene()
{
#ifdef USE_NETWORK
	//Init();
	//ConnectTo();

	cout.imbue(locale("korean"));
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 0), &WSAData) != 0) {
		cout << "WSA START ERROR!!" << endl;
	}

	m_c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (m_c_socket == INVALID_SOCKET) {
		cout << "SOCKET INIT ERROR!!" << endl;
	}
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);
	connect(m_c_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

	CS_LOGIN_PACKET login_packet;
	login_packet.size = sizeof(CS_LOGIN_PACKET);
	login_packet.type = CS_LOGIN;
	strcpy_s(login_packet.name, "SU");
	SendPacket(&login_packet);
	//send(m_c_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), 0);

	RecvPacket();
#endif // USE_NETWORK
}

Scene::~Scene()
{
}

void Scene::DoSend()
{
	
}

void Scene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const
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

void Scene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const
{
#ifdef USE_NETWORK

	char packetDirection = 0;
	

	if (GetAsyncKeyState('W') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * 10.0f));
		packetDirection += INPUT_KEY_W;
	

	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * -10.0f));
		packetDirection += INPUT_KEY_A;
	
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * -10.0f));
		packetDirection += INPUT_KEY_S;
	

	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * 10.0f));
		packetDirection += INPUT_KEY_D;
	}
	if (packetDirection) {
		CS_INPUT_KEY_PACKET key_packet;
		key_packet.size = sizeof(key_packet);
		key_packet.type = CS_MOVE;
		key_packet.direction = packetDirection;  // 입력받은 키값 전달
		SendPacket(&key_packet);
		//cout << "Do Send" << endl;
//		int ret = send(m_connect.m_c_socket, reinterpret_cast<char*>(&key_packet), sizeof(key_packet), 0);
		RecvPacket();
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
#endif // !USE_NETWORK

}



void Scene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
{

	//m_connect = make_shared<Connect>();

	vector<TextureVertex> vertices;
	vector<UINT> indices;

	// right
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));
	
	// left
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));

	// top
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));

	// bottom
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));

	// back
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));

	// front
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));

	// 플레이어 생성

	m_player = make_shared<Player>();
	auto playerShader{ make_unique<Shader>(device, rootsignature) };
	auto playerTexture{ make_shared<Texture>() };
	auto playerMesh{ make_shared<Mesh>(device, commandlist, vertices, indices) };
	playerTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Bricks.dds"), 2);
	playerTexture->CreateSrvDescriptorHeap(device);
	playerTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	m_player->SetTexture(playerTexture);
	m_player->SetMesh(playerMesh);
	m_player->SetPosition(XMFLOAT3{ 0.f, 0.5f, 0.f });
	playerShader->SetPlayer(m_player);

	for (int i = 0; i < MAX_USER; ++i) {
		m_multiPlayers[i] = make_shared<Player>();
		m_multiPlayers[i]->SetTexture(playerTexture);
		m_multiPlayers[i]->SetMesh(playerMesh);
		m_multiPlayers[i]->SetPosition(XMFLOAT3{ 0.f, 0.5f, 0.f });
		playerShader->SetMultiPlayer(m_multiPlayers[i]);
	}

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
	auto detailShader{ make_unique<DetailShader>(device, rootsignature) };
	auto field{ make_shared<Field>(device, commandlist, 51, 51, 0, 51, 51, 0, XMFLOAT3{ 1.f, 1.f, 1.f })};
	auto fieldTexture{ make_shared<Texture>()};
	fieldTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Base_Texture.dds"), 2); // BaseTexture
	fieldTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Detail_Texture.dds"), 3); // DetailTexture
	fieldTexture->CreateSrvDescriptorHeap(device);
	fieldTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	field->SetPosition(XMFLOAT3{ -25.0f, 0.f, -25.0f });
	field->SetTexture(fieldTexture);
	detailShader->SetField(field);

	// 펜스 생성
	auto blendingShader{ make_unique<BlendingShader>(device, rootsignature) };
	auto fence{ make_shared<Fence>(device, commandlist, 50, 50, 2, 2) };
	auto fenceTexture{ make_shared<Texture>() };
	fenceTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Fence.dds"), 2); // BaseTexture
	fenceTexture->CreateSrvDescriptorHeap(device);
	fenceTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	fence->SetPosition(XMFLOAT3{ 0.f, 1.f, 0.f });
	fence->SetTexture(fenceTexture);
	blendingShader->GetGameObjects().push_back(fence);

	// 스카이박스 생성
	auto skyboxShader{ make_unique<SkyboxShader>(device, rootsignature) };
	auto skybox{ make_shared<Skybox>(device, commandlist, 20.0f, 20.0f, 20.0f) };
	auto skyboxTexture{ make_shared<Texture>()};
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), 4);
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);
	skybox->SetTexture(skyboxTexture);
	skyboxShader->GetGameObjects().push_back(skybox);

	// 셰이더 설정
	m_shader.insert(make_pair("PLYAER", move(playerShader)));
	m_shader.insert(make_pair("SKYBOX", move(skyboxShader)));
	m_shader.insert(make_pair("DETAIL", move(detailShader)));
	m_blending.insert(make_pair("FENCE", move(blendingShader)));
}

void Scene::Update(FLOAT timeElapsed)
{
	m_camera->Update(timeElapsed);
	if (m_shader["SKYBOX"]) for (auto& skybox : m_shader["SKYBOX"]->GetGameObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shader)
		shader.second->Update(timeElapsed);
	for (const auto& shader : m_blending)
		shader.second->Update(timeElapsed);

	CheckBorderLimit();

	XMFLOAT3 temp = { my_info.m_x, my_info.m_y, my_info.m_z };
	m_player->SetPosition(temp);
}

void Scene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	for (const auto& shader : m_shader) { shader.second->Render(commandList); }
	for (const auto& shader : m_blending) { shader.second->Render(commandList); }
}

void Scene::CheckBorderLimit()
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
