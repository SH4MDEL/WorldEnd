﻿#include "loadingScene.h"

LoadingScene::LoadingScene() : m_loadEnd{false}
{

}

LoadingScene::~LoadingScene()
{

}

void LoadingScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	m_loadingText = make_shared<LoadingText>(60);

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

	DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator)));
	DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList)));

	m_loadingThread = thread{ &LoadingScene::BuildObjects, this, device, m_threadCommandList, rootSignature, postRootSignature };
	m_loadingThread.detach();
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
void LoadingScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) {}
void LoadingScene::OnProcessingMouseMessage(UINT message, LPARAM lParam) {}
void LoadingScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) {}

void LoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	// 플레이어 로딩
	auto animationShader{ make_shared<AnimationShader>(device, rootsignature) };
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/WarriorMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/ArcherMesh.bin"));

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/ArcherTexture.bin"));

	LoadAnimationSetFromFile(TEXT("./Resource/Animation/WarriorAnimation.bin"), "WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/ArcherAnimation.bin"), "ArcherAnimation");

	auto objectShader{ make_shared<StaticObjectShader>(device, rootsignature) };

	// 체력 바 로딩
	auto hpBarMesh{ make_shared<BillboardMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.75f, 0.15f }) };
	auto hpBarTexture{ make_shared<Texture>() };
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Full_HpBar.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Empty_HpBar.dds"), (INT)ShaderRegister::SubTexture); // SubTexture
	hpBarTexture->CreateSrvDescriptorHeap(device);
	hpBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 스테미너 바 로딩
	auto staminaBarMesh{ make_shared<BillboardMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.1f, 0.89f }) };
	auto staminaBarTexture{ make_shared<Texture>() };
	staminaBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Full_StaminaBar.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	staminaBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Empty_StaminaBar.dds"), (INT)ShaderRegister::SubTexture); // SubTexture
	staminaBarTexture->CreateSrvDescriptorHeap(device);
	staminaBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 스카이박스 로딩
	auto skyboxShader{ make_shared<SkyboxShader>(device, rootsignature) };
	auto skyboxMesh{ make_shared <SkyboxMesh>(device, commandlist, 20.0f, 20.0f, 20.0f)};
	auto skyboxTexture{ make_shared<Texture>() };
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), (INT)ShaderRegister::SkyboxTexture);	// Skybox
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	// UI 로딩
	auto uiShader{ make_shared<UIShader>(device, rootsignature) };
	auto postUiShader{ make_shared<UIShader>(device, rootsignature) };
	auto exitUITexture{ make_shared<Texture>() };
	exitUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/UI_Background.dds"), (INT)ShaderRegister::BaseTexture); // SubTexture
	exitUITexture->CreateSrvDescriptorHeap(device);
	exitUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto cancelUITexture{ make_shared<Texture>() };
	cancelUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/UI_Cancel.dds"), (INT)ShaderRegister::BaseTexture); // SubTexture
	cancelUITexture->CreateSrvDescriptorHeap(device);
	cancelUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto buttonUITexture{ make_shared<Texture>() };
	buttonUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/UI_Button.dds"), (INT)ShaderRegister::BaseTexture); // SubTexture
	buttonUITexture->CreateSrvDescriptorHeap(device);
	buttonUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	// 게이지 셰이더 로딩
	auto horzGaugeShader{ make_shared<HorzGaugeShader>(device, rootsignature) };
	auto vertGaugeShader{ make_shared<VertGaugeShader>(device, rootsignature) };

	// 그림자 셰이더 로딩
	auto shadowShader{ make_shared<ShadowShader>(device, rootsignature) };
	auto animationShadowShader{ make_shared<AnimationShadowShader>(device, rootsignature) };

	// 파티클 셰이더 로딩
	auto emitterParticleShader{ make_shared<EmitterParticleShader>(device, rootsignature) };
	auto pumperParticleShader{ make_shared<PumperParticleShader>(device, rootsignature) };


	// 블러 필터 셰이더 로딩
	auto horzBlurShader{ make_shared<HorzBlurShader>(device, postRootSignature) };
	auto vertBlurShader{ make_shared<VertBlurShader>(device, postRootSignature) };

	// 소벨 필터 셰이더 로딩
	auto sobelShader{ make_shared<SobelShader>(device, postRootSignature) };
	auto compositeShader{ make_shared<CompositeShader>(device, postRootSignature) };

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
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_Frame_B_01Mesh.bin"));
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
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_Frame_B_01Texture.bin"));
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

	// 룬 게이트 메쉬 로딩
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGate_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGate_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGateGround_B_01Mesh.bin"));

	// 룬 게이트 텍스처 로딩
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_RuneGate_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_RuneGate_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_RuneGateGround_B_01Texture.bin"));

	// 메쉬 설정
	m_meshs.insert({ "SKYBOX", skyboxMesh });
	m_meshs.insert({ "HPBAR", hpBarMesh });
	m_meshs.insert({ "STAMINABAR", staminaBarMesh });

	// 텍스처 설정
	m_textures.insert({ "SKYBOX", skyboxTexture });
	m_textures.insert({ "HPBAR", hpBarTexture });
	m_textures.insert({ "STAMINABAR", staminaBarTexture });
	// UI 관련 텍스처
	m_textures.insert({ "EXITUI", exitUITexture });
	m_textures.insert({ "CANCELUI", cancelUITexture });
	m_textures.insert({ "BUTTONUI", buttonUITexture });

	// 셰이더 설정
	m_shaders.insert({ "ANIMATION", animationShader });
	m_shaders.insert({ "OBJECT", objectShader });
	m_shaders.insert({ "SKYBOX", skyboxShader });
	m_shaders.insert({ "HORZGAUGE", horzGaugeShader });
	m_shaders.insert({ "VERTGAUGE", vertGaugeShader });
	m_shaders.insert({ "SHADOW", shadowShader });
	m_shaders.insert({ "ANIMATIONSHADOW", animationShadowShader });
	m_shaders.insert({ "EMITTERPARTICLE", emitterParticleShader });
	m_shaders.insert({ "PUMPERPARTICLE", pumperParticleShader });
	m_shaders.insert({ "POSTUI", postUiShader });
	m_shaders.insert({ "HORZBLUR", horzBlurShader });
	m_shaders.insert({ "VERTBLUR", vertBlurShader });
	m_shaders.insert({ "SOBEL", sobelShader });
	m_shaders.insert({ "COMPOSITE", compositeShader });

	commandlist->Close();
	ID3D12CommandList* ppCommandList[]{ commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_GameFramework.WaitForPreviousFrame();

	m_loadEnd = true;
}

void LoadingScene::Update(FLOAT timeElapsed) 
{
	m_loadingText->Update(timeElapsed);
	if (m_loadEnd) g_GameFramework.ChangeScene(SCENETAG::TowerScene);
}

void LoadingScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const {}
void LoadingScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) {}
void LoadingScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) {}

void LoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	m_loadingText->Render(deviceContext);
}

void LoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	m_loadingText->SetFileName(fileName);

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

	m_loadingText->LoadingFile();
}

void LoadingScene::LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	m_loadingText->SetFileName(fileName);

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
			
			m_materials.insert({ materials->m_materialName, materials });
			m_materials.insert({ "@" + materials->m_materialName, materials});
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}

	m_loadingText->LoadingFile();
}

void LoadingScene::LoadAnimationSetFromFile(wstring fileName, const string& animationSetName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	m_loadingText->SetFileName(fileName);

	BYTE strLength{};
	in.read((char*)(&strLength), sizeof(BYTE));

	string strDummy(strLength, '\0');
	in.read(&strDummy[0], sizeof(char) * strLength);

	INT dummy{};
	in.read((char*)(&dummy), sizeof(INT));
	
	auto animationSet = make_shared<AnimationSet>();
	animationSet->LoadAnimationSet(in, animationSetName);

	m_animationSets.insert({ animationSetName, animationSet });

	m_loadingText->LoadingFile();
}