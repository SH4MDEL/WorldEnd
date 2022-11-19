#include "scene.h"


Scene::~Scene()
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
}

void Scene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
{
	vector<TextureVertex> vertices;
	vector<UINT> indices;

	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));
								   
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));

	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));

	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));

	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT2(1.0f, 0.0f));

	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT2(0.0f, 0.0f));
	vertices.emplace_back(XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT2(1.0f, 0.0f));

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
	unique_ptr<TerrainShader> detailShader{ make_unique<TerrainShader>(device, rootsignature) };
	shared_ptr<Field> field{
		make_shared<Field>(device, commandlist, 30, 30, 0, 30, 30, XMFLOAT3{ 1.f, 1.f, 1.f })
	};
	shared_ptr<Texture> fieldTexture{
		make_shared<Texture>()
	};
	fieldTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Base_Texture.dds"), 2); // BaseTexture
	fieldTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Detail_Texture.dds"), 3); // DetailTexture
	fieldTexture->CreateSrvDescriptorHeap(device);
	fieldTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	field->SetPosition(XMFLOAT3{ -15.0f, 0.0f, -15.0f });
	field->SetTexture(fieldTexture);
	detailShader->SetField(field);

	// 스카이박스 생성
	unique_ptr<SkyboxShader> skyboxShader = make_unique<SkyboxShader>(device, rootsignature);
	shared_ptr<Skybox> skybox{ make_shared<Skybox>(device, commandlist, 20.0f, 20.0f, 20.0f) };
	shared_ptr<Texture> skyboxTexture{
		make_shared<Texture>()
	};
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), 4);
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);
	skybox->SetTexture(skyboxTexture);
	skyboxShader->GetGameObjects().push_back(skybox);

	// 셰이더 설정
	m_shader.insert(make_pair("PLYAER", move(playerShader)));
	m_shader.insert(make_pair("SKYBOX", move(skyboxShader)));
	m_shader.insert(make_pair("DETAIL", move(detailShader)));
}

void Scene::Update(FLOAT timeElapsed)
{
	m_camera->Update(timeElapsed);
	if (m_shader["SKYBOX"]) for (auto& skybox : m_shader["SKYBOX"]->GetGameObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shader)
		shader.second->Update(timeElapsed);
}

void Scene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	for (const auto& shader : m_shader) { shader.second->Render(commandList); }
}
