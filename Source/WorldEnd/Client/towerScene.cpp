#include "towerScene.h"

constexpr FLOAT FAR_POSITION = 10000.f;

TowerScene::TowerScene() : 
	m_NDCspace( 0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, -0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, 0.0f, 1.0f),
	m_sceneState{ (INT)State::Unused },
	m_accumulatedTime{ 0 }
{}

TowerScene::~TowerScene() {}

void TowerScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
	if (m_blurFilter) m_blurFilter->OnResize(device, width, height);
	if (m_fadeFilter) m_fadeFilter->OnResize(device, width, height);
	if (m_sobelFilter) m_sobelFilter->OnResize(device, width, height);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 100.0f));
	if (m_camera) m_camera->SetProjMatrix(projMatrix);
}

void TowerScene::OnCreate(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	m_sceneState = (INT)State::Unused;
	BuildObjects(device, commandList, rootSignature, postRootSignature);
}

void TowerScene::OnDestroy()
{
	m_meshs.clear();
	m_textures.clear();
	m_materials.clear();
	m_animationSets.clear();

	for (auto& shader : m_globalShaders) shader.second->Clear();

	DestroyObjects();
}

void TowerScene::ReleaseUploadBuffer() {}

void TowerScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	Utiles::ThrowIfFailed(device->CreateCommittedResource(
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
	// https://scahp.tistory.com/39

	const array<FLOAT, CASCADES_NUM + 1> cascadeBorders = { 0.f, 0.05f, 0.2f, 1.f };

	::memcpy(&m_sceneBufferPointer->NDCspace, &XMMatrixTranspose(m_NDCspace), sizeof(XMFLOAT4X4));

	for (UINT cascade = 0; cascade < cascadeBorders.size() - 1; ++cascade) {
		// NDC Space에서 Unit Cube의 각 꼭지점
		array<XMFLOAT3, 8> frustumVertices =
		{
			XMFLOAT3{ -1.0f,  1.0f, -1.0f },
			XMFLOAT3{  1.0f,  1.0f, -1.0f },
			XMFLOAT3{  1.0f, -1.0f, -1.0f },
			XMFLOAT3{ -1.0f, -1.0f, -1.0f },
			XMFLOAT3{ -1.0f,  1.0f, 1.0f },
			XMFLOAT3{  1.0f,  1.0f, 1.0f },
			XMFLOAT3{  1.0f, -1.0f, 1.0f },
			XMFLOAT3{ -1.0f, -1.0f, 1.0f }
		};

		// NDC Space -> World Space로 변환
		auto cameraViewProj = Matrix::Inverse(Matrix::Mul(m_camera->GetViewMatrix(), m_camera->GetProjMatrix()));
		for (auto& frustumVertex : frustumVertices) {
			frustumVertex = Vector3::TransformCoord(frustumVertex, cameraViewProj);
		}

		// Unit Cube의 각 코너 위치를 Slice에 맞게 설정
		for (UINT i = 0; i < 4; ++i) {
			XMFLOAT3 frontVector = Vector3::Sub(frustumVertices[i + 4], frustumVertices[i]);
			XMFLOAT3 nearRay = Vector3::Mul(frontVector, cascadeBorders[cascade]);
			XMFLOAT3 farRay = Vector3::Mul(frontVector, cascadeBorders[cascade + 1]);
			frustumVertices[i + 4] = Vector3::Add(frustumVertices[i], farRay);
			frustumVertices[i] = Vector3::Add(frustumVertices[i], nearRay);
		}

		// 뷰 프러스텀의 중심을 구함
		XMFLOAT3 frustumCenter{ 0.f, 0.f, 0.f };
		for (UINT i = 0; i < 8; ++i) frustumCenter = Vector3::Add(frustumCenter, frustumVertices[i]);
		frustumCenter = Vector3::Mul(frustumCenter, (1.f / 8.f));

		// 뷰 프러스텀을 감싸는 바운딩 스피어의 반지름을 구함
		FLOAT sphereRadius = 0.f;
		for (UINT i = 0; i < 8; ++i) {
			FLOAT dist = Vector3::Length(Vector3::Sub(frustumVertices[i], frustumCenter));
			sphereRadius = max(sphereRadius, dist);
		}

		BoundingSphere frustumBounds{ frustumCenter, sphereRadius };

		// 빛의 방향으로 정렬된 섀도우 맵에 사용할 뷰 매트릭스 생성
		XMVECTOR lightDir = XMLoadFloat3(&m_lightSystem->m_lights[(INT)LightTag::Directional].m_direction);
		XMVECTOR lightPos{ XMLoadFloat3(&frustumBounds.Center) - frustumBounds.Radius * lightDir };
		XMVECTOR targetPos = XMLoadFloat3(&frustumBounds.Center);
		XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));
		
		// 빛의 위치를 뒤로 조정하기 위한 오프셋
		const FLOAT offset = 50.f;

		float l = sphereCenterLS.x - frustumBounds.Radius;
		float b = sphereCenterLS.y - frustumBounds.Radius;
		float n = sphereCenterLS.z - frustumBounds.Radius - offset;
		float r = sphereCenterLS.x + frustumBounds.Radius;
		float t = sphereCenterLS.y + frustumBounds.Radius;
		float f = sphereCenterLS.z + frustumBounds.Radius + offset;
		
		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		memcpy(&m_sceneBufferPointer->lightView[cascade], &XMMatrixTranspose(lightView), sizeof(XMFLOAT4X4));
		memcpy(&m_sceneBufferPointer->lightProj[cascade], &XMMatrixTranspose(lightProj), sizeof(XMFLOAT4X4));
	}
	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_sceneBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::Scene, virtualAddress);
}

void TowerScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist,
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	CreateShaderVariable(device, commandlist);

	// 플레이어 생성
	m_player = make_shared<Player>();
	m_player->SetType(g_selectedPlayerType);
	LoadPlayerFromFile(m_player);

	m_globalShaders["ANIMATION"]->SetPlayer(m_player);
	m_player->SetPosition(RoomSetting::START_POSITION);

	// 체력 바 생성
	SetHpBar(m_player);

	// 스테미나 바 생성
	auto staminaBar = make_shared<GaugeBar>(0.015f);
	staminaBar->SetMesh("STAMINABAR");
	staminaBar->SetTexture("STAMINABAR");
	staminaBar->SetMaxGauge(m_player->GetMaxStamina());
	staminaBar->SetGauge(m_player->GetStamina());
	staminaBar->SetPosition(XMFLOAT3(FAR_POSITION, FAR_POSITION, FAR_POSITION));
	m_globalShaders["VERTGAUGE"]->SetObject(staminaBar);
	m_player->SetStaminaBar(staminaBar);

	// 카메라 생성
	m_camera = make_shared<ThirdPersonCamera>();
	m_camera->CreateShaderVariable(device, commandlist);
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 100.0f));
	m_camera->SetProjMatrix(projMatrix);

	m_shadow = make_shared<Shadow>(device, 1024, 1024);

	// 씬 로드
	LoadSceneFromFile(TEXT("./Resource/Scene/TowerScene.bin"), TEXT("TowerScene"));
	LoadMap();

	// 게이트 로드
	m_gate = make_shared<WarpGate>();
	LoadObjectFromFile(TEXT("./Resource/Model/TowerScene/AD_Gate.bin"), m_gate);
	m_gate->SetPosition(RoomSetting::BATTLE_STARTER_POSITION);
	m_gate->SetScale(0.5f, 0.5f, 0.5f);
	m_globalShaders["OBJECT"]->SetObject(m_gate);

	// 스카이 박스 생성
	auto skybox{ make_shared<GameObject>() };
	skybox->SetMesh("SKYBOX");
	skybox->SetTexture("SKYBOX");
	m_globalShaders["SKYBOX"]->SetObject(skybox);

	// 파티클 시스템 생성
	g_particleSystem = make_unique<ParticleSystem>(device, commandlist,
		static_pointer_cast<ParticleShader>(m_globalShaders["EMITTERPARTICLE"]),
		static_pointer_cast<ParticleShader>(m_globalShaders["PUMPERPARTICLE"]));
	m_towerObjectManager = make_unique<TowerObjectManager>(device, commandlist,
		m_globalShaders["OBJECT"], m_globalShaders["TRIGGEREFFECT"], 
		static_pointer_cast<InstancingShader>(m_globalShaders["ARROW_INSTANCE"]));

	BuildUI(device, commandlist);

	// 필터 생성
	auto windowWidth = g_GameFramework.GetWindowWidth();
	auto windowHeight = g_GameFramework.GetWindowHeight();
	m_blurFilter = make_unique<BlurFilter>(device, windowWidth, windowHeight);
	m_fadeFilter = make_unique<FadeFilter>(device, windowWidth, windowHeight);
	m_sobelFilter = make_unique<SobelFilter>(device, windowWidth, windowHeight, postRootSignature);

	//auto debugObject = make_shared<GameObject>();
	//debugObject->SetMesh("DEBUG");
	//m_globalShaders["DEBUG"]->SetObject(debugObject);

	// 조명 생성
	BuildLight(device, commandlist);

	InitServer();
}

void TowerScene::DestroyObjects()
{
	m_player.reset();
	m_camera.reset();

	m_gate.reset();
	m_portal.reset();

	m_towerObjectManager.reset();
	m_lightSystem.reset();
	m_shadow.reset();

	m_blurFilter.reset();
	m_fadeFilter.reset();
	m_sobelFilter.reset();

	g_particleSystem.reset();

	for (auto& hpUI : m_hpUI) {
		hpUI.reset();
	}
	m_idSet.clear();
	m_interactUI.reset();
	m_interactTextUI.reset();
	m_exitUI.reset();
	m_resultUI.reset();
	m_resultTextUI.reset();
	m_resultRewardTextUI.reset();

	m_multiPlayers.clear();
	m_monsters.clear();

	for (auto& structure : m_structures) {
		structure.reset();
	}
	for (auto& invisibleWall : m_invisibleWalls) {
		invisibleWall.reset();
	}
}

void TowerScene::BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	for (int i = 0; i < m_hpUI.size(); ++i) {
		m_hpUI[i] = make_shared<HorzGaugeUI>(XMFLOAT2{ -0.75f, 0.75f - i * 0.3f }, XMFLOAT2{ 0.4f, 0.08f }, 0.f);
		m_hpUI[i]->SetTexture("HPBAR");
		m_hpUI[i]->SetMaxGauge(100.f);
		m_hpUI[i]->SetDisable();
		m_globalShaders["UI"]->SetUI(m_hpUI[i]);
	}

	m_interactUI = make_shared<StandardUI>(XMFLOAT2{ 0.25f, 0.15f }, XMFLOAT2{ 0.24f, 0.08f });
	m_interactUI->SetTexture("BUTTONUI");
	m_interactTextUI = make_shared<TextUI>(XMFLOAT2{0.f, -0.2f}, XMFLOAT2{80.f, 20.f});
	m_interactTextUI->SetColorBrush("WHITE");
	m_interactTextUI->SetTextFormat("KOPUB18");
	m_interactUI->SetChild(m_interactTextUI);
	m_interactUI->SetDisable();
	m_globalShaders["UI"]->SetUI(m_interactUI);

	m_skillUI = make_shared<VertGaugeUI>(XMFLOAT2{ -0.60f, -0.75f }, XMFLOAT2{ 0.15f, 0.15f }, 0.f);
	auto eText = make_shared<TextUI>(XMFLOAT2{ 0.f, -0.8f }, XMFLOAT2{ 0.15f, 0.15f });
	eText->SetColorBrush("WHITE");
	eText->SetTextFormat("KOPUB24");
	eText->SetText(TEXT("E"));
	m_skillUI->SetChild(eText);
	m_ultimateUI = make_shared<VertGaugeUI>(XMFLOAT2{ -0.85f, -0.75f }, XMFLOAT2{ 0.15f, 0.15f }, 0.f);
	auto qText = make_shared<TextUI>(XMFLOAT2{ 0.f, -0.8f }, XMFLOAT2{ 0.15f, 0.15f });
	qText->SetColorBrush("WHITE");
	qText->SetTextFormat("KOPUB24");
	qText->SetText(TEXT("Q"));
	m_ultimateUI->SetChild(qText);
	if (g_selectedPlayerType == PlayerType::WARRIOR) {
		m_skillUI->SetTexture("WARRIORSKILL");
		m_ultimateUI->SetTexture("WARRIORULTIMATE");
	}
	if (g_selectedPlayerType == PlayerType::ARCHER) {
		m_skillUI->SetTexture("ARCHERSKILL");
		m_ultimateUI->SetTexture("ARCHERULTIMATE");
	}
	m_globalShaders["UI"]->SetUI(m_skillUI);
	m_player->SetSkillGauge(m_skillUI);
	m_globalShaders["UI"]->SetUI(m_ultimateUI);
	m_player->SetUltimateGauge(m_ultimateUI);
	m_player->ResetAllCooldown();

	m_exitUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 1.f, 1.f }); 
	auto exitUI{ make_shared<StandardUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.4f, 0.5f}) };
	exitUI->SetTexture("FRAMEUI");
	auto exitTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{120.f, 20.f}) };
	exitTextUI->SetText(L"던전에서 나가시겠습니까?");
	exitTextUI->SetColorBrush("WHITE");
	exitTextUI->SetTextFormat("KOPUB18");
	exitUI->SetChild(exitTextUI);
	auto exitButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.7f}, XMFLOAT2{0.15f, 0.075f}) };
	exitButtonUI->SetTexture("BUTTONUI");
	exitButtonUI->SetClickEvent([&]() {
		SetState(State::Fading);
		m_fadeFilter->FadeOut([&]() {
			SetState(State::SceneLeave);
		});
	});
	auto exitButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{10.f, 10.f}) };
	exitButtonTextUI->SetText(L"예");
	exitButtonTextUI->SetColorBrush("WHITE");
	exitButtonTextUI->SetTextFormat("KOPUB18");
	exitButtonUI->SetChild(exitButtonTextUI);
	exitUI->SetChild(exitButtonUI);

	m_exitUI->SetChild(exitUI);
	m_exitUI->SetDisable();
	m_globalShaders["POSTUI"]->SetUI(m_exitUI);

	m_resultUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 1.f, 1.f });
	auto resultUI{ make_shared<StandardUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.4f, 0.5f}) };
	resultUI->SetTexture("FRAMEUI");
	m_resultTextUI = make_shared<TextUI>(XMFLOAT2{ 0.f, 0.2f }, XMFLOAT2{ 100.f, 20.f });
	m_resultTextUI->SetText(L"클리어!");
	m_resultTextUI->SetColorBrush("WHITE");
	m_resultTextUI->SetTextFormat("MAPLE27");
	resultUI->SetChild(m_resultTextUI);
	auto resultRewardTextureUI{ make_shared<StandardUI>(XMFLOAT2{-0.2f, -0.2f}, XMFLOAT2{0.05f, 0.05f}) };
	resultRewardTextureUI->SetTexture("GOLDUI");
	resultUI->SetChild(resultRewardTextureUI);
	m_resultRewardTextUI = make_shared<TextUI>(XMFLOAT2{0.2f, -0.2f}, XMFLOAT2{100.f, 20.f});
	m_resultRewardTextUI->SetText(TEXT(""));
	m_resultRewardTextUI->SetColorBrush("WHITE");
	m_resultRewardTextUI->SetTextFormat("KOPUB24");
	resultUI->SetChild(m_resultRewardTextUI);
	auto resultButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.7f}, XMFLOAT2{0.15f, 0.075f}) };
	resultButtonUI->SetTexture("BUTTONUI");
	resultButtonUI->SetClickEvent([&]() {
		m_resultUI->SetDisable();
		ResetState(State::OutputResult);
		});
	auto ResultButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{20.f, 10.f}) };
	ResultButtonTextUI->SetText(L"닫기");
	ResultButtonTextUI->SetColorBrush("WHITE");
	ResultButtonTextUI->SetTextFormat("KOPUB18");
	resultButtonUI->SetChild(ResultButtonTextUI);
	resultUI->SetChild(resultButtonUI);

	m_resultUI->SetChild(resultUI);
	m_resultUI->SetDisable();
	m_globalShaders["POSTUI"]->SetUI(m_resultUI);
}

void TowerScene::BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	m_lightSystem = make_shared<LightSystem>();
	m_lightSystem->m_globalAmbient = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.f };
	m_lightSystem->m_numLight = (INT)LightTag::Count;

	m_directionalDiffuse = XMFLOAT4{ 0.4f, 0.4f, 0.4f, 1.f };
	m_directionalDirection = XMFLOAT3{ 1.f, -1.f, 1.f };

	m_lightSystem->m_lights[(INT)LightTag::Directional].m_type = DIRECTIONAL_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Directional].m_ambient = XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.f };
	m_lightSystem->m_lights[(INT)LightTag::Directional].m_diffuse = m_directionalDiffuse;
	m_lightSystem->m_lights[(INT)LightTag::Directional].m_specular = XMFLOAT4{ 0.2f, 0.2f, 0.2f, 0.f };
	m_lightSystem->m_lights[(INT)LightTag::Directional].m_direction = m_directionalDirection;
	m_lightSystem->m_lights[(INT)LightTag::Directional].m_enable = true;

	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_position = XMFLOAT3(-2.943f, -2.468f, -45.335f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Corridor1].m_enable = false;

	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_position = XMFLOAT3(-2.963f, -2.468f, -33.575f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Corridor2].m_enable = false;

	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_position = XMFLOAT3(-2.755f, -2.468f, -21.307f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Corridor3].m_enable = false;

	m_lightSystem->m_lights[(INT)LightTag::Room1].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_position = XMFLOAT3(-4.833f, 1.952f, 8.224f);
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Room1].m_enable = false;

	m_lightSystem->m_lights[(INT)LightTag::Room2].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_position = XMFLOAT3(4.796f, 1.952f, 8.044f);
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Room2].m_enable = false;

	m_lightSystem->m_lights[(INT)LightTag::Room3].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_position = XMFLOAT3(-4.433f, 1.952f, 39.324f);
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Room3].m_enable = false;

	m_lightSystem->m_lights[(INT)LightTag::Room4].m_type = SPOT_LIGHT;
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_range = 5.f;
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_diffuse = XMFLOAT4(0.64f, 0.64f, 0.44f, 1.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_position = XMFLOAT3(3.116f, 1.952f, 39.184f);
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_attenuation = XMFLOAT3(1.0f, 0.25f, 0.05f);
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_falloff = 3.f;
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_phi = (float)cos(XMConvertToRadians(120.f));
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_theta = (float)cos(XMConvertToRadians(60.f));
	m_lightSystem->m_lights[(INT)LightTag::Room4].m_enable = false;

	m_lightSystem->CreateShaderVariable(device, commandlist);
}

void TowerScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
	if (m_exitUI) m_exitUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	if (m_resultUI) m_resultUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);

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

void TowerScene::Update(FLOAT timeElapsed)
{
	if (CheckState(State::SceneLeave)) {
		g_GameFramework.ChangeScene(SCENETAG::VillageLoadingScene);
		closesocket(g_socket);
		return;
	}
	RecvPacket();

	m_camera->Update(timeElapsed);
	if (m_globalShaders["SKYBOX"]) for (auto& skybox : m_globalShaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_globalShaders)
		shader.second->Update(timeElapsed);
	g_particleSystem->Update(timeElapsed);
	m_towerObjectManager->Update(timeElapsed);
	m_fadeFilter->Update(timeElapsed);

	//CollideWithMap();
	//CollideWithObject();

	UpdateLightSystem(timeElapsed);

	if (CheckState(State::WarpGate)) {
		m_age += timeElapsed;
		if (m_age >= m_lifeTime) {
			m_age = 0.f;
			ResetState(State::WarpGate);
		}
		else {
			m_lightSystem->m_lights[(INT)LightTag::Directional].m_diffuse = 
				Vector4::Add(m_directionalDiffuse, Vector4::Mul({ -0.3f, -0.3f, -0.3f, 0.f}, m_age / m_lifeTime));
			m_lightSystem->m_lights[(INT)LightTag::Directional].m_direction =
				Vector3::Add(m_directionalDirection, Vector3::Mul({ 2.f, 0.f, 2.f }, m_age / m_lifeTime));
		}
	}
}

void TowerScene::UpdateLightSystem(FLOAT timeElapsed)
{
	const array<FLOAT, 3> corridorLight{ -45.335f, -33.575f, -21.307f };
	const array<INT, 3> corridorLightIndex{ 1, 2, 3 };

	for (const auto& index : corridorLightIndex) {
		[&]() {
			if (m_lightSystem) {
				if (!m_lightSystem->m_lights[index].m_enable) {
					if (m_player->GetPosition().z >= corridorLight[index - corridorLightIndex[0]]) {
						m_lightSystem->m_lights[index].m_enable = true;
						return;
					}

					for (const auto& player : m_multiPlayers) {
						if (player.second->GetPosition().z >= corridorLight[index - corridorLightIndex[0]]) {
							m_lightSystem->m_lights[index].m_enable = true;
							return;
						}
					}
				}
			}
		}();
	}
}

void TowerScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex)
{
	if (CheckState(State::SceneLeave)) return;
	switch (threadIndex)
	{
	case 0:
	{
		m_globalShaders["ANIMATION"]->Render(commandList, m_globalShaders["ANIMATIONSHADOW"]);
		break;
	}
	case 1:
	{
		m_globalShaders["OBJECT"]->Render(commandList, m_globalShaders["SHADOW"]);
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
	if (CheckState(State::SceneLeave)) return;
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);

	switch (threadIndex)
	{
	case 0:
	{
		m_globalShaders.at("OBJECT")->Render(commandList);
		m_towerObjectManager->Render(commandList);
		break;
	}
	case 1:
	{
		m_globalShaders.at("ANIMATION")->Render(commandList);
		break;
	}
	case 2:
	{
		m_globalShaders.at("SKYBOX")->Render(commandList);
		g_particleSystem->Render(commandList);
		m_globalShaders.at("HORZGAUGE")->Render(commandList);
		m_globalShaders.at("VERTGAUGE")->Render(commandList);
		m_globalShaders.at("UI")->Render(commandList);
		//m_globalShaders["DEBUG"]->Render(commandList);
		break;
	}
	}
}

void TowerScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
{
	if (CheckState(State::SceneLeave)) return;
	switch (threadIndex)
	{
	case 0:
	{
		if (CheckState(State::BlurLevel5)) {
			m_blurFilter->Execute(commandList, renderTarget, 5);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}
		else if (CheckState(State::BlurLevel4)) {
			m_blurFilter->Execute(commandList, renderTarget, 4);
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
			m_blurFilter->Execute(commandList, renderTarget, 2);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}
		else if (CheckState(State::BlurLevel1)) {
			m_blurFilter->Execute(commandList, renderTarget, 1);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_blurFilter->ResetResourceBarrier(commandList);
		}

		//m_sobelFilter->Execute(commandList, renderTarget);
		//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		//m_globalShaders["COMPOSITE"]->Render(commandList);
		//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
		break;
	}
	case 1:
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		m_globalShaders.at("POSTUI")->Render(commandList);
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
	if (m_interactUI) m_interactUI->RenderText(deviceContext);
	if (m_skillUI) m_skillUI->RenderText(deviceContext);
	if (m_ultimateUI) m_ultimateUI->RenderText(deviceContext);
}

void TowerScene::PostRenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
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

		m_globalShaders["OBJECT"]->SetObject(object);
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

	BoundingOrientedBox obb = BoundingOrientedBox{ player->GetPosition(),
		XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f}};
	player->SetBoundingBox(obb);

	player->SetAnimationSet(m_globalAnimationSets[animationSet], animationSet);
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

	monster->SetAnimationSet(m_animationSets[animationSet], animationSet);
	monster->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	monster->GetAnimationController()->SetTrackEnable(1, false);
	monster->GetAnimationController()->SetTrackEnable(2, false);
}

void TowerScene::SetHpBar(const shared_ptr<AnimationObject>& object)
{
	auto hpBar = make_shared<GaugeBar>(0.16f);
	hpBar->SetMesh("HPBAR");
	hpBar->SetTexture("HPBAR");
	hpBar->SetMaxGauge(object->GetMaxHp());
	hpBar->SetGauge(object->GetHp());
	hpBar->SetPosition(XMFLOAT3{ FAR_POSITION, FAR_POSITION, FAR_POSITION });
	m_globalShaders["HORZGAUGE"]->SetObject(hpBar);
	object->SetHpBar(hpBar);
}

void TowerScene::RotateToTarget(const shared_ptr<GameObject>& object, INT targetId)
{
	if (-1 == targetId) {
		return;
	}

	auto& monster = m_monsters[targetId];
	if (!monster)
		return;

	XMFLOAT3 dir = Vector3::Normalize(
		Vector3::Sub(monster->GetPosition(), object->GetPosition()));
	XMFLOAT3 front{ object->GetFront() };

	XMVECTOR radian{ XMVector3AngleBetweenNormals(XMLoadFloat3(&front), XMLoadFloat3(&dir))};
	FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };
	XMFLOAT3 clockwise{ Vector3::Cross(front, dir) };
	angle = clockwise.y < 0 ? -angle : angle;

	object->Rotate(0.f, 0.f, angle);
}

void TowerScene::RotateToTarget(const shared_ptr<GameObject>& object)
{
	INT target = SetTarget(object);
	RotateToTarget(object, target);
}

INT TowerScene::SetTarget(const shared_ptr<GameObject>& object)
{
	XMFLOAT3 sub{};
	float length{}, min_length{ std::numeric_limits<float>::max() };
	XMFLOAT3 pos{ object->GetPosition() };
	int target{ -1 };

	for (const auto& elm : m_monsters) {
		if (elm.second) {
			// 체력이 0이면(죽었다면) 건너뜀
			if (elm.second->GetHp() <= std::numeric_limits<float>::epsilon()) continue;

			// 추가는 되었으나 그려지고 있지 않다면 건너뜀
			if (!m_globalShaders["ANIMATION"]->GetMonsters().contains(elm.first)) continue;

			sub = Vector3::Sub(pos, elm.second->GetPosition());
			length = Vector3::Length(sub);
			if (length < min_length && length < PlayerSetting::AUTO_TARGET_RANGE + 2.f) {
				min_length = length;
				target = elm.first;
			}
		}
	}
	return target;
}

bool TowerScene::CheckState(State sceneState) const
{
	return m_sceneState & (INT)sceneState;
}

void TowerScene::SetState(State sceneState)
{
	m_sceneState |= (INT)sceneState;
}

void TowerScene::ResetState(State sceneState)
{
	m_sceneState &= ~(INT)sceneState;
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

	unsigned long noblock = 1;
	ioctlsocket(g_socket, FIONBIO, &noblock);

	constexpr char name[10] = "HSC\0";
	CS_LOGIN_PACKET login_packet{};
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	login_packet.player_type = m_player->GetType();
	memcpy(login_packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);

	//g_networkThread = thread{ &TowerScene::RecvPacket, this};
	//g_networkThread.detach();

#endif
}

void TowerScene::SendPlayerData()
{
#ifdef USE_NETWORK
	CS_PLAYER_MOVE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_PLAYER_MOVE;
	packet.pos = m_player->GetPosition();
	packet.yaw = m_player->GetYaw();
	send(g_socket, reinterpret_cast<char*>(&packet), packet.size, 0);
#endif
}

void TowerScene::RecvPacket()
{
	char buf[BUF_SIZE] = { 0 };
	WSABUF wsabuf{ BUF_SIZE, buf };
	DWORD recv_byte{ 0 }, recv_flag{ 0 };

	int retval = WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr);
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
		ErrorDisplay("RecvSizeType");
	}

	if (recv_byte > 0) {
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
	case SC_PACKET_WARP_NEXT_FLOOR:
		RecvWarpNextFloor(ptr);
		break;
	case SC_PACKET_PLAYER_DEATH:
		RecvPlayerDeath(ptr);
		break;
	case SC_PACKET_ARROW_SHOOT:
		RecvArrowShoot(ptr);
		break;
	case SC_PACKET_REMOVE_ARROW:
		RecvRemoveArrow(ptr);
		break;
	case SC_PACKET_INTERACT_OBJECT:
		RecvInteractObject(ptr);
		break;
	case SC_PACKET_CHANGE_HP:
		RecvChangeHp(ptr);
		break;
	case SC_PACKET_ADD_TRIGGER:
		RecvAddTrigger(ptr);
		break;
	case SC_PACKET_ADD_MAGIC_CIRCLE:
		RecvAddMagicCircle(ptr);
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
//	m_player->SetPosition(packet->player_data.pos);
	m_player->SetHp(packet->player_data.hp);
}

void TowerScene::RecvAddObject(char* ptr)
{
	SC_ADD_OBJECT_PACKET* packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);

	INT id = packet->player_data.id;

	auto multiPlayer = make_shared<Player>();
	multiPlayer->SetType(packet->player_type);
	LoadPlayerFromFile(multiPlayer);

	// 멀티플레이어 설정
	multiPlayer->SetPosition(packet->player_data.pos);
	multiPlayer->SetHp(packet->player_data.hp);

	m_multiPlayers.insert({ id, multiPlayer });

	SetHpBar(multiPlayer);

	m_idSet.insert({ id, (INT)m_idSet.size() });
	m_hpUI[m_idSet[id]]->SetEnable();

	m_globalShaders["ANIMATION"]->SetMultiPlayer(id, multiPlayer);
	cout << "add player" << id << endl;
}

void TowerScene::RecvRemovePlayer(char* ptr)
{
	SC_REMOVE_PLAYER_PACKET* packet = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(ptr);

	m_multiPlayers[packet->id]->SetPosition(XMFLOAT3(FAR_POSITION, FAR_POSITION, FAR_POSITION));
	m_multiPlayers[packet->id]->SetId(-1);

	m_hpUI[m_idSet[packet->id]]->SetDisable();
}

void TowerScene::RecvRemoveMonster(char* ptr)
{
	SC_REMOVE_MONSTER_PACKET* packet = reinterpret_cast<SC_REMOVE_MONSTER_PACKET*>(ptr);
	m_monsters[packet->id]->SetPosition(XMFLOAT3(FAR_POSITION, FAR_POSITION, FAR_POSITION));

	// 임시 삭제
	m_globalShaders["ANIMATION"]->RemoveMonster(packet->id);
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
		m_player->SetHp(packet->data.hp);
	}
	else {
		auto& player = m_multiPlayers[packet->data.id];

		player->SetPosition(packet->data.pos);
		//player->SetVelocity(packet->data.velocity);
		player->Rotate(0.f, 0.f, packet->data.yaw - player->GetYaw());
		player->SetHp(packet->data.hp);
		m_hpUI[m_idSet[packet->data.id]]->SetGauge(packet->data.hp);
	}
}

void TowerScene::RecvAddMonster(char* ptr)
{
	SC_ADD_MONSTER_PACKET* packet = reinterpret_cast<SC_ADD_MONSTER_PACKET*>(ptr);

	if (packet->monster_data.id == -1)
		return;

	auto monster = make_shared<Monster>();

	monster->SetType(packet->monster_type);
	LoadMonsterFromFile(monster);
	monster->ChangeAnimation(ObjectAnimation::WALK, false);

	monster->SetMaxHp(packet->monster_data.hp);
	monster->SetHp(packet->monster_data.hp);
	monster->SetPosition(XMFLOAT3{ packet->monster_data.pos.x,
		packet->monster_data.pos.y, packet->monster_data.pos.z });
	m_monsters.insert({ static_cast<INT>(packet->monster_data.id), monster });

	SetHpBar(monster);
}

void TowerScene::RecvUpdateMonster(char* ptr)
{
	SC_UPDATE_MONSTER_PACKET* packet = reinterpret_cast<SC_UPDATE_MONSTER_PACKET*>(ptr);

	if (packet->monster_data.id < 0) return;

	if (!m_monsters.contains(packet->monster_data.id))
		return;
	auto& monster = m_monsters[packet->monster_data.id];

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

	if (!m_monsters.contains(packet->id))
		return;
	auto& monster = m_monsters[packet->id];

	if (ObjectAnimation::DEATH == monster->GetCurrentAnimation())
		return;

	monster->ChangeAnimation(packet->animation, false);
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
	m_resultRewardTextUI->SetText(to_wstring(packet->reward));
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
			m_hpUI[m_idSet[id]]->SetGauge(packet->hps[i]);
		}
		++i;
	}
}

void TowerScene::RecvSetInteractable(char* ptr)
{
	SC_SET_INTERACTABLE_PACKET* packet = reinterpret_cast<SC_SET_INTERACTABLE_PACKET*>(ptr);

	m_player->SetInteractable(packet->interactable);
	m_player->SetInteractableType(packet->interactable_type);

	if (packet->interactable) {
		m_interactUI->SetEnable();
	}
	else {
		m_interactUI->SetDisable();
	}

	switch (packet->interactable_type)
	{
	case BATTLE_STARTER:
		m_interactTextUI->SetText(TEXT("F : 전투 시작"));
		break;
	case PORTAL:
		m_interactTextUI->SetText(TEXT("F : 다음 층 이동"));
		break;
	}
}

void TowerScene::RecvWarpNextFloor(char* ptr)
{
	SC_WARP_NEXT_FLOOR_PACKET* packet = reinterpret_cast<SC_WARP_NEXT_FLOOR_PACKET*>(ptr);
	
	m_interactUI->SetDisable();
	SetState(State::Fading);
	m_fadeFilter->FadeOut([&]() {
		m_player->ChangeAnimation(ObjectAnimation::IDLE, false);
		m_player->SetPosition(RoomSetting::START_POSITION);
		m_player->SetHp(m_player->GetMaxHp());

		for (auto& elm : m_multiPlayers) {
			if (-1 == elm.second->GetId()) continue;

			elm.second->SetPosition(RoomSetting::START_POSITION);
			elm.second->ChangeAnimation(ObjectAnimation::IDLE, false);
			elm.second->SetHp(m_player->GetMaxHp());
		}

		m_globalShaders["OBJECT"]->SetObject(m_gate);

		for (int i = 1; i < 8; ++i) {
			m_lightSystem->m_lights[i].m_enable = false;
		}
		m_lightSystem->m_lights[0].m_diffuse = m_directionalDiffuse;
		m_lightSystem->m_lights[0].m_direction = m_directionalDirection;

		m_player->ResetAllCooldown();

		m_fadeFilter->FadeIn([&](){ ResetState(State::Fading); });
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

void TowerScene::RecvArrowShoot(char* ptr)
{
	SC_ARROW_SHOOT_PACKET* packet = reinterpret_cast<SC_ARROW_SHOOT_PACKET*>(ptr);

	if (0 <= packet->id && packet->id < MAX_USER) {
		if (packet->id == m_player->GetId()) {
			m_towerObjectManager->CreateArrow(m_player, packet->arrow_id, PlayerSetting::ARROW_SPEED,
				packet->action_type);
		}
		else {
			auto& player = m_multiPlayers[packet->id];
			m_towerObjectManager->CreateArrow(player, packet->arrow_id, PlayerSetting::ARROW_SPEED,
				packet->action_type);
		}
	}
	else {
		m_towerObjectManager->CreateArrow(m_monsters[packet->id], packet->arrow_id, MonsterSetting::ARROW_SPEED,
			packet->action_type);
	}
}

void TowerScene::RecvRemoveArrow(char* ptr)
{
	SC_REMOVE_ARROW_PACKET* packet = reinterpret_cast<SC_REMOVE_ARROW_PACKET*>(ptr);
	m_towerObjectManager->RemoveArrow(packet->arrow_id);
}

void TowerScene::RecvInteractObject(char* ptr)
{
	SC_INTERACT_OBJECT_PACKET* packet = reinterpret_cast<SC_INTERACT_OBJECT_PACKET*>(ptr);
	m_player->SetInteractable(false);
	m_player->SetInteractableType(InteractionType::NONE);

	switch (packet->interaction_type) {
	case InteractionType::BATTLE_STARTER:
		SetState(State::WarpGate);

		m_interactUI->SetDisable();
		m_gate->SetInterect([&]() {
			m_globalShaders["OBJECT"]->RemoveObject(m_gate);
		m_lightSystem->m_lights[4].m_enable = true;
		m_lightSystem->m_lights[5].m_enable = true;
		m_lightSystem->m_lights[6].m_enable = true;
		m_lightSystem->m_lights[7].m_enable = true;

		for (auto& monster : m_monsters) {
			m_globalShaders["ANIMATION"]->SetMonster(monster.first, monster.second);
		}

			});

		break;
	}
}

void TowerScene::RecvChangeHp(char* ptr)
{
	SC_CHANGE_HP_PACKET* packet = reinterpret_cast<SC_CHANGE_HP_PACKET*>(ptr);

	if (0 <= packet->id && 0 < MAX_USER) {
		if (m_player->GetId() == packet->id) {
			m_player->SetHp(packet->hp);
		}
		else {
			m_multiPlayers[packet->id]->SetHp(packet->hp);
		}
	}
	else {
		m_monsters[packet->id]->SetHp(packet->hp);
	}
}

void TowerScene::RecvAddTrigger(char* ptr)
{
	SC_ADD_TRIGGER_PACKET* packet = reinterpret_cast<SC_ADD_TRIGGER_PACKET*>(ptr);

	switch (packet->trigger_type) {
	case TriggerType::ARROW_RAIN:
		m_towerObjectManager->CreateArrowRain(packet->pos);
		break;
	case TriggerType::UNDEAD_GRASP:
		cout << "UNDEAD_GRASP 생성\n";
		break;
	}
}

void TowerScene::RecvAddMagicCircle(char* ptr)
{
	SC_ADD_MAGIC_CIRCLE_PACKET* packet = reinterpret_cast<SC_ADD_MAGIC_CIRCLE_PACKET*>(ptr);
	m_towerObjectManager->CreateMonsterMagicCircle(packet->pos);
}

void TowerScene::LoadMap()
{
	std::unordered_map<std::string, BoundingOrientedBox> bounding_box_data;

	std::ifstream in{ "./Resource/GameRoom/GameRoomObject.bin", std::ios::binary };

	BYTE strLength{};
	std::string objectName;
	BoundingOrientedBox boundingBox{};

	while (in.read((char*)(&strLength), sizeof(BYTE))) {
		objectName.resize(strLength, '\0');
		in.read((char*)(&objectName[0]), strLength);

		in.read((char*)(&boundingBox.Extents), sizeof(XMFLOAT3));

		bounding_box_data.insert({ objectName, boundingBox });
	}
	in.close();


	in.open("./Resource/GameRoom/GameRoomMap.bin", std::ios::binary);

	XMFLOAT3 position{};
	FLOAT yaw{};

	while (in.read((char*)(&strLength), sizeof(BYTE))) {
		objectName.resize(strLength, '\0');
		in.read(&objectName[0], strLength);

		in.read((char*)(&position), sizeof(XMFLOAT3));
		in.read((char*)(&yaw), sizeof(FLOAT));
		// 라디안 값

		if (!bounding_box_data.contains(objectName))
			continue;

		auto object = std::make_shared<GameObject>();
		object->SetPosition(position);
		//object->SetYaw(XMConvertToDegrees(yaw));
		object->Rotate(0, 0, yaw);

		XMVECTOR vec = XMQuaternionRotationRollPitchYaw(0.f, yaw, 0.f);
		XMFLOAT4 q{};
		XMStoreFloat4(&q, vec);

		boundingBox = bounding_box_data[objectName];
		boundingBox.Center = position;
		boundingBox.Orientation = XMFLOAT4{ q.x, q.y, q.z, q.w };

		object->SetBoundingBox(boundingBox);

		if (objectName == "Invisible_Wall")
			m_invisibleWalls.push_back(object);
		else
			m_structures.push_back(object);
	}
}

void TowerScene::CollideWithMap()
{
	auto& player_obb = m_player->GetBoundingBox();

	for (const auto& obj : m_structures) {
		auto& obb = obj->GetBoundingBox();
		if (player_obb.Intersects(obb)) {
			CollideByStaticOBB(m_player, obj);
		}
	}

	auto& v = m_globalShaders["OBJECT"]->GetObjects();
	if (!m_monsters.empty() &&
		find(v.begin(), v.end(), m_gate) == v.end())
	{
		for (const auto& obj : m_invisibleWalls) {
			auto& obb = obj->GetBoundingBox();
			if (player_obb.Intersects(obb)) {
				CollideByStaticOBB(m_player, obj);
			}
		}
	}
}

void TowerScene::CollideWithObject()
{
	auto& player_obb = m_player->GetBoundingBox();

	for (const auto& elm : m_monsters) {
		if (elm.second) {
			if (player_obb.Intersects(elm.second->GetBoundingBox())) {
				CollideByStatic(m_player, elm.second);
			}
		}
	}

	for (const auto& elm : m_multiPlayers) {
		if (elm.second) {
			if (player_obb.Intersects(elm.second->GetBoundingBox())) {
				CollideByStatic(m_player, elm.second);
			}
		}
	}
}

void TowerScene::CollideByStatic(const shared_ptr<GameObject>& obj, const shared_ptr<GameObject>& static_obj)
{
	BoundingOrientedBox& static_obb = static_obj->GetBoundingBox();
	BoundingOrientedBox& obb = obj->GetBoundingBox();

	FLOAT obb_left = obb.Center.x - obb.Extents.x;
	FLOAT obb_right = obb.Center.x + obb.Extents.x;
	FLOAT obb_front = obb.Center.z + obb.Extents.z;
	FLOAT obb_back = obb.Center.z - obb.Extents.z;

	FLOAT static_obb_left = static_obb.Center.x - static_obb.Extents.x;
	FLOAT static_obb_right = static_obb.Center.x + static_obb.Extents.x;
	FLOAT static_obb_front = static_obb.Center.z + static_obb.Extents.z;
	FLOAT static_obb_back = static_obb.Center.z - static_obb.Extents.z;

	FLOAT x_bias{}, z_bias{};
	bool push_out_x_plus{ false }, push_out_z_plus{ false };

	// 충돌한 물체의 중심이 x가 더 크면
	if (obb.Center.x - static_obb.Center.x <= std::numeric_limits<FLOAT>::epsilon()) {
		x_bias = obb_right - static_obb_left;
	}
	else {
		x_bias = static_obb_right - obb_left;
		push_out_x_plus = true;
	}

	// 충돌한 물체의 중심이 z가 더 크면
	if (obb.Center.z - static_obb.Center.z <= std::numeric_limits<FLOAT>::epsilon()) {
		z_bias = obb_front - static_obb_back;
	}
	else {
		z_bias = static_obb_front - obb_back;
		push_out_z_plus = true;
	}

	XMFLOAT3 pos = obj->GetPosition();

	// z 방향으로 밀어내기
	if (x_bias - z_bias >= std::numeric_limits<FLOAT>::epsilon()) {
		// object가 +z 방향으로
		if (push_out_z_plus) {
			pos.z += z_bias;
		}
		// object가 -z 방향으로
		else {
			pos.z -= z_bias;
		}
	}

	// x 방향으로 밀어내기
	else {
		// object가 +x 방향으로
		if (push_out_x_plus) {
			pos.x += x_bias;
		}
		// object가 -x 방향으로
		else {
			pos.x -= x_bias;
		}
	}

	obj->SetPosition(pos);
	obb.Center = pos;
}

void TowerScene::CollideByStaticOBB(const shared_ptr<GameObject>& obj, const shared_ptr<GameObject>& static_obj)
{
	XMFLOAT3 corners[8]{};

	BoundingOrientedBox static_obb = static_obj->GetBoundingBox();
	static_obb.Center.y = 0.f;
	static_obb.GetCorners(corners);

	// 꼭짓점 시계방향 0,1,5,4
	XMFLOAT3 o_square[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z} ,
		{corners[5].x, 0.f, corners[5].z} ,
		{corners[4].x, 0.f, corners[4].z} };

	BoundingOrientedBox object_obb = obj->GetBoundingBox();
	object_obb.Center.y = 0.f;
	object_obb.GetCorners(corners);

	XMFLOAT3 p_square[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z} ,
		{corners[5].x, 0.f, corners[5].z} ,
		{corners[4].x, 0.f, corners[4].z} };



	for (const XMFLOAT3& point : p_square) {
		if (!static_obb.Contains(XMLoadFloat3(&point))) continue;

		std::array<float, 4> dist{};
		dist[0] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[0]), XMLoadFloat3(&o_square[1]), XMLoadFloat3(&point)));
		dist[1] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[1]), XMLoadFloat3(&o_square[2]), XMLoadFloat3(&point)));
		dist[2] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[2]), XMLoadFloat3(&o_square[3]), XMLoadFloat3(&point)));
		dist[3] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&o_square[3]), XMLoadFloat3(&o_square[0]), XMLoadFloat3(&point)));

		auto min = min_element(dist.begin(), dist.end());

		XMFLOAT3 v{};
		if (*min == dist[0])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[1], o_square[2]));
		}
		else if (*min == dist[1])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[1], o_square[0]));
		}
		else if (*min == dist[2])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[2], o_square[1]));
		}
		else if (*min == dist[3])
		{
			v = Vector3::Normalize(Vector3::Sub(o_square[0], o_square[1]));
		}
		v = Vector3::Mul(v, *min);
		obj->SetPosition(Vector3::Add(obj->GetPosition(), v));
		break;
	}

}