#include "scene.h"


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
	XMFLOAT4X4 pos = m_player->GetWorldMatrix();

	cs_packet_inputKey packet;
	WSAOVERLAPPED* c_over = new WSAOVERLAPPED;

	if (GetAsyncKeyState('W') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * 10.0f));
		packet.inputKey &= INPUT_KEY_W;
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * -10.0f));
		packet.inputKey &= INPUT_KEY_A;
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), timeElapsed * -10.0f));
		packet.inputKey &= INPUT_KEY_S;

	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		//m_player->AddVelocity(Vector3::Mul(m_player->GetRight(), timeElapsed * 10.0f));
		packet.inputKey &= INPUT_KEY_D;

	}
	if (packet.inputKey |= (INPUT_KEY_W | INPUT_KEY_A | INPUT_KEY_S | INPUT_KEY_D)) {
		int retval = WSASend(m_socket, (WSABUF*)&packet, 1, 0, 0, c_over, NULL);
		cout << "[]: " << packet.id << " dir - " << packet.inputKey << endl;
	}
	delete c_over;
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
