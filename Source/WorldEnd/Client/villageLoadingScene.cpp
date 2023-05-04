#include "villageLoadingScene.h"

VillageLoadingScene::VillageLoadingScene() : m_loadEnd{ false }
{

}

VillageLoadingScene::~VillageLoadingScene()
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

	DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator)));
	DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList)));

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

			materials->LoadMaterials(device, commandList, in);

			m_globalMaterials.insert({ materials->m_materialName, materials });
			m_globalMaterials.insert({ "@" + materials->m_materialName, materials });
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

	m_globalAnimationSets.insert({ animationSetName, animationSet });
}
