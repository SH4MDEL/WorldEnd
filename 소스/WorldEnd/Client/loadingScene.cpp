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

void LoadingScene::ReleaseUploadBuffer()
{
	for (const auto& mesh : m_meshs) mesh.second->ReleaseUploadBuffer();
	for (const auto& texture : m_textures) texture.second->ReleaseUploadBuffer();
}

void LoadingScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const {}
void LoadingScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) const {}

void LoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT aspectRatio)
{
	// 플레이어 생성
	//auto playerShader{ make_shared<TextureHierarchyShader>(device, rootsignature) };
	auto playerShader{ make_shared<SkinnedAnimationShader>(device, rootsignature) };

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

	// 플레이어 메쉬, 메테리얼, 애니메이션 설정
	LoadAnimationFromFile(TEXT("./Resource/Animation/WarriorAnimation.bin"), "WarriorAnimation");
	LoadAnimationFromFile(TEXT("./Resource/Animation/ArcherAnimation.bin"), "ArcherAnimation");

	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/WarriorMesh.bin"));
	LoadMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/ArcherMesh.bin"));

	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/ArcherTexture.bin"));


	// 텍스처 설정
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

void LoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;
	string backup;
	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {}
		else if (strToken == "<SkinningInfo>:") {
			auto skinnedMesh = make_shared<SkinnedMesh>();
			skinnedMesh->LoadSkinnedMesh(device, commandList, in);
			skinnedMesh->SetMeshType(SKINNED_MESH);

			m_meshs.insert({ skinnedMesh->GetSkinnedMeshName(), skinnedMesh });
		}
		else if (strToken == "<Mesh>:") {
			auto mesh = make_shared<SkinnedMesh>();
			mesh->LoadSkinnedMesh(device, commandList, in);
			mesh->SetMeshType(STANDARD_MESH);

			m_meshs.insert({ mesh->GetSkinnedMeshName(), mesh });
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
}

void LoadingScene::LoadAnimationFromFile(wstring fileName, const string& animationName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	auto animationSet = make_shared<AnimationSet>();

	// frameNum = 뼈대 개수
	BYTE strLength{};
	INT num{}, frameNum{};

	INT animationCount{};

	while (true) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<AnimationSets>:") {
			// 애니메이션 조합에 애니메이션 갯수 resize
			in.read((char*)(&num), sizeof(INT));
			animationSet->GetAnimations().resize(num);
		}
		else if (strToken == "<FrameNames>:") {
			// 애니메이션이 적용될 뼈 개수 resize
			in.read((char*)(&frameNum), sizeof(INT));
			auto& frameNames = animationSet->GetFrameNames();
			frameNames.resize(frameNum);
			auto& frameCaches = animationSet->GetBoneFramesCaches();
			frameCaches.resize(frameNum);
			
			// 프레임이름을 담는 벡터에
			// 각 이름을 읽어서 넣음
			for (int i = 0; i < frameNum; ++i) {
				in.read((char*)(&strLength), sizeof(BYTE));
				frameNames[i].resize(strLength);
				in.read((char*)(frameNames[i].data()), sizeof(char) * strLength);
			}
		}
		else if (strToken == "<AnimationSet>:") {
			// 애니메이션 번호, 이름, 시간, 초당 프레임, 총 프레임
			INT animationNum{};
			in.read((char*)(&animationNum), sizeof(INT));

			in.read((char*)(&strLength), sizeof(BYTE));
			string animationName(strLength, '\0');
			in.read(&animationName[0], sizeof(char) * strLength);

			float animationLength{};
			in.read((char*)(&animationLength), sizeof(float));

			INT framePerSecond{};
			in.read((char*)(&framePerSecond), sizeof(INT));

			INT totalFrames{};
			in.read((char*)(&totalFrames), sizeof(INT));

			auto animation = make_shared<Animation>(animationLength, framePerSecond,
				totalFrames, frameNum, animationName);

			auto& keyFrameTimes = animation->GetKeyFrameTimes();
			auto& keyFrameTransforms = animation->GetKeyFrameTransforms();

			for (int i = 0; i < totalFrames; ++i) {
				in.read((char*)(&strLength), sizeof(BYTE));
				string strToken(strLength, '\0');
				in.read(&strToken[0], sizeof(char) * strLength);

				// 키프레임 번호, 키프레임 시간, 키프레임 행렬들
				if (strToken == "<Transforms>:") {
					INT keyFrameNum{};
					in.read((char*)(&keyFrameNum), sizeof(INT));

					float keyFrameTime{};
					in.read((char*)(&keyFrameTime), sizeof(float));

					keyFrameTimes[i] = keyFrameTime;
					in.read((char*)(keyFrameTransforms[i].data()), sizeof(XMFLOAT4X4) * frameNum);
				}
			}
			
			// 애니메이션 조합에 해당 애니메이션 추가
			animationSet->GetAnimations()[animationCount++] = animation;
		}
		else if (strToken == "</AnimationSets>") {
			break;
		}
	}

	m_animations.insert({ animationName, animationSet });
}


