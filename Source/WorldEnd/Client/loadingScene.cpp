#include "loadingScene.h"

LoadingScene::LoadingScene(const ComPtr<ID3D12Device>& device, 
	const ComPtr<ID3D12GraphicsCommandList>& commandList, 
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature) : m_loadEnd{ false }
{
	OnCreate(device, commandList, rootSignature, postRootSignature);
}

LoadingScene::~LoadingScene()
{
	//OnDestroy();
}

void LoadingScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) {};
void LoadingScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) {};

void LoadingScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) {};
void LoadingScene::OnProcessingMouseMessage(UINT message, LPARAM lParam) {};
void LoadingScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) {};

void LoadingScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
}

void LoadingScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature,
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	g_loadingIndex = 0;
	m_loadingText = make_shared<Text>();

	auto skyblueBrush = ComPtr<ID2D1SolidColorBrush>();
	Utiles::ThrowIfFailed(g_GameFramework.GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::SkyBlue, 1.f), &skyblueBrush));
	Text::m_colorBrushes.insert({ "SKYBLUE", skyblueBrush });
	m_loadingText->SetColorBrush("SKYBLUE");

	auto koPub24 = ComPtr<IDWriteTextFormat>();
	Utiles::ThrowIfFailed(g_GameFramework.GetWriteFactory()->CreateTextFormat(
		TEXT("./Resource/Font/KoPub Dotum Bold.ttf"), nullptr,
		DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.f,
		TEXT("ko-kr"), &koPub24
	));
	koPub24->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	koPub24->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	Text::m_textFormats.insert({ "KOPUB24", koPub24 });
	m_loadingText->SetTextFormat("KOPUB24");

	Utiles::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator)));
	Utiles::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList)));
	m_loadingThread = thread{ &LoadingScene::BuildObjects, this, device, m_threadCommandList, rootSignature, postRootSignature };
	m_loadingThread.detach();
}

void LoadingScene::OnDestroy()
{
	m_loadEnd = false;

	DestroyObjects();
}

void LoadingScene::ReleaseUploadBuffer()
{
	for (const auto& mesh : m_meshs) mesh.second->ReleaseUploadBuffer();
	for (const auto& texture : m_textures) texture.second->ReleaseUploadBuffer();
	for (const auto& material : m_materials) material.second->ReleaseUploadBuffer();
}

void LoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	BuildShader(device, rootSignature, postRootSignature);
	BuildMesh(device, commandList);
	BuildTexture(device, commandList);
	BuildMeterial(device, commandList);
	BuildAnimationSet();
	BuildText();

	commandList->Close();
	ID3D12CommandList* ppCommandList[]{ commandList.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_GameFramework.WaitForPreviousFrame();

	m_loadEnd = true;
}

void LoadingScene::BuildShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	auto objectShader1{ make_shared<StaticObjectShader>(device, rootSignature) };
	auto objectShader2{ make_shared<StaticObjectShader>(device, rootSignature) };
	auto objectBlendShader{ make_shared<StaticObjectBlendShader>(device, rootSignature) };
	auto animationShader{ make_shared<AnimationShader>(device, rootSignature) };
	auto terrainShader{ make_shared<TerrainShader>(device, rootSignature) };
	auto skyboxShader{ make_shared<SkyboxShader>(device, rootSignature) };
	auto uiShader{ make_shared<UIShader>(device, rootSignature) };
	auto postUiShader{ make_shared<UIShader>(device, rootSignature) };
	auto triggerEffectShader{ make_shared<TriggerEffectShader>(device, rootSignature) };
	auto horzGaugeShader{ make_shared<HorzGaugeShader>(device, rootSignature) };
	auto vertGaugeShader{ make_shared<VertGaugeShader>(device, rootSignature) };
	auto shadowShader{ make_shared<ShadowShader>(device, rootSignature) };
	auto animationShadowShader{ make_shared<AnimationShadowShader>(device, rootSignature) };
	auto emitterParticleShader{ make_shared<EmitterParticleShader>(device, rootSignature) };
	auto pumperParticleShader{ make_shared<PumperParticleShader>(device, rootSignature) };
	auto wireFrameShader{ make_shared<Shader>(device, rootSignature) };
	auto horzBlurShader{ make_shared<HorzBlurShader>(device, postRootSignature) };
	auto vertBlurShader{ make_shared<VertBlurShader>(device, postRootSignature) };
	auto sobelShader{ make_shared<SobelShader>(device, postRootSignature) };
	auto compositeShader{ make_shared<CompositeShader>(device, postRootSignature) };
	auto fadeShader{ make_shared<FadeShader>(device, postRootSignature) };
	//auto debugShader{ make_shared<DebugShader>(device, rootsignature) };

	auto arrowInstance{ make_shared<ArrowInstance>(device, rootSignature, MAX_ARROWRAIN_ARROWS) };

	m_shaders.insert({ "ANIMATION", animationShader });
	m_shaders.insert({ "OBJECT1", objectShader1 });
	m_shaders.insert({ "OBJECT2", objectShader2 });
	m_shaders.insert({ "OBJECTBLEND", objectBlendShader });
	m_shaders.insert({ "TERRAIN", terrainShader });
	m_shaders.insert({ "SKYBOX", skyboxShader });
	m_shaders.insert({ "TRIGGEREFFECT", triggerEffectShader });
	m_shaders.insert({ "HORZGAUGE", horzGaugeShader });
	m_shaders.insert({ "VERTGAUGE", vertGaugeShader });
	m_shaders.insert({ "SHADOW", shadowShader });
	m_shaders.insert({ "ANIMATIONSHADOW", animationShadowShader });
	m_shaders.insert({ "EMITTERPARTICLE", emitterParticleShader });
	m_shaders.insert({ "PUMPERPARTICLE", pumperParticleShader });
	m_shaders.insert({ "WIREFRAME", wireFrameShader });
	m_shaders.insert({ "UI", uiShader });
	m_shaders.insert({ "POSTUI", postUiShader });
	m_shaders.insert({ "HORZBLUR", horzBlurShader });
	m_shaders.insert({ "VERTBLUR", vertBlurShader });
	m_shaders.insert({ "SOBEL", sobelShader });
	m_shaders.insert({ "COMPOSITE", compositeShader });
	m_shaders.insert({ "FADE", fadeShader });
	//m_globalShaders.insert({ "DEBUG", debugShader });

	m_shaders.insert({ "ARROW_INSTANCE", arrowInstance });
}

void LoadingScene::BuildMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	LoadAnimationMeshFromFile(device, commandList, TEXT("./Resource/Mesh/WarriorMesh.bin"));
	LoadAnimationMeshFromFile(device, commandList, TEXT("./Resource/Mesh/ArcherMesh.bin"));

	auto staminaBarMesh{ make_shared<PlaneMesh>(device, commandList, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.1f, 0.89f }) };
	m_meshs.insert({ "STAMINABAR", staminaBarMesh });

	LoadAnimationMeshFromFile(device, commandList, TEXT("./Resource/Mesh/Undead_WarriorMesh.bin"));
	LoadAnimationMeshFromFile(device, commandList, TEXT("./Resource/Mesh/Undead_ArcherMesh.bin"));
	LoadAnimationMeshFromFile(device, commandList, TEXT("./Resource/Mesh/Undead_WizardMesh.bin"));

	LoadAnimationMeshFromFile(device, commandList, TEXT("./Resource/Mesh/CentaurMesh.bin"));

	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/MeshArrowMesh.bin"));

	// 마을 씬 메쉬 로딩
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Barrel_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Basket_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Basket_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_EMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_FMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_GMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_HMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Box_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Bridge_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingDoor_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingPillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_D_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_03Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_04Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_05Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_06Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_03Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_04Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_05Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_C_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Building_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleDoor_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_EMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_FMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_GMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_HMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Castle_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Castle_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Chimney_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Decal_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Door_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_F_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_G_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Frame_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_EMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_FMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_GMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_HMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_IMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_JMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_KMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_LMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_MMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_NMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_OMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_PMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_QMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_RMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_A_03Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_B_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_B_03Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Lamp_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Lamp_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_A_LeafMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_A_TrunkMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_B_LeafMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_B_TrunkMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Wagon_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_EMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_FMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_GMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Statue_Pillar_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Stone_Statue_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_03Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_04Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_EMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_FMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_GMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Store_HMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownChair_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_BMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_CMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_DMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_EMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_FMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_GMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_HMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_IMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_JMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_KMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_LMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_MMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_NMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_OMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_PMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_QMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_RMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_SMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_TMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_UMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_VMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_WMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Tree_branch_AMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Windmill_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Windmill_Door_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Windmill_Wing_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Window_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/Window_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/BlockingMesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/VillageSceneMesh/NonblockingMesh.bin"));

	vector<Vertex> vertices;
	vertices.emplace_back(XMFLOAT3{ -1.f, +1.f, +1.f });
	vertices.emplace_back(XMFLOAT3{ +1.f, +1.f, +1.f });
	vertices.emplace_back(XMFLOAT3{ +1.f, +1.f, -1.f });
	vertices.emplace_back(XMFLOAT3{ -1.f, +1.f, -1.f });

	vertices.emplace_back(XMFLOAT3{ -1.f, -1.f, +1.f });
	vertices.emplace_back(XMFLOAT3{ +1.f, -1.f, +1.f });
	vertices.emplace_back(XMFLOAT3{ +1.f, -1.f, -1.f });
	vertices.emplace_back(XMFLOAT3{ -1.f, -1.f, -1.f });

	vector<UINT> indices;
	indices.push_back(0); indices.push_back(1); indices.push_back(2);
	indices.push_back(0); indices.push_back(2); indices.push_back(3);

	indices.push_back(3); indices.push_back(2); indices.push_back(6);
	indices.push_back(3); indices.push_back(6); indices.push_back(7);

	indices.push_back(7); indices.push_back(6); indices.push_back(5);
	indices.push_back(7); indices.push_back(5); indices.push_back(4);

	indices.push_back(1); indices.push_back(0); indices.push_back(4);
	indices.push_back(1); indices.push_back(4); indices.push_back(5);

	indices.push_back(0); indices.push_back(3); indices.push_back(7);
	indices.push_back(0); indices.push_back(7); indices.push_back(4);

	indices.push_back(2); indices.push_back(1); indices.push_back(5);
	indices.push_back(2); indices.push_back(5); indices.push_back(6);

	shared_ptr<Mesh> wireFrame{ make_shared<Mesh>(device, commandList, vertices ,indices) };
	m_meshs.insert({ "WIREFRAME", wireFrame });


	// 타워 씬 메쉬 로딩
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchDeco_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchPillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_ArchPillar_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Arch_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Brazier_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_DecoTile_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_DoorFrame_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Door_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_GatePillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Ground_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Ground_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Pillar_G_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Stair_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Wall_D_01Mesh.bin"));

	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGate_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGate_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandList, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGateGround_B_01Mesh.bin"));

	auto hpBarMesh{ make_shared<PlaneMesh>(device, commandList, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.75f, 0.15f }) };
	auto skyboxMesh{ make_shared <SkyboxMesh>(device, commandList, 20.0f, 20.0f, 20.0f) };
	auto magicCircleExtent = TriggerSetting::EXTENT[static_cast<INT>(TriggerType::ARROW_RAIN)];
	auto magicCircleMesh{ make_shared<PlaneMesh>(device, commandList, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ magicCircleExtent.x * 2, magicCircleExtent.z * 2 }) };
	auto monsterMagicCircleExtent = TriggerSetting::EXTENT[static_cast<INT>(TriggerType::UNDEAD_GRASP)];
	auto monsterMagicCircleMesh{ make_shared<PlaneMesh>(device, commandList, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ monsterMagicCircleExtent.x * 2, monsterMagicCircleExtent.z * 2 }) };
	auto debugMesh{ make_shared<PlaneMesh>(device, commandList, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.3f, 0.5f }) };

	m_meshs.insert({ "SKYBOX", skyboxMesh });
	m_meshs.insert({ "HPBAR", hpBarMesh });
	m_meshs.insert({ "MAGICCIRCLE", magicCircleMesh });
	m_meshs.insert({ "MONSTERMAGICCIRCLE", monsterMagicCircleMesh });
	m_meshs.insert({ "DEBUG", debugMesh });
}

void LoadingScene::BuildTexture(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	auto frameUITexture{ make_shared<Texture>() };
	frameUITexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/UI_Background.dds"), (INT)ShaderRegister::BaseTexture);
	frameUITexture->CreateSrvDescriptorHeap(device);
	frameUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto cancelUITexture{ make_shared<Texture>() };
	cancelUITexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/UI_Cancel.dds"), (INT)ShaderRegister::BaseTexture);
	cancelUITexture->CreateSrvDescriptorHeap(device);
	cancelUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto buttonUITexture{ make_shared<Texture>() };
	buttonUITexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/UI_Button.dds"), (INT)ShaderRegister::BaseTexture);
	buttonUITexture->CreateSrvDescriptorHeap(device);
	buttonUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto goldUITexture{ make_shared<Texture>() };
	goldUITexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Gold.dds"), (INT)ShaderRegister::BaseTexture);
	goldUITexture->CreateSrvDescriptorHeap(device);
	goldUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	auto warriorSkillTexture{ make_shared<Texture>() };
	warriorSkillTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Warrior_Skill.dds"), (INT)ShaderRegister::BaseTexture);
	warriorSkillTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Warrior_Skill_Cool.dds"), (INT)ShaderRegister::SubTexture);
	warriorSkillTexture->CreateSrvDescriptorHeap(device);
	warriorSkillTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto warriorUltimateTexture{ make_shared<Texture>() };
	warriorUltimateTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Warrior_Ultimate.dds"), (INT)ShaderRegister::BaseTexture);
	warriorUltimateTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Warrior_Ultimate_Cool.dds"), (INT)ShaderRegister::SubTexture);
	warriorUltimateTexture->CreateSrvDescriptorHeap(device);
	warriorUltimateTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto archerSkillTexture{ make_shared<Texture>() };
	archerSkillTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Archer_Skill.dds"), (INT)ShaderRegister::BaseTexture);
	archerSkillTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Archer_Skill_Cool.dds"), (INT)ShaderRegister::SubTexture);
	archerSkillTexture->CreateSrvDescriptorHeap(device);
	archerSkillTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto archerUltimateTexture{ make_shared<Texture>() };
	archerUltimateTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Archer_Ultimate.dds"), (INT)ShaderRegister::BaseTexture);
	archerUltimateTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/SkillTexture/Archer_Ultimate_Cool.dds"), (INT)ShaderRegister::SubTexture);
	archerUltimateTexture->CreateSrvDescriptorHeap(device);
	archerUltimateTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	auto staminaBarTexture{ make_shared<Texture>() };
	staminaBarTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Full_StaminaBar.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	staminaBarTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Empty_StaminaBar.dds"), (INT)ShaderRegister::SubTexture); // SubTexture
	staminaBarTexture->CreateSrvDescriptorHeap(device);
	staminaBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	m_textures.insert({ "FRAMEUI", frameUITexture });
	m_textures.insert({ "CANCELUI", cancelUITexture });
	m_textures.insert({ "BUTTONUI", buttonUITexture });
	m_textures.insert({ "GOLDUI", goldUITexture });
	m_textures.insert({ "WARRIORSKILL", warriorSkillTexture });
	m_textures.insert({ "WARRIORULTIMATE", warriorUltimateTexture });
	m_textures.insert({ "ARCHERSKILL", archerSkillTexture });
	m_textures.insert({ "ARCHERULTIMATE", archerUltimateTexture });
	m_textures.insert({ "STAMINABAR", staminaBarTexture });

	auto titleUITexture{ make_shared<Texture>() };
	titleUITexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/VillageSceneTexture/Title.dds"), (INT)ShaderRegister::BaseTexture);
	titleUITexture->CreateSrvDescriptorHeap(device);
	titleUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	m_textures.insert({ "TITLE", titleUITexture });

	auto villageSkyboxTexture{ make_shared<Texture>() };
	villageSkyboxTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Skybox/VillageSkyBox.dds"), (INT)ShaderRegister::SkyboxTexture);	// Skybox
	villageSkyboxTexture->CreateSrvDescriptorHeap(device);
	villageSkyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	m_textures.insert({ "VILLAGESKYBOX", villageSkyboxTexture });

	auto terrainTexture{ make_shared<Texture>() };
	terrainTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Terrain/Base_Texture.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	terrainTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Terrain/Detail_Texture.dds"), (INT)ShaderRegister::SubTexture); // DetailTexture
	terrainTexture->CreateSrvDescriptorHeap(device);
	terrainTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	m_textures.insert({ "TERRAIN", terrainTexture });

	auto hpBarTexture{ make_shared<Texture>() };
	hpBarTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Full_HpBar.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	hpBarTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Empty_HpBar.dds"), (INT)ShaderRegister::SubTexture); // SubTexture
	hpBarTexture->CreateSrvDescriptorHeap(device);
	hpBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	auto towerSkyboxTexture{ make_shared<Texture>() };
	towerSkyboxTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/Skybox/TowerSkyBox.dds"), (INT)ShaderRegister::SkyboxTexture);	// Skybox
	towerSkyboxTexture->CreateSrvDescriptorHeap(device);
	towerSkyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	auto magicCircleTexture{ make_shared<Texture>() };
	magicCircleTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/MagicCircle.dds"), (INT)ShaderRegister::BaseTexture);	// MagicCircle
	magicCircleTexture->CreateSrvDescriptorHeap(device);
	magicCircleTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto monsterMagicCircleTexture{ make_shared<Texture>() };
	monsterMagicCircleTexture->LoadTextureFile(device, commandList, TEXT("Resource/Texture/MonsterMagicCircle.dds"), (INT)ShaderRegister::BaseTexture);	// MagicCircle
	monsterMagicCircleTexture->CreateSrvDescriptorHeap(device);
	monsterMagicCircleTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	m_textures.insert({ "TOWERSKYBOX", towerSkyboxTexture });
	m_textures.insert({ "HPBAR", hpBarTexture });
	m_textures.insert({ "MAGICCIRCLE", magicCircleTexture });
	m_textures.insert({ "MONSTERMAGICCIRCLE", monsterMagicCircleTexture });
}

void LoadingScene::BuildMeterial(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 캐릭터 메테리얼 로딩
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/ArcherTexture.bin"));

	// 마을 씬 메테리얼 로딩
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Barrel_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Basket_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Basket_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_ETexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_FTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_GTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_HTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Box_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Bridge_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingDoor_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingPillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_D_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_03Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_04Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_05Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_06Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_03Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_04Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_05Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_C_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Building_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleDoor_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_ETexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_FTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_GTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_HTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Castle_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Castle_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Chimney_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Decal_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Door_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flag_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flag_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flag_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flag_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_F_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Flower_G_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Frame_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_ETexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_FTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_GTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_HTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_ITexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_JTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_KTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_LTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_MTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_NTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_OTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_PTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_QTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_RTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_A_03Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_B_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_B_03Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Lamp_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Lamp_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_01_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_01_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_02_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_02_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_03_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_03_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_04_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_04_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_01_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_01_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_02_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_02_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_03_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_03_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_04_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_04_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/MV_Wagon_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_ETexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_FTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_GTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Statue_Pillar_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Stone_Statue_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_03Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_04Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_ETexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_FTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_GTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Store_HTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownChair_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_BTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_CTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_DTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_ETexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_FTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_GTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_HTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_ITexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_JTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_KTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_LTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_MTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_NTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_OTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_PTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_QTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_RTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_STexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_TTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_UTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_VTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_WTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Tree_branch_ATexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Windmill_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Windmill_Door_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Windmill_Wing_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Window_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/Window_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/BlockingTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/VillageSceneTexture/NonblockingTexture.bin"));


	// 타워 씬 메테리얼 로딩
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/ArcherTexture.bin"));

	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/Undead_WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/Undead_ArcherTexture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/Undead_WizardTexture.bin"));

	LoadMaterialFromFile(device, commandList, TEXT("./Resource/Texture/CentaurTexture.bin"));

	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/Archer_WeaponArrowTexture.bin"));

	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_ArchDeco_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_ArchPillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_ArchPillar_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Arch_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Brazier_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_DecoTile_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_DoorFrame_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Door_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Frame_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Frame_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Frame_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Frame_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_GatePillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Ground_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Ground_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Pillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Pillar_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Pillar_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Pillar_G_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Stair_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Wall_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Wall_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_Wall_D_01Texture.bin"));

	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_RuneGate_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_RuneGate_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandList, TEXT("Resource/Texture/TowerSceneTexture/AD_RuneGateGround_B_01Texture.bin"));
}

void LoadingScene::BuildAnimationSet()
{
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/WarriorAnimation.bin"), "WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/ArcherAnimation.bin"), "ArcherAnimation");

	LoadAnimationSetFromFile(TEXT("Resource/Animation/Undead_WarriorAnimation.bin"), "Undead_WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("Resource/Animation/Undead_ArcherAnimation.bin"), "Undead_ArcherAnimation");
	LoadAnimationSetFromFile(TEXT("Resource/Animation/Undead_WizardAnimation.bin"), "Undead_WizardAnimation");

	LoadAnimationSetFromFile(TEXT("./Resource/Animation/CentaurAnimation.bin"), "CentaurAnimation");
}

void LoadingScene::BuildText()
{
	auto whiteBrush = ComPtr<ID2D1SolidColorBrush>();
	Utiles::ThrowIfFailed(g_GameFramework.GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.f), &whiteBrush));
	Text::m_colorBrushes.insert({ "WHITE", whiteBrush });

	const array<INT, 4> fontSize{ 15, 18, 21, 27 };
	for (const auto size : fontSize) {
		auto koPub = ComPtr<IDWriteTextFormat>();
		Utiles::ThrowIfFailed(g_GameFramework.GetWriteFactory()->CreateTextFormat(
			TEXT("./Resource/Font/KoPub Dotum Bold.ttf"), nullptr,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, (FLOAT)size,
			TEXT("ko-kr"), &koPub
		));
		koPub->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		koPub->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		string name{ "KOPUB" };
		name += to_string(size);
		Text::m_textFormats.insert({ name, koPub });
	}
	const array<INT, 5> mapleFontSize{ 15, 18, 21, 24, 27 };
	for (const auto size : mapleFontSize) {
		auto maple = ComPtr<IDWriteTextFormat>();
		Utiles::ThrowIfFailed(g_GameFramework.GetWriteFactory()->CreateTextFormat(
			TEXT("./Resource/Font/Maplestory Bold.ttf"), nullptr,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, (FLOAT)size,
			TEXT("ko-kr"), &maple
		));
		maple->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		maple->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		string name{ "MAPLE" };
		name += to_string(size);
		Text::m_textFormats.insert({ name, maple });
	}
}

void LoadingScene::DestroyObjects()
{
	m_loadingText.reset();
}

void LoadingScene::Update(FLOAT timeElapsed)
{
	m_loadingText->SetText(g_loadingText + L" 로딩 중.. " + to_wstring(g_loadingIndex) + m_maxFileCount);
	if (m_loadEnd) {
		ReleaseUploadBuffer();
		g_GameFramework.ChangeScene(SCENETAG::LoginScene);
	}
}

void LoadingScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) {}
void LoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	m_loadingText->Render(deviceContext);
}
void LoadingScene::PostRenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) {}

void LoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	g_loadingText = fileName;
	++g_loadingIndex;
	g_mutex.unlock();

	BYTE strLength;
	string backup;
	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {}
		else if (strToken == "<Mesh>:") {
			auto mesh = make_shared<MeshFromFile>();
			mesh->LoadMesh(device, commandList, in);

			m_meshs.insert({ mesh->GetMeshName(), mesh });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void LoadingScene::LoadAnimationMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	g_loadingText = fileName;
	++g_loadingIndex;
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
}

void LoadingScene::LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	g_loadingText = fileName;
	++g_loadingIndex;
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

			materials->LoadMaterials(device, commandList, in, false);

			m_materials.insert({ materials->m_materialName, materials });
			m_materials.insert({ "@" + materials->m_materialName, materials });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void LoadingScene::LoadAnimationSetFromFile(wstring fileName, const string& animationSetName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	g_mutex.lock();
	g_loadingText = fileName;
	++g_loadingIndex;
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
}