#include "villageLoadingScene.h"

VillageLoadingScene::VillageLoadingScene() : m_loadEnd{ false }
{

}

VillageLoadingScene::~VillageLoadingScene()
{

}

void VillageLoadingScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
}

void VillageLoadingScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature,
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	g_loadingIndex = 0;
	m_loadingText = make_shared<Text>();

	m_loadingText->SetColorBrush("SKYBLUE");
	m_loadingText->SetTextFormat("KOPUB24");

	Utiles::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator)));
	Utiles::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList)));

	m_loadingThread = thread{ &VillageLoadingScene::BuildObjects, this, device, m_threadCommandList, rootSignature, postRootSignature };
	m_loadingThread.detach();
}

void VillageLoadingScene::OnDestroy()
{
	m_loadEnd = false;

	DestroyObjects();
}

void VillageLoadingScene::ReleaseUploadBuffer()
{
	for (const auto& mesh : m_meshs) mesh.second->ReleaseUploadBuffer();
	for (const auto& texture : m_textures) texture.second->ReleaseUploadBuffer();
	for (const auto& material : m_materials) material.second->ReleaseUploadBuffer();
}

void VillageLoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist,
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	// UI 로딩
	auto frameUITexture{ make_shared<Texture>() };
	frameUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/VillageSceneTexture/Title.dds"), (INT)ShaderRegister::BaseTexture);
	frameUITexture->CreateSrvDescriptorHeap(device);
	frameUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	m_textures.insert({ "TITLE", frameUITexture });

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Barrel_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Basket_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Basket_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_ETexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_FTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_GTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BilldingStair_HTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Box_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Bridge_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingDoor_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingPillar_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BuildingWindow_D_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_03Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_04Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_05Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_A_06Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_03Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_04Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_B_05Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_C_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Building_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleDoor_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_ETexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_FTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_GTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/CastleWall_HTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Castle_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Castle_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Chimney_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Decal_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Door_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flag_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flag_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flag_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flag_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_F_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Flower_G_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Frame_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_ETexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_FTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_GTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_HTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_ITexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_JTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_KTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_LTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_MTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_NTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_OTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_PTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_QTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/GrassTile_RTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_A_03Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_B_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Ivy_B_03Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Lamp_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Lamp_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_01_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_01_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_02_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_02_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_03_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_03_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_04_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_A_04_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_01_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_01_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_02_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_02_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_03_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_03_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_04_LeafTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Tree_B_04_TrunkTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/MV_Wagon_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_ETexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_FTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Pillar_GTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Plaza_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Statue_Pillar_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Stone_Statue_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_02Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_03Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/StoreBox_A_04Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_ETexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_FTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_GTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Store_HTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownChair_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_BTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_CTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_DTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_ETexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_FTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_GTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_HTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_ITexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_JTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_KTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_LTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_MTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_NTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_OTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_PTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_QTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_RTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_STexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_TTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_UTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_VTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/TownGroundTile_WTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Tree_branch_ATexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Windmill_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Windmill_Door_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Windmill_Wing_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Window_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/Window_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_A_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_B_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_C_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_D_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/WoodFrame_E_01Texture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/BlockingTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/VillageSceneTexture/NonblockingTexture.bin"));

	// 마을 씬 메쉬 로딩
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Barrel_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Basket_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Basket_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_EMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_FMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_GMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BilldingStair_HMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Box_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Bridge_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingDoor_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingPillar_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BuildingWindow_D_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_03Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_04Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_05Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_A_06Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_03Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_04Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_B_05Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_C_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Building_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleDoor_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_EMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_FMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_GMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/CastleWall_HMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Castle_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Castle_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Chimney_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Decal_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Door_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flag_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_F_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Flower_G_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Frame_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_EMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_FMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_GMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_HMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_IMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_JMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_KMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_LMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_MMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_NMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_OMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_PMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_QMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/GrassTile_RMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_A_03Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_B_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Ivy_B_03Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Lamp_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Lamp_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_A_LeafMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_A_TrunkMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_B_LeafMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Tree_B_TrunkMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/MV_Wagon_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_EMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_FMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Pillar_GMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Plaza_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Statue_Pillar_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Stone_Statue_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_02Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_03Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/StoreBox_A_04Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_EMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_FMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_GMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Store_HMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownChair_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_BMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_CMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_DMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_EMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_FMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_GMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_HMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_IMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_JMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_KMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_LMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_MMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_NMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_OMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_PMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_QMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_RMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_SMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_TMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_UMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_VMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/TownGroundTile_WMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Tree_branch_AMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Windmill_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Windmill_Door_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Windmill_Wing_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Window_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/Window_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_A_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_B_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_C_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_D_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/WoodFrame_E_01Mesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/BlockingMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/VillageSceneMesh/NonblockingMesh.bin"));

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

	shared_ptr<Mesh> wireFrame{ make_shared<Mesh>(device, commandlist, vertices ,indices) };
	m_meshs.insert({ "WIREFRAME", wireFrame });

	auto skyboxMesh{ make_shared <SkyboxMesh>(device, commandlist, 20.0f, 20.0f, 20.0f) };

	m_meshs.insert({ "SKYBOX", skyboxMesh });

	auto skyboxTexture{ make_shared<Texture>() };
	skyboxTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Skybox/VillageSkyBox.dds"), (INT)ShaderRegister::SkyboxTexture);	// Skybox
	skyboxTexture->CreateSrvDescriptorHeap(device);
	skyboxTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURECUBE);

	m_textures.insert({ "SKYBOX", skyboxTexture });

	commandlist->Close();
	ID3D12CommandList* ppCommandList[]{ commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_GameFramework.WaitForPreviousFrame();

	m_loadEnd = true;
}

void VillageLoadingScene::DestroyObjects()
{
	m_loadingText.reset();
}

void VillageLoadingScene::Update(FLOAT timeElapsed)
{
	m_loadingText->SetText(g_loadingText + L" 로딩 중.. " + to_wstring(g_loadingIndex) + m_maxFileCount);
	if (m_loadEnd) {
		ReleaseUploadBuffer();
		g_GameFramework.ChangeScene(SCENETAG::LoginScene);
	}
}

void VillageLoadingScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
{
	switch (threadIndex)
	{
	case 0:
	{
		break;
	}
	case 1:
	{
		break;
	}
	case 2:
	{
		//m_fadeFilter->Execute(commandList, renderTarget);
		break;
	}
	}
}

void VillageLoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	m_loadingText->Render(deviceContext);
}

void VillageLoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

void VillageLoadingScene::LoadAnimationMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

void VillageLoadingScene::LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

void VillageLoadingScene::LoadAnimationSetFromFile(wstring fileName, const string& animationSetName)
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
