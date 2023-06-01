#include "globalLoadingScene.h"

GlobalLoadingScene::GlobalLoadingScene() : m_loadEnd { false }
{

}

GlobalLoadingScene::~GlobalLoadingScene()
{

}

void GlobalLoadingScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
}

void GlobalLoadingScene::OnCreate(const ComPtr<ID3D12Device>& device,
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

	m_loadingThread = thread{ &GlobalLoadingScene::BuildObjects, this, device, m_threadCommandList, rootSignature, postRootSignature };
	m_loadingThread.detach();
}

void GlobalLoadingScene::OnDestroy()
{
	m_loadEnd = false;

	DestroyObjects();
}

void GlobalLoadingScene::ReleaseUploadBuffer()
{
	for (const auto& mesh : m_globalMeshs) mesh.second->ReleaseUploadBuffer();
	for (const auto& texture : m_globalTextures) texture.second->ReleaseUploadBuffer();
	for (const auto& material : m_globalMaterials) material.second->ReleaseUploadBuffer();
}

void GlobalLoadingScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist,
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	// 셰이더 로딩
	auto objectShader1{ make_shared<StaticObjectShader>(device, rootsignature) };
	auto objectShader2{ make_shared<StaticObjectShader>(device, rootsignature) };
	auto objectBlendShader{ make_shared<StaticObjectBlendShader>(device, rootsignature) };
	auto animationShader{ make_shared<AnimationShader>(device, rootsignature) };
	auto skyboxShader{ make_shared<SkyboxShader>(device, rootsignature) };
	auto uiShader{ make_shared<UIShader>(device, rootsignature) };
	auto postUiShader{ make_shared<UIShader>(device, rootsignature) };
	auto triggerEffectShader{ make_shared<TriggerEffectShader>(device, rootsignature) };
	auto horzGaugeShader{ make_shared<HorzGaugeShader>(device, rootsignature) };
	auto vertGaugeShader{ make_shared<VertGaugeShader>(device, rootsignature) };
	auto shadowShader{ make_shared<ShadowShader>(device, rootsignature) };
	auto animationShadowShader{ make_shared<AnimationShadowShader>(device, rootsignature) };
	auto emitterParticleShader{ make_shared<EmitterParticleShader>(device, rootsignature) };
	auto pumperParticleShader{ make_shared<PumperParticleShader>(device, rootsignature) };
	auto horzBlurShader{ make_shared<HorzBlurShader>(device, postRootSignature) };
	auto vertBlurShader{ make_shared<VertBlurShader>(device, postRootSignature) };
	auto sobelShader{ make_shared<SobelShader>(device, postRootSignature) };
	auto compositeShader{ make_shared<CompositeShader>(device, postRootSignature) };
	auto fadeShader{ make_shared<FadeShader>(device, postRootSignature) };
	//auto debugShader{ make_shared<DebugShader>(device, rootsignature) };

	auto arrowInstance{ make_shared<ArrowInstance>(device, rootsignature, MAX_ARROWRAIN_ARROWS) };

	m_globalShaders.insert({ "ANIMATION", animationShader });
	m_globalShaders.insert({ "OBJECT1", objectShader1 });
	m_globalShaders.insert({ "OBJECT2", objectShader2 });
	m_globalShaders.insert({ "OBJECTBLEND", objectBlendShader });
	m_globalShaders.insert({ "SKYBOX", skyboxShader });
	m_globalShaders.insert({ "TRIGGEREFFECT", triggerEffectShader });
	m_globalShaders.insert({ "HORZGAUGE", horzGaugeShader });
	m_globalShaders.insert({ "VERTGAUGE", vertGaugeShader });
	m_globalShaders.insert({ "SHADOW", shadowShader });
	m_globalShaders.insert({ "ANIMATIONSHADOW", animationShadowShader });
	m_globalShaders.insert({ "EMITTERPARTICLE", emitterParticleShader });
	m_globalShaders.insert({ "PUMPERPARTICLE", pumperParticleShader });
	m_globalShaders.insert({ "UI", uiShader });
	m_globalShaders.insert({ "POSTUI", postUiShader });
	m_globalShaders.insert({ "HORZBLUR", horzBlurShader });
	m_globalShaders.insert({ "VERTBLUR", vertBlurShader });
	m_globalShaders.insert({ "SOBEL", sobelShader });
	m_globalShaders.insert({ "COMPOSITE", compositeShader });
	m_globalShaders.insert({ "FADE", fadeShader });
	//m_globalShaders.insert({ "DEBUG", debugShader });

	m_globalShaders.insert({ "ARROW_INSTANCE", arrowInstance });

	// 메쉬 로딩
	LoadAnimationMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/WarriorMesh.bin"));
	LoadAnimationMeshFromFile(device, commandlist, TEXT("./Resource/Mesh/ArcherMesh.bin"));

	auto staminaBarMesh{ make_shared<PlaneMesh>(device, commandlist, XMFLOAT3{ 0.f, 0.f, 0.f }, XMFLOAT2{ 0.1f, 0.89f }) };
	m_globalMeshs.insert({ "STAMINABAR", staminaBarMesh });


	// 텍스처 로딩
	auto frameUITexture{ make_shared<Texture>() };
	frameUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/UI_Background.dds"), (INT)ShaderRegister::BaseTexture);
	frameUITexture->CreateSrvDescriptorHeap(device);
	frameUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto cancelUITexture{ make_shared<Texture>() };
	cancelUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/UI_Cancel.dds"), (INT)ShaderRegister::BaseTexture);
	cancelUITexture->CreateSrvDescriptorHeap(device);
	cancelUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto buttonUITexture{ make_shared<Texture>() };
	buttonUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/UI_Button.dds"), (INT)ShaderRegister::BaseTexture);
	buttonUITexture->CreateSrvDescriptorHeap(device);
	buttonUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto goldUITexture{ make_shared<Texture>() };
	goldUITexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Gold.dds"), (INT)ShaderRegister::BaseTexture);
	goldUITexture->CreateSrvDescriptorHeap(device);
	goldUITexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	auto warriorSkillTexture{ make_shared<Texture>() };
	warriorSkillTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Warrior_Skill.dds"), (INT)ShaderRegister::BaseTexture);
	warriorSkillTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Warrior_Skill_Cool.dds"), (INT)ShaderRegister::SubTexture);
	warriorSkillTexture->CreateSrvDescriptorHeap(device);
	warriorSkillTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto warriorUltimateTexture{ make_shared<Texture>() };
	warriorUltimateTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Warrior_Ultimate.dds"), (INT)ShaderRegister::BaseTexture);
	warriorUltimateTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Warrior_Ultimate_Cool.dds"), (INT)ShaderRegister::SubTexture);
	warriorUltimateTexture->CreateSrvDescriptorHeap(device);
	warriorUltimateTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto archerSkillTexture{ make_shared<Texture>() };
	archerSkillTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Archer_Skill.dds"), (INT)ShaderRegister::BaseTexture);
	archerSkillTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Archer_Skill_Cool.dds"), (INT)ShaderRegister::SubTexture);
	archerSkillTexture->CreateSrvDescriptorHeap(device);
	archerSkillTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
	auto archerUltimateTexture{ make_shared<Texture>() };
	archerUltimateTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Archer_Ultimate.dds"), (INT)ShaderRegister::BaseTexture);
	archerUltimateTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/SkillTexture/Archer_Ultimate_Cool.dds"), (INT)ShaderRegister::SubTexture);
	archerUltimateTexture->CreateSrvDescriptorHeap(device);
	archerUltimateTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	auto staminaBarTexture{ make_shared<Texture>() };
	staminaBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Full_StaminaBar.dds"), (INT)ShaderRegister::BaseTexture); // BaseTexture
	staminaBarTexture->LoadTextureFile(device, commandlist, TEXT("Resource/Texture/Empty_StaminaBar.dds"), (INT)ShaderRegister::SubTexture); // SubTexture
	staminaBarTexture->CreateSrvDescriptorHeap(device);
	staminaBarTexture->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);

	m_globalTextures.insert({ "FRAMEUI", frameUITexture });
	m_globalTextures.insert({ "CANCELUI", cancelUITexture });
	m_globalTextures.insert({ "BUTTONUI", buttonUITexture });
	m_globalTextures.insert({ "GOLDUI", goldUITexture });
	m_globalTextures.insert({ "WARRIORSKILL", warriorSkillTexture });
	m_globalTextures.insert({ "WARRIORULTIMATE", warriorUltimateTexture });
	m_globalTextures.insert({ "ARCHERSKILL", archerSkillTexture });
	m_globalTextures.insert({ "ARCHERULTIMATE", archerUltimateTexture });
	m_globalTextures.insert({ "STAMINABAR", staminaBarTexture });


	// 메테리얼 로딩
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/WarriorTexture.bin"));
	LoadMaterialFromFile(device, commandlist, TEXT("./Resource/Texture/ArcherTexture.bin"));


	// 애니메이션 로딩
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/WarriorAnimation.bin"), "WarriorAnimation");
	LoadAnimationSetFromFile(TEXT("./Resource/Animation/ArcherAnimation.bin"), "ArcherAnimation");
	// 텍스트 로딩
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

	commandlist->Close();
	ID3D12CommandList* ppCommandList[]{ commandlist.Get() };
	g_GameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_GameFramework.WaitForPreviousFrame();

	m_loadEnd = true;
}

void GlobalLoadingScene::DestroyObjects()
{
	m_loadingText.reset();
}

void GlobalLoadingScene::Update(FLOAT timeElapsed)
{
	m_loadingText->SetText(g_loadingText + L" 로딩 중.. " + to_wstring(g_loadingIndex) + m_maxFileCount);
	if (m_loadEnd) {
		ReleaseUploadBuffer();
		g_GameFramework.ChangeScene(SCENETAG::VillageLoadingScene);
	}
}

void GlobalLoadingScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
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
		break;
	}
	}
}

void GlobalLoadingScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	m_loadingText->Render(deviceContext);
}

void GlobalLoadingScene::LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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
			auto mesh = make_shared<AnimationMesh>();
			mesh->LoadAnimationMesh(device, commandList, in);
			mesh->SetType(AnimationSetting::STANDARD_MESH);

			m_globalMeshs.insert({ mesh->GetAnimationMeshName(), mesh });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void GlobalLoadingScene::LoadAnimationMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

			m_globalMeshs.insert({ animationMesh->GetAnimationMeshName(), animationMesh });
		}
		else if (strToken == "<Mesh>:") {
			auto mesh = make_shared<AnimationMesh>();
			mesh->LoadAnimationMesh(device, commandList, in);
			mesh->SetType(AnimationSetting::STANDARD_MESH);

			m_globalMeshs.insert({ mesh->GetAnimationMeshName(), mesh });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void GlobalLoadingScene::LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName)
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

			materials->LoadMaterials(device, commandList, in, true);

			m_globalMaterials.insert({ materials->m_materialName, materials });
			m_globalMaterials.insert({ "@" + materials->m_materialName, materials });
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void GlobalLoadingScene::LoadAnimationSetFromFile(wstring fileName, const string& animationSetName)
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

