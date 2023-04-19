#include "towerScene.h"

constexpr FLOAT FAR_POSITION = 10000.f;

TowerScene::TowerScene() : 
	m_NDCspace( 0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, -0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, 0.0f, 1.0f),
	m_sceneState{ (INT)State::None }
{}

TowerScene::~TowerScene()
{

}

void TowerScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	BuildObjects(device, commandList, rootSignature, postRootSignature);
}

void TowerScene::OnDestroy()
{
}

void TowerScene::ReleaseUploadBuffer() {}

void TowerScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
	if (!CheckState(State::CantPlayerControl)) {
		SetCursor(NULL);
		RECT windowRect; 
		GetWindowRect(hWnd, &windowRect);

		POINT prevPosition{ windowRect.left + width / 2, windowRect.top + height / 2 };
		POINT nextPosition; 
		GetCursorPos(&nextPosition);

		int dx = nextPosition.x - prevPosition.x;
		int dy = nextPosition.y - prevPosition.y;

		if (m_camera) m_camera->Rotate(0.f, dy * 5.0f * deltaTime, dx * 5.0f * deltaTime);
		SetCursorPos(prevPosition.x, prevPosition.y);
	}
}

void TowerScene::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	if (!CheckState(State::CantPlayerControl)) {
		if (m_player) m_player->OnProcessingMouseMessage(message, lParam);
	}
	if (CheckState(State::OutputExitUI)) {
		if (m_exitUI) m_exitUI->OnProcessingMouseMessage(message, lParam);
		if (!g_clickEventStack.empty()) {
			g_clickEventStack.top()();
			while (!g_clickEventStack.empty()) {
				g_clickEventStack.pop();
			}
		}
	}
	if (CheckState(State::OutputResult)) {
		if (m_resultUI) m_resultUI->OnProcessingMouseMessage(message, lParam);
		if (!g_clickEventStack.empty()) {
			g_clickEventStack.top()();
			while (!g_clickEventStack.empty()) {
				g_clickEventStack.pop();
			}
		}
	}
}

void TowerScene::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
		SetState(State::OutputExitUI);
		if (m_exitUI) m_exitUI->SetEnable();
	}
	else {
		ResetState(State::OutputExitUI);
		if (m_exitUI) m_exitUI->SetDisable();
	}
	if (!CheckState(State::CantPlayerControl)) {
		if (m_player) m_player->OnProcessingKeyboardMessage(timeElapsed);
	}
}

void TowerScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_sceneBuffer)));

	m_sceneBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_sceneBufferPointer));
}

void TowerScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	BoundingSphere sceneBounds{ XMFLOAT3{ 0.0f, 0.0f, 10.0f }, 60.f };

	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&m_lightSystem->m_lights[0].m_direction);
	XMVECTOR lightPos{ -2.0f * sceneBounds.Radius * lightDir };
	XMVECTOR targetPos = XMLoadFloat3(&sceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, m_lightView));

	float l = sphereCenterLS.x - sceneBounds.Radius;
	float b = sphereCenterLS.y - sceneBounds.Radius;
	float n = sphereCenterLS.z - sceneBounds.Radius;
	float r = sphereCenterLS.x + sceneBounds.Radius;
	float t = sphereCenterLS.y + sceneBounds.Radius;
	float f = sphereCenterLS.z + sceneBounds.Radius;

	m_lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	::memcpy(&m_sceneBufferPointer->lightView, &XMMatrixTranspose(m_lightView), sizeof(XMFLOAT4X4));
	::memcpy(&m_sceneBufferPointer->lightProj, &XMMatrixTranspose(m_lightProj), sizeof(XMFLOAT4X4));
	::memcpy(&m_sceneBufferPointer->NDCspace, &XMMatrixTranspose(m_NDCspace), sizeof(XMFLOAT4X4));

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_sceneBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::Scene, virtualAddress);

	//if (m_camera) m_camera->UpdateShaderVariable(commandList);
	//if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);
}

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	CreateShaderVariable(device, commandlist);

	// 화살 생성
	for (size_t i = 0; i < RoomSetting::MAX_ARROWS; ++i) {
		auto arrow = make_shared<Arrow>();
		LoadObjectFromFile(TEXT("./Resource/Model/Archer_WeaponArrow.bin"), arrow);
		m_arrows.insert({ i, arrow });
	}

	// 플레이어 생성
	m_player = make_shared<Player>();
	m_player->SetType(PlayerType::ARCHER);
	LoadPlayerFromFile(m_player);

	m_player->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });
	m_shaders["ANIMATION"]->SetPlayer(m_player);

	// 체력 바 생성
	SetHpBar(m_player);

	// 스테미나 바 생성
	auto staminaBar = make_shared<GaugeBar>(0.015f);
	staminaBar->SetMesh(m_meshs["STAMINABAR"]);
	staminaBar->SetTexture(m_textures["STAMINABAR"]);
	staminaBar->SetMaxGauge(m_player->GetMaxStamina());
	staminaBar->SetGauge(m_player->GetStamina());
	staminaBar->SetPosition(XMFLOAT3(FAR_POSITION, FAR_POSITION, FAR_POSITION));
	m_shaders["VERTGAUGE"]->SetObject(staminaBar);
	m_player->SetStaminaBar(staminaBar);

	// 카메라 생성
	m_camera = make_shared<ThirdPersonCamera>();
	m_camera->CreateShaderVariable(device, commandlist);
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 100.0f));
	m_camera->SetProjMatrix(projMatrix);

	m_shadow = make_shared<Shadow>(device, 4096 << 1, 4096 << 1);

	// 씬 로드
	LoadSceneFromFile(TEXT("./Resource/Scene/TowerScene.bin"), TEXT("TowerScene"));

	// 게이트 로드
	m_gate = make_shared<GameObject>();
	LoadObjectFromFile(TEXT("./Resource/Model/TowerScene/AD_Gate.bin"), m_gate);
	m_gate->SetPosition(RoomSetting::BATTLE_STARTER_POSITION);
	m_gate->SetScale(0.5f, 0.5f, 0.5f);
	m_shaders["OBJECT"]->SetObject(m_gate);

	// 스카이 박스 생성
	auto skybox{ make_shared<GameObject>() };
	skybox->SetMesh(m_meshs["SKYBOX"]);
	skybox->SetTexture(m_textures["SKYBOX"]);
	m_shaders["SKYBOX"]->SetObject(skybox);

	// 파티클 시스템 생성
	g_particleSystem = make_unique<ParticleSystem>(device, commandlist, 
		static_pointer_cast<ParticleShader>(m_shaders["EMITTERPARTICLE"]), 
		static_pointer_cast<ParticleShader>(m_shaders["PUMPERPARTICLE"]));

	g_particleSystem->CreateParticle(ParticleSystem::Type::PUMPER, XMFLOAT3{0.f, 0.f, 0.f});

	// UI 생성
	m_exitUI = make_shared<BackgroundUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{1.f, 1.f});
	auto exitUI{ make_shared<StandardUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.4f, 0.5f}) };
	exitUI->SetTexture(m_textures["FRAMEUI"]);
	auto exitTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.0f}, XMFLOAT2{100.f, 20.f}) };
	exitTextUI->SetText(L"던전에서 나가시겠습니까?");
	exitUI->SetChild(exitTextUI);
	auto exitButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.7f}, XMFLOAT2{0.15f, 0.075f}) };
	exitButtonUI->SetTexture(m_textures["BUTTONUI"]);
	exitButtonUI->SetClickEvent([]() {
		cout << "종료" << endl;
		});
	auto exitButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{10.f, 10.f}) };
	exitButtonTextUI->SetText(L"예");
	exitButtonUI->SetChild(exitButtonTextUI);
	exitUI->SetChild(exitButtonUI);

	m_exitUI->SetChild(exitUI);
	m_shaders["POSTUI"]->SetUI(m_exitUI);

	m_resultUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 1.f, 1.f });
	auto resultUI{ make_shared<StandardUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.4f, 0.5f}) };
	resultUI->SetTexture(m_textures["FRAMEUI"]);
	m_resultTextUI = make_shared<TextUI>(XMFLOAT2{0.f, 0.0f}, XMFLOAT2{100.f, 20.f});
	m_resultTextUI->SetText(L"클리어!");
	resultUI->SetChild(m_resultTextUI);
	auto resultButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.7f}, XMFLOAT2{0.15f, 0.075f}) };
	resultButtonUI->SetTexture(m_textures["BUTTONUI"]);
	resultButtonUI->SetClickEvent([&]() {
		m_resultUI->SetDisable();
		ResetState(State::OutputResult);
		});
	auto ResultButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{20.f, 10.f}) };
	ResultButtonTextUI->SetText(L"닫기");
	resultButtonUI->SetChild(ResultButtonTextUI);
	resultUI->SetChild(resultButtonUI);

	m_resultUI->SetChild(resultUI);
	m_resultUI->SetDisable();
	m_shaders["POSTUI"]->SetUI(m_resultUI);

	// 필터 생성
	m_blurFilter = make_unique<BlurFilter>(device, g_GameFramework.GetWindowWidth(), g_GameFramework.GetWindowHeight());
	m_fadeFilter = make_unique<FadeFilter>(device, g_GameFramework.GetWindowWidth(), g_GameFramework.GetWindowHeight());
	m_sobelFilter = make_unique<SobelFilter>(device, g_GameFramework.GetWindowWidth(), g_GameFramework.GetWindowHeight(), postRootSignature);

	// 오브젝트 설정	
	m_object.push_back(skybox);

	// 조명 생성
	CreateLight(device, commandlist);

	InitServer();
}

void TowerScene::CreateLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	m_lightSystem = make_shared<LightSystem>();
	m_lightSystem->m_globalAmbient = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.f };
	m_lightSystem->m_numLight = 8;

	m_lightSystem->m_lights[0].m_type = DIRECTIONAL_LIGHT;
	m_lightSystem->m_lights[0].m_ambient = XMFLOAT4{ 0.3f, 0.3f, 0.3f, 1.f };
	m_lightSystem->m_lights[0].m_diffuse = XMFLOAT4{ 0.7f, 0.7f, 0.7f, 1.f };
	m_lightSystem->m_lights[0].m_specular = XMFLOAT4{ 0.4f, 0.4f, 0.4f, 0.f };
	m_lightSystem->m_lights[0].m_direction = XMFLOAT3{ 1.f, -1.f, 1.f };
	m_lightSystem->m_lights[0].m_enable = true;

	m_lightSystem->m_lights[1].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[1].m_range = 5.f;
	m_lightSystem->m_lights[1].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[1].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[1].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[1].m_position = XMFLOAT3(-2.943f, -2.468f, -45.335f);
	m_lightSystem->m_lights[1].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[1].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[1].m_falloff = 3.f;
	m_lightSystem->m_lights[1].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[1].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[1].m_enable = true;

	m_lightSystem->m_lights[2].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[2].m_range = 5.f;
	m_lightSystem->m_lights[2].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[2].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[2].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[2].m_position = XMFLOAT3(-2.963f, -2.468f, -33.575f);
	m_lightSystem->m_lights[2].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[2].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[2].m_falloff = 3.f;
	m_lightSystem->m_lights[2].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[2].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[2].m_enable = true;

	m_lightSystem->m_lights[3].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[3].m_range = 5.f;
	m_lightSystem->m_lights[3].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[3].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[3].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[3].m_position = XMFLOAT3(-2.755f, -2.468f, -21.307f);
	m_lightSystem->m_lights[3].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[3].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[3].m_falloff = 3.f;
	m_lightSystem->m_lights[3].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[3].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[3].m_enable = true;

	m_lightSystem->m_lights[4].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[4].m_range = 5.f;
	m_lightSystem->m_lights[4].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[4].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[4].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[4].m_position = XMFLOAT3(-4.833f, 1.952f, 8.224f);
	m_lightSystem->m_lights[4].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[4].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[4].m_falloff = 3.f;
	m_lightSystem->m_lights[4].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[4].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[4].m_enable = true;

	m_lightSystem->m_lights[5].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[5].m_range = 5.f;
	m_lightSystem->m_lights[5].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[5].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[5].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[5].m_position = XMFLOAT3(4.796f, 1.952f, 8.044f);
	m_lightSystem->m_lights[5].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[5].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[5].m_falloff = 3.f;
	m_lightSystem->m_lights[5].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[5].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[5].m_enable = true;

	m_lightSystem->m_lights[6].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[6].m_range = 5.f;
	m_lightSystem->m_lights[6].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[6].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[6].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[6].m_position = XMFLOAT3(-4.433f, 1.952f, 39.324f);
	m_lightSystem->m_lights[6].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[6].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[6].m_falloff = 3.f;
	m_lightSystem->m_lights[6].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[6].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[6].m_enable = true;

	m_lightSystem->m_lights[7].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[7].m_range = 5.f;
	m_lightSystem->m_lights[7].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[7].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[7].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[7].m_position = XMFLOAT3(3.116f, 1.952f, 39.184f);
	m_lightSystem->m_lights[7].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[7].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[7].m_falloff = 3.f;
	m_lightSystem->m_lights[7].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[7].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[7].m_enable = true;

	m_lightSystem->CreateShaderVariable(device, commandlist);
}

void TowerScene::Update(FLOAT timeElapsed)
{
	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shaders)
		shader.second->Update(timeElapsed);
	g_particleSystem->Update(timeElapsed);
	m_fadeFilter->Update(timeElapsed);
}

void TowerScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex)
{
	switch (threadIndex)
	{
	case 0:
	{
		m_shaders["ANIMATION"]->Render(commandList, m_shaders["ANIMATIONSHADOW"]);
		break;
	}
	case 1:
	{
		m_shaders["OBJECT"]->Render(commandList, m_shaders["SHADOW"]);
		break;
	}
	case 2:
	{
		break;
	}
	}
}

void TowerScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);

	switch (threadIndex)
	{
	case 0:
	{
		m_shaders.at("ANIMATION")->Render(commandList);
		break;
	}
	case 1:
	{
		m_shaders.at("OBJECT")->Render(commandList);
		break;
	}
	case 2:
	{
		m_shaders.at("SKYBOX")->Render(commandList);
		g_particleSystem->Render(commandList);
		m_shaders.at("HORZGAUGE")->Render(commandList);
		m_shaders.at("VERTGAUGE")->Render(commandList);
		m_shaders.at("UI")->Render(commandList);
		break;
	}
	}
}

void TowerScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
{
	switch (threadIndex)
	{
	case 0:
	{
		if (CheckState(State::BlurLevel5)) {
			m_blurFilter->Execute(commandList, renderTarget, 1);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}
		else if (CheckState(State::BlurLevel4)) {
			m_blurFilter->Execute(commandList, renderTarget, 2);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}
		else if (CheckState(State::BlurLevel3)) {
			m_blurFilter->Execute(commandList, renderTarget, 3);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}
		else if (CheckState(State::BlurLevel2)) {
			m_blurFilter->Execute(commandList, renderTarget, 4);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}
		else if (CheckState(State::BlurLevel1)) {
			m_blurFilter->Execute(commandList, renderTarget, 5);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}

		//m_sobelFilter->Execute(commandList, renderTarget);
		//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		//m_shaders["COMPOSITE"]->Render(commandList);
		//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		break;
	}
	case 1:
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		m_shaders.at("POSTUI")->Render(commandList);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
		break;
	}
	case 2:
	{
		m_fadeFilter->Execute(commandList, renderTarget);
		break;
	}
	}
}

void TowerScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	if (m_exitUI) m_exitUI->RenderText(deviceContext);
	if (m_resultUI) m_resultUI->RenderText(deviceContext);
}

void TowerScene::LoadSceneFromFile(wstring fileName, wstring sceneName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;
	in.read((CHAR*)(&strLength), sizeof(BYTE));
	string strToken(strLength, '\0');
	in.read(&strToken[0], sizeof(CHAR) * strLength);
	INT objectCount;
	in.read((CHAR*)(&objectCount), sizeof(INT));

	for (int i = 0; i < objectCount; ++i) {
		auto object = make_shared<GameObject>();

		in.read((CHAR*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(CHAR) * strLength);

		in.read((CHAR*)(&strLength), sizeof(BYTE));
		string objectName(strLength, '\0');
		in.read(&objectName[0], sizeof(CHAR) * strLength);

		wstring wstrToken = L"";
		wstrToken.assign(objectName.begin(), objectName.end());
		wstring strPath = L"./Resource/Model/" + sceneName + L"/" + wstrToken + L".bin";

		LoadObjectFromFile(strPath, object);

		XMFLOAT4X4 worldMatrix;
		in.read((CHAR*)(&worldMatrix), sizeof(XMFLOAT4X4));
		object->SetWorldMatrix(worldMatrix);

		m_object.push_back(object);
		m_shaders["OBJECT"]->SetObject(object);
	}
}

void TowerScene::LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	BYTE strLength;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Hierarchy>:") {
			object->LoadObject(in);
		}
		else if (strToken == "</Hierarchy>") {
			break;
		}
	}
}

void TowerScene::LoadPlayerFromFile(const shared_ptr<Player>& player)
{
	wstring filePath{};
	string animationSet{};

	switch (player->GetType()) {
	case PlayerType::WARRIOR:
		filePath = TEXT("./Resource/Model/Warrior.bin");
		animationSet = "WarriorAnimation";
		break;

	case PlayerType::ARCHER: 
		filePath = TEXT("./Resource/Model/Archer.bin");
		animationSet = "ArcherAnimation";
		break; 
	}

	LoadObjectFromFile(filePath, player);

	player->SetAnimationSet(m_animationSets[animationSet]);
	player->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	player->GetAnimationController()->SetTrackEnable(1, false);
	player->GetAnimationController()->SetTrackEnable(2, false);
}

void TowerScene::LoadMonsterFromFile(const shared_ptr<Monster>& monster)
{
	wstring filePath{};
	string animationSet{};

	switch (monster->GetType()) {
	case MonsterType::WARRIOR:
		filePath = TEXT("./Resource/Model/Undead_Warrior.bin");
		animationSet = "Undead_WarriorAnimation";
		break;

	case MonsterType::ARCHER:
		filePath = TEXT("./Resource/Model/Undead_Archer.bin");
		animationSet = "Undead_ArcherAnimation";
		break;

	case MonsterType::WIZARD:
		filePath = TEXT("./Resource/Model/Undead_Wizard.bin");
		animationSet = "Undead_WizardAnimation";
		break;
	}

	LoadObjectFromFile(filePath, monster);

	monster->SetAnimationSet(m_animationSets[animationSet]);
	monster->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	monster->GetAnimationController()->SetTrackEnable(1, false);
	monster->GetAnimationController()->SetTrackEnable(2, false);
}

void TowerScene::SetHpBar(const shared_ptr<AnimationObject>& object)
{
	auto hpBar = make_shared<GaugeBar>();
	hpBar->SetMesh(m_meshs["HPBAR"]);
	hpBar->SetTexture(m_textures["HPBAR"]);
	hpBar->SetMaxGauge(object->GetMaxHp());
	hpBar->SetGauge(object->GetHp());
	hpBar->SetPosition(XMFLOAT3{ FAR_POSITION, FAR_POSITION, FAR_POSITION });
	m_shaders["HORZGAUGE"]->SetObject(hpBar);
	object->SetHpBar(hpBar);
}

void TowerScene::SetArrow(const shared_ptr<Player>& player, INT arrowId)
{
	auto& arrow = m_arrows[arrowId];
	arrow->SetPosition(Vector3::Add(player->GetPosition(), XMFLOAT3{ 0.f, 0.9f, 0.f }));
	
	arrow->SetVelocity({ Vector3::Mul(Vector3::Normalize(player->GetFront()), PlayerSetting::ARROW_SPEED) });
	arrow->Rotate(0.f, 0.f, player->GetYaw());

	m_shaders["OBJECT"]->SetArrow(arrowId, arrow);
}

void TowerScene::RotateToTarget(const shared_ptr<Player>& player, INT targetId)
{
	if (-1 == targetId) {
		return;
	}

	auto& monster = m_monsters[targetId];
	if (!monster)
		return;

	XMFLOAT3 dir = Vector3::Normalize(
		Vector3::Sub(monster->GetPosition(), player->GetPosition()));
	XMFLOAT3 front{ player->GetFront() };

	XMVECTOR radian{ XMVector3AngleBetweenNormals(XMLoadFloat3(&front), XMLoadFloat3(&dir))};
	FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };
	XMFLOAT3 clockwise{ Vector3::Cross(front, dir) };
	angle = clockwise.y < 0 ? -angle : angle;

	player->Rotate(0.f, 0.f, angle);
}

void TowerScene::RotateToTarget(const shared_ptr<Player>& player)
{
	INT target = SetTarget(player);
	RotateToTarget(player, target);
}

INT TowerScene::SetTarget(const shared_ptr<Player>& player)
{
	XMFLOAT3 sub{};
	float length{}, min_length{ std::numeric_limits<float>::max() };
	XMFLOAT3 pos{ player->GetPosition() };
	int target{ -1 };
	for (const auto& elm : m_monsters) {
		if (elm.second) {
			if (elm.second->GetHp() <= std::numeric_limits<float>::epsilon()) continue;
			// 체력이 0이면(죽었다면) 건너뜀

			sub = Vector3::Sub(pos, elm.second->GetPosition());
			length = Vector3::Length(sub);
			if (length < min_length && length < PlayerSetting::AUTO_TARET_RANGE + 2.f) {
				min_length = length;
				target = elm.first;
			}
		}
	}

	return target;
}

bool TowerScene::CheckState(State sceneState)
{
	return m_sceneState & (INT)sceneState;
}

void TowerScene::SetState(State sceneState)
{
	m_sceneState |= (1 << ((INT)sceneState - 1));
}

void TowerScene::ResetState(State sceneState)
{
	m_sceneState &= ~(1 << ((INT)sceneState - 1));
}


void TowerScene::InitServer()
{
#ifdef USE_NETWORK

	wcout.imbue(locale("korean"));
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		cout << "WSA START ERROR" << endl;
	}

	// socket 생성
	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (g_socket == INVALID_SOCKET) {
		cout << "SOCKET INIT ERROR!" << endl;
	}

	// connect
	SOCKADDR_IN server_address{};
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, g_serverIP.c_str(), &(server_address.sin_addr.s_addr));

	connect(g_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address));

	constexpr char name[10] = "HSC\0";
	CS_LOGIN_PACKET login_packet{};
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	login_packet.player_type = m_player->GetType();
	memcpy(login_packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);

	g_networkThread = thread{ &TowerScene::RecvPacket, this};
	g_networkThread.detach();

#endif
}

void TowerScene::SendPlayerData()
{
#ifdef USE_NETWORK
	CS_PLAYER_MOVE_PACKET move_packet;
	move_packet.size = sizeof(move_packet);
	move_packet.type = CS_PACKET_PLAYER_MOVE;
	move_packet.pos = m_player->GetPosition();
	move_packet.velocity = m_player->GetVelocity();
	move_packet.yaw = m_player->GetYaw();
	send(g_socket, reinterpret_cast<char*>(&move_packet), sizeof(move_packet), 0);
#endif
}

void TowerScene::RecvPacket()
{
	while (true) {

		char buf[BUF_SIZE] = {0};
		WSABUF wsabuf{ BUF_SIZE, buf };
		DWORD recv_byte{ 0 }, recv_flag{ 0 };

		if (WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr) == SOCKET_ERROR)
			ErrorDisplay("RecvSizeType");

		if(recv_byte > 0)
			PacketReassembly(wsabuf.buf, recv_byte);

	}
}


void TowerScene::ProcessPacket(char* ptr)
{
	//cout << "[Process Packet] Packet Type: " << (int)ptr[1] << endl;//test

	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
		RecvLoginOk(ptr);
		break;
	case SC_PACKET_ADD_OBJECT:
		RecvAddObject(ptr);
		break;
	case SC_PACKET_REMOVE_PLAYER:
		RecvRemovePlayer(ptr);
		break;
	case SC_PACKET_REMOVE_MONSTER:
		RecvRemoveMonster(ptr);
		break;
	case SC_PACKET_UPDATE_CLIENT:
		RecvUpdateClient(ptr);
		break;
	case SC_PACKET_ADD_MONSTER:
		RecvAddMonster(ptr);
		break;
	case SC_PACKET_UPDATE_MONSTER:
		RecvUpdateMonster(ptr);
		break;
	case SC_PACKET_CHANGE_MONSTER_BEHAVIOR:
		RecvChangeMonsterBehavior(ptr);
		break;
	case SC_PACKET_CHANGE_ANIMATION:
		RecvChangeAnimation(ptr);
		break;
	case SC_PACKET_RESET_COOLDOWN:
		RecvResetCooldown(ptr);
		break;
	case SC_PACKET_CLEAR_FLOOR:
		RecvClearFloor(ptr);
		break;
	case SC_PACKET_FAIL_FLOOR:
		RecvFailFloor(ptr);
		break;
	case SC_PACKET_MONSTER_HIT:
		RecvMonsterHit(ptr);
		break;
	case SC_PACKET_CHANGE_STAMINA:
		RecvChangeStamina(ptr);
		break;
	case SC_PACKET_MONSTER_ATTACK_COLLISION:
		RecvMonsterAttackCollision(ptr);
		break;
	case SC_PACKET_SET_INTERACTABLE:
		RecvSetInteractable(ptr);
		break;
	case SC_PACKET_START_BATTLE:
		RecvStartBattle(ptr);
		break;
	case SC_PACKET_WARP_NEXT_FLOOR:
		RecvWarpNextFloor(ptr);
		break;
	case SC_PACKET_PLAYER_DEATH:
		RecvPlayerDeath(ptr);
		break;
	case SC_PACKET_PLAYER_SHOOT:
		RecvPlayerShoot(ptr);
		break;
	case SC_PACKET_REMOVE_ARROW:
		RecvRemoveArrow(ptr);
		break;
	}
}



void TowerScene::PacketReassembly(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t make_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];
	ZeroMemory(packet_buffer, BUF_SIZE);

	while (io_byte != 0) {
		if (make_packet_size == 0)
			make_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= make_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, make_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += make_packet_size - saved_packet_size;
			io_byte -= make_packet_size - saved_packet_size;
			//cout << "io byte - " << io_byte << endl;
			make_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void TowerScene::RecvLoginOk(char* ptr)
{
	SC_LOGIN_OK_PACKET* packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(ptr);
	m_player->SetId(packet->player_data.id);
}

void TowerScene::RecvAddObject(char* ptr)
{
	SC_ADD_OBJECT_PACKET* packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);

	PLAYER_DATA player_data{};

	player_data.hp = packet->player_data.hp;
	player_data.id = packet->player_data.id;
	player_data.pos = packet->player_data.pos;

	auto multiPlayer = make_shared<Player>();
	multiPlayer->SetType(packet->player_type);
	LoadPlayerFromFile(multiPlayer);

	multiPlayer->SetPosition(XMFLOAT3{ 0.f, 0.f, 0.f });

	m_multiPlayers.insert({ player_data.id, multiPlayer });

	SetHpBar(multiPlayer);

	m_shaders["ANIMATION"]->SetMultiPlayer(player_data.id, multiPlayer);
	cout << "add player" << static_cast<int>(player_data.id) << endl;

}

void TowerScene::RecvRemovePlayer(char* ptr)
{
	SC_REMOVE_PLAYER_PACKET* packet = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(ptr);

	m_multiPlayers[packet->id]->SetPosition(XMFLOAT3(FAR_POSITION, FAR_POSITION, FAR_POSITION));
}

void TowerScene::RecvRemoveMonster(char* ptr)
{
	SC_REMOVE_MONSTER_PACKET* packet = reinterpret_cast<SC_REMOVE_MONSTER_PACKET*>(ptr);
	m_monsters[packet->id]->SetPosition(XMFLOAT3(FAR_POSITION, FAR_POSITION, FAR_POSITION));

	// 임시 삭제
	m_shaders["ANIMATION"]->RemoveMonster(packet->id);
	m_monsters.erase(packet->id);
}

void TowerScene::RecvUpdateClient(char* ptr)
{
	SC_UPDATE_CLIENT_PACKET* packet = reinterpret_cast<SC_UPDATE_CLIENT_PACKET*>(ptr);

	if (-1 == packet->data.id) {
		return;
	}

	if (packet->data.id == m_player->GetId()) {
		m_player->SetPosition(packet->data.pos);
	}
	else {
		// towerScene의 multiPlayer를 업데이트 해도 shader의 multiPlayer도 업데이트 됨.
		XMFLOAT3 playerPosition = packet->data.pos;
		auto& player = m_multiPlayers[packet->data.id];
		player->SetPosition(playerPosition);
		player->SetVelocity(packet->data.velocity);
		player->Rotate(0.f, 0.f, packet->data.yaw - player->GetYaw());
		player->SetHp(packet->data.hp);
	}
}

void TowerScene::RecvAddMonster(char* ptr)
{
	SC_ADD_MONSTER_PACKET* packet = reinterpret_cast<SC_ADD_MONSTER_PACKET*>(ptr);

	MONSTER_DATA monster_data{};

	monster_data.id = packet->monster_data.id;
	monster_data.pos = packet->monster_data.pos;
	monster_data.hp = packet->monster_data.hp;

	if (monster_data.id == -1)
		return;

	auto monster = make_shared<Monster>();
	monster->SetType(packet->monster_type);
	LoadMonsterFromFile(monster);
	monster->ChangeAnimation(ObjectAnimation::WALK);

	monster->SetPosition(XMFLOAT3{ monster_data.pos.x, monster_data.pos.y, monster_data.pos.z });
	m_monsters.insert({ static_cast<INT>(monster_data.id), monster });

	SetHpBar(monster);
}

void TowerScene::RecvUpdateMonster(char* ptr)
{
	SC_UPDATE_MONSTER_PACKET* packet = reinterpret_cast<SC_UPDATE_MONSTER_PACKET*>(ptr);

	if (packet->monster_data.id < 0) return;

	auto& monster = m_monsters[packet->monster_data.id];
	if (!monster)
		return;

	monster->SetPosition(packet->monster_data.pos);
	monster->SetHp(packet->monster_data.hp);

	FLOAT newYaw = packet->monster_data.yaw;
	FLOAT oldYaw = monster->GetYaw();
	monster->Rotate(0.f, 0.f, newYaw - oldYaw);
}

void TowerScene::RecvChangeMonsterBehavior(char* ptr)
{
	SC_CHANGE_MONSTER_BEHAVIOR_PACKET* packet =
		reinterpret_cast<SC_CHANGE_MONSTER_BEHAVIOR_PACKET*>(ptr);

	auto& monster = m_monsters[packet->id];
	if (!monster)
		return;

	monster->ChangeAnimation(packet->animation);
	//monster->ChangeBehavior(packet->behavior);
}

void TowerScene::RecvResetCooldown(char* ptr)
{
	SC_RESET_COOLDOWN_PACKET* packet = reinterpret_cast<SC_RESET_COOLDOWN_PACKET*>(ptr);
	m_player->ResetCooldown(packet->cooldown_type);
}

void TowerScene::RecvClearFloor(char* ptr)
{
	SC_CLEAR_FLOOR_PACKET* packet = reinterpret_cast<SC_CLEAR_FLOOR_PACKET*>(ptr);
	cout << packet->reward << endl;
	SetState(State::OutputResult);
	m_resultUI->SetEnable();
}

void TowerScene::RecvFailFloor(char* ptr)
{
	SC_FAIL_FLOOR_PACKET* packet = reinterpret_cast<SC_FAIL_FLOOR_PACKET*>(ptr);
	
}

void TowerScene::RecvMonsterHit(char* ptr)
{
	SC_CREATE_MONSTER_HIT* packet = reinterpret_cast<SC_CREATE_MONSTER_HIT*>(ptr);

	auto& monster = m_monsters[packet->id];
	if (!monster)
		return;

	monster->SetHp(packet->hp);

	// 파티클 생성
	XMFLOAT3 particlePosition = monster->GetPosition();
	particlePosition.y += 1.f;
	g_particleSystem->CreateParticle(ParticleSystem::Type::EMITTER, particlePosition);
}

void TowerScene::RecvChangeAnimation(char* ptr)
{
	SC_CHANGE_ANIMATION_PACKET* packet = reinterpret_cast<SC_CHANGE_ANIMATION_PACKET*>(ptr);

	if (packet->id == m_player->GetId()) {
		if (PlayerType::ARCHER == m_player->GetType() &&
			(packet->animation == ObjectAnimation::ATTACK ||
				packet->animation == PlayerAnimation::SKILL))
		{
			RotateToTarget(m_player);
		}
	}
	else {
		auto& player = m_multiPlayers[packet->id];
		player->ChangeAnimation(packet->animation, false);

		if (PlayerType::ARCHER == player->GetType() &&
			(packet->animation == ObjectAnimation::ATTACK ||
			packet->animation == PlayerAnimation::SKILL))
		{
			RotateToTarget(player);
		}
	}
}

void TowerScene::RecvChangeStamina(char* ptr)
{
	SC_CHANGE_STAMINA_PACKET* packet = reinterpret_cast<SC_CHANGE_STAMINA_PACKET*>(ptr);
	m_player->SetStamina(packet->stamina);
}

void TowerScene::RecvMonsterAttackCollision(char* ptr)
{
	SC_MONSTER_ATTACK_COLLISION_PACKET* packet =
		reinterpret_cast<SC_MONSTER_ATTACK_COLLISION_PACKET*>(ptr);

	for (size_t i = 0; INT id : packet->ids) {
		if (-1 == id) break;

		if (id == m_player->GetId()) {
			//m_player->ChangeAnimation(ObjectAnimation::HIT);
			m_player->SetHp(packet->hps[i]);
		}
		else {
			//m_multiPlayers[id]->ChangeAnimation(ObjectAnimation::HIT, true);
			m_multiPlayers[id]->SetHp(packet->hps[i]);
		}
		++i;
	}
}

void TowerScene::RecvSetInteractable(char* ptr)
{
	SC_SET_INTERACTABLE_PACKET* packet = reinterpret_cast<SC_SET_INTERACTABLE_PACKET*>(ptr);

	m_player->SetInteractable(packet->interactable);
	m_player->SetInteractableType(packet->interactable_type);

	cout << "충돌 상태 : " << packet->interactable << endl;
}

void TowerScene::RecvStartBattle(char* ptr)
{
	SC_START_BATTLE_PACKET* packet = reinterpret_cast<SC_START_BATTLE_PACKET*>(ptr);

	// 오브젝트와 상호작용했다면 해당 오브젝트는 다시 상호작용 X
	m_player->SetInteractable(false);
	m_player->SetInteractableType(InteractableType::NONE);

	if(m_shaders["OBJECT"]->FindObject(m_gate))
		m_shaders["OBJECT"]->RemoveObject(m_gate);

	for (auto& monster : m_monsters) {
		m_shaders["ANIMATION"]->SetMonster(monster.first, monster.second);
	}
}

void TowerScene::RecvWarpNextFloor(char* ptr)
{
	SC_WARP_NEXT_FLOOR_PACKET* packet = reinterpret_cast<SC_WARP_NEXT_FLOOR_PACKET*>(ptr);
	
	m_fadeFilter->FadeOut([&]() {
		XMFLOAT3 startPosition{ 0.f, 0.f, 0.f };
		m_player->SetPosition(startPosition);

		for (auto& elm : m_multiPlayers) {
			elm.second->SetPosition(startPosition);
		}

		m_shaders["OBJECT"]->SetObject(m_gate);

		for (size_t i = 0; i < ActionType::COUNT; ++i) {
			m_player->ResetCooldown(static_cast<char>(i));
		}

		m_fadeFilter->FadeIn([&](){});
	});
}

void TowerScene::RecvPlayerDeath(char* ptr)
{
	SC_PLAYER_DEATH_PACKET* packet = reinterpret_cast<SC_PLAYER_DEATH_PACKET*>(ptr);
	
	if (packet->id == m_player->GetId()) {
		m_player->ChangeAnimation(ObjectAnimation::DEATH, false);
	}
	else {
		m_multiPlayers[packet->id]->ChangeAnimation(ObjectAnimation::DEATH, false);
	}
}

void TowerScene::RecvPlayerShoot(char* ptr)
{
	SC_PLAYER_SHOOT_PACKET* packet = reinterpret_cast<SC_PLAYER_SHOOT_PACKET*>(ptr);

	if (packet->id == m_player->GetId()) {
		//RotateToTarget(m_player, packet->target_id);
		SetArrow(m_player, packet->arrow_id);
	}
	else {
		auto& player = m_multiPlayers[packet->id];
		//RotateToTarget(player, packet->target_id);
		SetArrow(player, packet->arrow_id);
	}
}

void TowerScene::RecvRemoveArrow(char* ptr)
{
	SC_REMOVE_ARROW_PACKET* packet = reinterpret_cast<SC_REMOVE_ARROW_PACKET*>(ptr);

	auto arrow = m_shaders["OBJECT"]->FindArrow(packet->arrow_id);
	if (!arrow)
		return;

	arrow->Reset();
	m_shaders["OBJECT"]->RemoveArrow(packet->arrow_id);
}
