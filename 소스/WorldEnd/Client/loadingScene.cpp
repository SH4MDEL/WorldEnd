#include "loadingScene.h"


LoadingScene::~LoadingScene()
{
}

void LoadingScene::OnCreate()
{

}

void LoadingScene::OnDestroy()
{

}

void LoadingScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const {}
void LoadingScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const {}

void LoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
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
	auto playerShader{ make_shared<Shader>(device, rootsignature) };
	auto playerTexture{ make_shared<Texture>() };
	auto playerMesh{ make_shared<Mesh>(device, commandlist, vertices, indices) };
	playerTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Bricks.dds"), 2);
	playerTexture->CreateSrvDescriptorHeap(device);
	playerTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);


	// 지형 생성
	auto fieldShader{ make_shared<DetailShader>(device, rootsignature) };
	auto fieldTexture{ make_shared<Texture>() };
	fieldTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Base_Texture.dds"), 2); // BaseTexture
	fieldTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Detail_Texture.dds"), 3); // DetailTexture
	fieldTexture->CreateSrvDescriptorHeap(device);
	fieldTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 펜스 생성
	auto blendingShader{ make_shared<BlendingShader>(device, rootsignature) };
	auto fenceTexture{ make_shared<Texture>() };
	fenceTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Fence.dds"), 2); // BaseTexture
	fenceTexture->CreateSrvDescriptorHeap(device);
	fenceTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 스카이박스 생성
	auto skyboxShader{ make_shared<SkyboxShader>(device, rootsignature) };
	auto skyboxTexture{ make_shared<Texture>() };
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), 4);
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	// 메쉬 설정
	m_meshs.insert({ "PLAYER", playerMesh });

	// 텍스처 설정
	m_textures.insert({ "PLAYER", playerTexture });
	m_textures.insert({ "SKYBOX", skyboxTexture });
	m_textures.insert({ "FIELD", fieldTexture });
	m_textures.insert({ "FENCE", fenceTexture });

	// 셰이더 설정
	m_shaders.insert({ "PLAYER", playerShader });
	m_shaders.insert({ "SKYBOX", skyboxShader });
	m_shaders.insert({ "FIELD", fieldShader });
	m_shaders.insert({ "FENCE", blendingShader });

	g_GameFramework.ChangeScene(SCENETAG::TowerScene);
}

void LoadingScene::Update(FLOAT timeElapsed) {}

void LoadingScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const {}

