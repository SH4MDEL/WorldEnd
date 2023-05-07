#include "towerLoadingScene.h"

TowerLoadingScene::TowerLoadingScene() : m_loadEnd{false}
{

}

TowerLoadingScene::~TowerLoadingScene()
{

}

void TowerLoadingScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	m_loadingText = make_shared<LoadingText>(51);

	m_loadingText->SetColorBrush("SKYBLUE");
	m_loadingText->SetTextFormat("KOPUB24");

	DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator)));
	DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList)));

	m_loadingThread = thread{ &TowerLoadingScene::BuildObjects, this, device, m_threadCommandList, rootSignature, postRootSignature };
	m_loadingThread.detach();
}

void TowerLoadingScene::OnDestroy()
{
	m_loadEnd = false;

	DestroyObjects();
}

void TowerLoadingScene::ReleaseUploadBuffer()
{
	for (const auto& mesh : m_meshs) mesh.second->ReleaseUploadBuffer();
	for (const auto& texture : m_textures) texture.second->ReleaseUploadBuffer();
	for (const auto& material : m_materials) material.second->ReleaseUploadBuffer();
}

void TowerLoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	// 메쉬 로딩
	LoadAnimationMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/Undead_WarriorMesh.bin"));
	LoadAnimationMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/Undead_ArcherMesh.bin"));
	LoadAnimationMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/Undead_WizardMesh.bin"));

	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/MeshArrowMesh.bin"));


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

	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGate_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGate_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/TowerSceneMesh/AD_RuneGateGround_B_01Mesh.bin"));

	auto hpBarMesh{ make_shared<PlaneMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.75f, 0.15f }) };
	auto skyboxMesh{ make_shared <SkyboxMesh>(device, commandlist, 20.0f, 20.0f, 20.0f) };
	auto magicCircleExtent = TriggerSetting::EXTENT[static_cast<INT>(TriggerType::ARROW_RAIN)];
	auto magicCircleMesh{ make_shared<PlaneMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ magicCircleExtent.x * 2, magicCircleExtent.z * 2 }) };

	m_meshs.insert({ "SKYBOX", skyboxMesh });
	m_meshs.insert({ "HPBAR", hpBarMesh });
	m_meshs.insert({ "MAGICCIRCLE", magicCircleMesh });


	// 텍스처 로딩
	auto hpBarTexture{ make_shared<Texture>() };
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Full_HpBar.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	hpBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Empty_HpBar.dds"), (INT)ShaderRegister::SubTexture); // SubTexture
	hpBarTexture->CreateSrvDescriptorHeap(device);
	hpBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	auto skyboxTexture{ make_shared<Texture>() };
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkyBox.dds"), (INT)ShaderRegister::SkyboxTexture);	// Skybox
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	auto magicCircleTexture{ make_shared<Texture>() };
	magicCircleTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/MagicCircle.dds"), (INT)ShaderRegister::BaseTexture);	// MagicCircle
	magicCircleTexture->CreateSrvDescriptorHeap(device);
	magicCircleTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	m_textures.insert({ "SKYBOX", skyboxTexture });
	m_textures.insert({ "HPBAR", hpBarTexture });
	m_textures.insert({ "MAGICCIRCLE", magicCircleTexture });


	// 메테리얼 로딩
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/Undead_WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/Undead_ArcherTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/Undead_WizardTexture.bin"));

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/Archer_WeaponArrowTexture.bin"));

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

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_RuneGate_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_RuneGate_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/TowerSceneTexture/AD_RuneGateGround_B_01Texture.bin"));

	// 애니메이션 로딩
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/Undead_WarriorAnimation.bin"), "Undead_WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/Undead_ArcherAnimation.bin"), "Undead_ArcherAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/Undead_WizardAnimation.bin"), "Undead_WizardAnimation");


	commandlist->Close();
	ID3D12CommandList* ppCommandList[]{ commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_GameFramework.WaitForPreviousFrame();

	m_loadEnd = true;
}

void TowerLoadingScene::DestroyObjects()
{
	m_loadingText.reset();
}

void TowerLoadingScene::Update(FLOAT timeElapsed) 
{
	m_loadingText->Update(timeElapsed);
	if (m_loadEnd) {
		ReleaseUploadBuffer();
		g_GameFramework.ChangeScene(SCENETAG::TowerScene);
	}
}

void TowerLoadingScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
{
}

void TowerLoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	m_loadingText->Render(deviceContext);
}

void TowerLoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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
		else if (strToken == "<Mesh>:") {
			auto mesh = make_shared<MeshFromFile>();
			mesh->LoadMesh(device, commandList, in);

			m_meshs.insert({ mesh->GetMeshName(), mesh });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}

	m_loadingText->LoadingFile();
}

void TowerLoadingScene::LoadAnimationMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

void TowerLoadingScene::LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

void TowerLoadingScene::LoadAnimationSetFromFile(wstring fileName, const string& animationSetName)
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
