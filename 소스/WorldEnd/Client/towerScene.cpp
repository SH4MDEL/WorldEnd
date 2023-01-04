#include "towerScene.h"

TowerScene::~TowerScene()
{
}

void TowerScene::OnCreate()
{
}

void TowerScene::OnDestroy()
{
}

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

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
{
	// 플레이어 생성
	m_player = make_shared<Player>();
	/*m_player->SetTexture(m_textures["PLAYER"]);
	m_player->SetMesh(m_meshs["PLAYER"]);*/
	m_player->LoadGeometry(device, commandlist, TEXT("./Resource/Model/Warrior.bin"));
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
	m_object.insert({ "SKYBOX", skybox });
	m_object.insert({ "FIELD", field });
	m_object.insert({ "FENCE", fence });
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

