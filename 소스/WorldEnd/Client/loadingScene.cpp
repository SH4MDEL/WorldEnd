#include "loadingScene.h"

LoadingScene::LoadingScene(const ComPtr<ID3D12Device>& device)
{
	for (int i = 0; i < MAX_THREAD; ++i) {
		m_canNextScene[i] = false;
	}
	//DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator)));
	//DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList)));
}

LoadingScene::~LoadingScene()
{

}

void LoadingScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& mainCommandList,
	const array<ComPtr<ID3D12GraphicsCommandList>, MAX_THREAD>& threadCommandList,
	array<thread, MAX_THREAD>& subThread,
	const ComPtr<ID3D12RootSignature>& rootSignature)
{
	m_loadingText = make_shared<LoadingText>(53);

	auto textBrush = ComPtr<ID2D1SolidColorBrush>();
	DX::ThrowIfFailed(g_GameFramework.GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::SkyBlue, 1.f), &textBrush));
	m_loadingText->SetTextBrush(textBrush);

	auto textFormat = ComPtr<IDWriteTextFormat>();
	DX::ThrowIfFailed(g_GameFramework.GetWriteFactory()->CreateTextFormat(
		TEXT("돋움체"), nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		20.f,
		TEXT("ko-kr"),
		&textFormat
	));
	textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	m_loadingText->SetTextFormat(textFormat);

	subThread[0] = thread{&LoadingScene::BulidObjectsByThread1, this, device, threadCommandList[0], rootSignature};
	subThread[1] = thread{&LoadingScene::BulidObjectsByThread2, this, device, threadCommandList[1], rootSignature };
	subThread[2] = thread{&LoadingScene::BulidObjectsByThread3, this, device, threadCommandList[2], rootSignature };
	subThread[3] = thread{&LoadingScene::BulidObjectsByThread4, this, device, threadCommandList[3], rootSignature };

	for (int i = 0; i < MAX_THREAD; ++i) {
		subThread[i].detach();
	}
}

void LoadingScene::OnDestroy()
{
	m_loadingText.reset();
}

void LoadingScene::ReleaseUploadBuffer()
{
	for (const auto& mesh : m_meshs) mesh.second->ReleaseUploadBuffer();
	for (const auto& texture : m_textures) texture.second->ReleaseUploadBuffer();
}

void LoadingScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) {}
void LoadingScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) {}
void LoadingScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const {}
void LoadingScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const {}

void LoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	// 플레이어 로딩
	auto animationShader{ make_shared<AnimationShader>(device, rootsignature) };
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/WarriorMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/ArcherMesh.bin"));

	LoadAnimationSetFromFile(TEXT("./Resource/Animation/WarriorAnimation.bin"), "WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/ArcherAnimation.bin"), "ArcherAnimation");

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/ArcherTexture.bin"));

	auto objectShader{ make_shared<TextureHierarchyShader>(device, rootsignature) };

	// 체력 바 로딩
	auto hpBarShader{ make_shared<HpBarShader>(device, rootsignature) };
	auto hpBarMesh{ make_shared<BillboardMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.75f, 0.15f }) };
	auto hpBarTexture{ make_shared<Texture>() };
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Full_HpBar.dds"), 5); // BaseTexture
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Empty_HpBar.dds"), 6); // SubTexture
	hpBarTexture->CreateSrvDescriptorHeap(device);
	hpBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 스카이박스 로딩
	auto skyboxShader{ make_shared<SkyboxShader>(device, rootsignature) };
	auto skyboxMesh{ make_shared <SkyboxMesh>(device, commandlist, 20.0f, 20.0f, 20.0f)};
	auto skyboxTexture{ make_shared<Texture>() };
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), 7);	// Skybox
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	// 그림자 셰이더 로딩
	auto shadowShader{ make_shared<ShadowShader>(device, rootsignature) };
	auto animationShadowShader{ make_shared<AnimationShadowShader>(device, rootsignature) };

	// 몬스터 로딩
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/Undead_WarriorMesh.bin"));
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/Undead_WarriorAnimation.bin"), "Undead_WarriorAnimation");
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/Undead_WarriorTexture.bin"));

	// 타워 씬 메쉬 로딩
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchDeco_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchPillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchPillar_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Arch_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Brazier_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_DecoTile_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_DoorFrame_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Door_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_GatePillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Ground_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Ground_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_G_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Stair_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_D_01Mesh.bin"));

	//타워 씬 텍스처 로딩
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_ArchDeco_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_ArchPillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_ArchPillar_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Arch_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Brazier_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_DecoTile_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_DoorFrame_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Door_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_GatePillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Ground_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Ground_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_G_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Stair_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Wall_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Wall_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Wall_D_01Texture.bin"));

	// 디버그용 메쉬와 셰이더
	auto debugMesh{ make_shared<UIMesh>(device, commandlist) };
	auto debugShader{ make_shared<UIRenderShader>(device, rootsignature) };

	// 메쉬 설정
	m_meshs.insert({ "HPBAR", hpBarMesh });
	m_meshs.insert({ "SKYBOX", skyboxMesh });
	m_meshs.insert({ "DEBUG", debugMesh });

	// 텍스처 설정
	m_textures.insert({ "SKYBOX", skyboxTexture });
	m_textures.insert({ "HPBAR", hpBarTexture });

	// 셰이더 설정
	m_shaders.insert({ "ANIMATION", animationShader });
	m_shaders.insert({ "OBJECT", objectShader });
	m_shaders.insert({ "SKYBOX", skyboxShader });
	m_shaders.insert({ "HPBAR", hpBarShader });
	m_shaders.insert({ "SHADOW", shadowShader });
	m_shaders.insert({ "ANIMATIONSHADOW", animationShadowShader });
	m_shaders.insert({ "DEBUG", debugShader });
}

void LoadingScene::BulidObjectsByThread1(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	auto animationShader{ make_shared<AnimationShader>(device, rootsignature) };
	auto objectShader{ make_shared<TextureHierarchyShader>(device, rootsignature) };
	auto hpBarShader{ make_shared<HpBarShader>(device, rootsignature) };
	auto skyboxShader{ make_shared<SkyboxShader>(device, rootsignature) };
	auto shadowShader{ make_shared<ShadowShader>(device, rootsignature) };
	auto animationShadowShader{ make_shared<AnimationShadowShader>(device, rootsignature) };
	auto debugShader{ make_shared<UIRenderShader>(device, rootsignature) };

	m_shaders.insert({ "ANIMATION", animationShader });
	m_shaders.insert({ "OBJECT", objectShader });
	m_shaders.insert({ "SKYBOX", skyboxShader });
	m_shaders.insert({ "HPBAR", hpBarShader });
	m_shaders.insert({ "SHADOW", shadowShader });
	m_shaders.insert({ "ANIMATIONSHADOW", animationShadowShader });
	m_shaders.insert({ "DEBUG", debugShader });

	LoadAnimationSetFromFile(TEXT("./Resource/Animation/WarriorAnimation.bin"), "WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/ArcherAnimation.bin"), "ArcherAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/Undead_WarriorAnimation.bin"), "Undead_WarriorAnimation");

	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/WarriorMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/ArcherMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/Undead_WarriorMesh.bin"));
	// 타워 씬 메쉬 로딩
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchDeco_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchPillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchPillar_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Arch_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Brazier_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_DecoTile_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_DoorFrame_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Door_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_GatePillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Ground_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Ground_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_G_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Stair_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_D_01Mesh.bin"));

	auto hpBarMesh{ make_shared<BillboardMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.75f, 0.15f }) };
	auto skyboxMesh{ make_shared <SkyboxMesh>(device, commandlist, 20.0f, 20.0f, 20.0f) };
	auto debugMesh{ make_shared<UIMesh>(device, commandlist) };

	m_meshs.insert({ "HPBAR", hpBarMesh });
	m_meshs.insert({ "SKYBOX", skyboxMesh });
	m_meshs.insert({ "DEBUG", debugMesh });

	DX::ThrowIfFailed(commandlist->Close());

	// 명령 실행을 위해 커맨드 리스트를 커맨드 큐에 추가한다.
	ID3D12CommandList* ppCommandList[] = { commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	g_GameFramework.WaitForPreviousFrame();
	m_canNextScene[0] = true;
}

void LoadingScene::BulidObjectsByThread2(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/WarriorTexture.bin"));

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_ArchDeco_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_ArchPillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_ArchPillar_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Arch_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Brazier_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_DecoTile_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_DoorFrame_B_01Texture.bin"));

	DX::ThrowIfFailed(commandlist->Close());

	// 명령 실행을 위해 커맨드 리스트를 커맨드 큐에 추가한다.
	ID3D12CommandList* ppCommandList[] = { commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	g_GameFramework.WaitForPreviousFrame();
	m_canNextScene[1] = true;
}

void LoadingScene::BulidObjectsByThread3(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/ArcherTexture.bin"));


	//타워 씬 텍스처 로딩
	
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Door_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_GatePillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Ground_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Ground_D_01Texture.bin"));

	auto skyboxTexture{ make_shared<Texture>() };
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), 7);	// Skybox
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	auto hpBarTexture{ make_shared<Texture>() };
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Full_HpBar.dds"), 5); // BaseTexture
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Empty_HpBar.dds"), 6); // SubTexture
	hpBarTexture->CreateSrvDescriptorHeap(device);
	hpBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 텍스처 설정
	m_textures.insert({ "SKYBOX", skyboxTexture });
	m_textures.insert({ "HPBAR", hpBarTexture });

	DX::ThrowIfFailed(commandlist->Close());

	// 명령 실행을 위해 커맨드 리스트를 커맨드 큐에 추가한다.
	ID3D12CommandList* ppCommandList[] = { commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	g_GameFramework.WaitForPreviousFrame();
	m_canNextScene[2] = true;

}

void LoadingScene::BulidObjectsByThread4(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature)
{
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/Undead_WarriorTexture.bin"));

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Pillar_G_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Stair_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Wall_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Wall_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Wall_D_01Texture.bin"));

	DX::ThrowIfFailed(commandlist->Close());

	// 명령 실행을 위해 커맨드 리스트를 커맨드 큐에 추가한다.
	ID3D12CommandList* ppCommandList[] = { commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	g_GameFramework.WaitForPreviousFrame();
	m_canNextScene[3] = true;
}


void LoadingScene::Update(FLOAT timeElapsed) 
{
	m_loadingText->Update(timeElapsed);

	bool canNextScene = true;
	for (int i = 0; i < MAX_THREAD; ++i) {
		if (!m_canNextScene[i]) canNextScene = false;
	}
	if (canNextScene) 
		g_GameFramework.ChangeScene(SCENETAG::TowerScene);
}

void LoadingScene::Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) const {}
void LoadingScene::RenderShadow(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) {}

void LoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	m_loadingText->Render(deviceContext);
}

void LoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	m_loadingText->SetFileName(fileName);
	g_mutex.unlock();

	BYTE strLength;
	string backup;
	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {}
		else if (strToken == "<SkinningInfo>:") {
			auto animationMesh = make_shared<AnimationMesh>();
			animationMesh->LoadAnimationMesh(device, commandList, in);
			animationMesh->SetType(AnimationSetting::ANIMATION_MESH);

			m_meshs.insert({ animationMesh->GetAnimationMeshName(), animationMesh });
		}
		else if (strToken == "<Mesh>:") {
			auto mesh = make_shared<AnimationMesh>();
			mesh->LoadAnimationMesh(device, commandList, in);
			mesh->SetType(AnimationSetting::STANDARD_MESH);

			m_meshs.insert({ mesh->GetAnimationMeshName(), mesh });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}

	g_mutex.lock();
	m_loadingText->LoadingFile();
	g_mutex.unlock();
}

void LoadingScene::LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	m_loadingText->SetFileName(fileName);
	g_mutex.unlock();

	BYTE strLength;
	INT frame, texture;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {}
		else if (strToken == "<Frame>:") {
			auto materials = make_shared<Materials>();

			in.read((char*)(&frame), sizeof(INT));
			in.read((char*)(&texture), sizeof(INT));

			in.read((char*)(&strLength), sizeof(BYTE));
			materials->m_materialName.resize(strLength);
			in.read((&materials->m_materialName[0]), sizeof(char) * strLength);

			materials->LoadMaterials(device, commandList, in);
			
			unique_lock<mutex> materialLock { m_materialMutex };
			m_materials.insert({ materials->m_materialName, materials });
			m_materials.insert({ "@" + materials->m_materialName, materials});
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}

	g_mutex.lock();
	m_loadingText->LoadingFile();
	g_mutex.unlock();
}

void LoadingScene::LoadAnimationSetFromFile(wstring fileName, const string& animationSetName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	m_loadingText->SetFileName(fileName);
	g_mutex.unlock();

	BYTE strLength{};
	in.read((char*)(&strLength), sizeof(BYTE));

	string strDummy(strLength, '\0');
	in.read(&strDummy[0], sizeof(char) * strLength);

	INT dummy{};
	in.read((char*)(&dummy), sizeof(INT));
	
	auto animationSet = make_shared<AnimationSet>();
	animationSet->LoadAnimationSet(in, animationSetName);

	m_animationSets.insert({ animationSetName, animationSet });
	g_mutex.lock();
	m_loadingText->LoadingFile();
	g_mutex.unlock();
}
