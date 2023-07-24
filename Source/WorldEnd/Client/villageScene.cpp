#include "villageScene.h"

VillageScene::VillageScene(const ComPtr<ID3D12Device>& device, 
	const ComPtr<ID3D12GraphicsCommandList>& commandList, 
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature) :
	m_NDCspace(0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f),
	m_sceneState{ (INT)State::Unused },
	m_roomPage{0}
{
	OnCreate(device, commandList, rootSignature, postRootSignature);
}

VillageScene::~VillageScene()
{
	//OnDestroy();
}

void VillageScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
	if (m_blurFilter) m_blurFilter->OnResize(device, width, height);
	if (m_fadeFilter) m_fadeFilter->OnResize(device, width, height);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 100.0f));
	if (m_camera) m_camera->SetProjMatrix(projMatrix);
}

void VillageScene::OnCreate(
	const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature,
	const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	m_sceneState = (INT)State::Unused;
	BuildObjects(device, commandList, rootSignature, postRootSignature);
}

void VillageScene::OnDestroy()
{
	for (auto& shader : m_shaders) shader.second->Clear();

	DestroyObjects();
}

void VillageScene::ReleaseUploadBuffer() {}

void VillageScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) 
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
void VillageScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) 
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

void VillageScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist,
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	CreateShaderVariable(device, commandlist);

	// 플레이어 생성
	m_player = make_shared<Player>();

	// DB 에서 받아온 플레이어 정보 입력
	m_player->SetType(g_playerInfo.playerType);
	m_player->SetId(g_playerInfo.id);
#ifdef USE_NETWORK
	//m_player->SetPosition(g_playerInfo.position);
	m_player->SetPosition(XMFLOAT3{ -35.f, 5.65f, 66.f });
#else
	m_player->SetPosition(XMFLOAT3{ 25.f, 5.65f, 66.f });
#endif

	cout << "스킬 타입 : " << (INT)g_playerInfo.skill[(INT)g_playerInfo.playerType].first << endl;
	cout << "궁극기 타입 : " << (INT)g_playerInfo.skill[(INT)g_playerInfo.playerType].second << endl;


	LoadPlayerFromFile(m_player);
	m_shaders["ANIMATION"]->SetPlayer(m_player);

	// 카메라 생성
	m_camera = make_shared<ThirdPersonCamera>();
	m_camera->CreateShaderVariable(device, commandlist);
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 300.0f));
	m_camera->SetProjMatrix(projMatrix);
	m_shaders["OBJECTBLEND"]->SetCamera(m_camera);

	m_shadow = make_shared<Shadow>(device, 1024, 1024);

	// 쿼드트리 생성
	m_quadtree = make_unique<QuadtreeFrustum>(XMFLOAT3{ -100.f, 0, 100 }, XMFLOAT3{ 200.f, 50.f, 200.f }, 4);

	// 씬 로드
	LoadSceneFromFile(TEXT("Resource/Scene/VillageScene.bin"), TEXT("VillageScene"));

	// 터레인 로드
	m_terrain = make_shared<HeightMapTerrain>(device, commandlist,
		TEXT("Resource/HeightMap/Terrain.raw"), 1025, 1025, 1025, 1025, XMFLOAT3{ 1.f, 1.f, 1.f });
	m_terrain->Rotate(0.f, 0.f, 180.f);
	m_terrain->SetPosition(XMFLOAT3{ 418.3, -2.11f, 697.f });
	m_terrain->SetTexture("TERRAIN");
	m_shaders["TERRAIN"]->SetObject(m_terrain);

	// 스카이 박스 생성
	auto skybox{ make_shared<GameObject>() };
	skybox->SetMesh("SKYBOX");
	skybox->SetTexture("VILLAGESKYBOX");
	m_shaders["SKYBOX"]->SetObject(skybox);

	// NPC 생성
	auto skillNpc = make_shared<Player>();
	LoadPlayerFromFile(skillNpc);
	skillNpc->SetType(PlayerType::WARRIOR);
	skillNpc->SetPosition(VillageSetting::SKILL_NPC);
	skillNpc->Rotate(0.f, 0.f, 180.f);
	m_shaders["ANIMATION"]->SetObject(skillNpc);

	auto inhenceNpc = make_shared<Player>();
	LoadPlayerFromFile(inhenceNpc);
	inhenceNpc->SetType(PlayerType::WARRIOR);
	inhenceNpc->SetPosition(VillageSetting::INHENCE_NPC);
	inhenceNpc->Rotate(0.f, 0.f, 180.f);
	m_shaders["ANIMATION"]->SetObject(inhenceNpc);

	BuildUI();

	// 필터 생성
	auto windowWidth = g_GameFramework.GetWindowWidth();
	auto windowHeight = g_GameFramework.GetWindowHeight();
	m_blurFilter = make_unique<BlurFilter>(device, windowWidth, windowHeight);
	m_fadeFilter = make_unique<FadeFilter>(device, windowWidth, windowHeight);

	// 조명 생성
	BuildLight(device, commandlist);
}

void VillageScene::BuildUI()
{
	BuildInteractUI();
	BulidRoomUI();
	BuildPartyUI();
	BuildSkillSettingUI();
	BuildInhenceUI();
	BuildMainUI();
}

void VillageScene::BuildInteractUI()
{
	m_interactUI = make_shared<ImageUI>(XMFLOAT2{ 0.25f, 0.15f }, XMFLOAT2{ 0.29f, 0.1f });
	m_interactUI->SetTexture("BUTTONUI");
	m_interactTextUI = make_shared<TextUI>(XMFLOAT2{ 0.f, -0.2f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 80.f, 20.f });
	m_interactTextUI->SetColorBrush("WHITE");
	m_interactTextUI->SetTextFormat("KOPUB18");
	m_interactUI->SetChild(m_interactTextUI);
	m_interactUI->SetDisable();
	m_shaders["UI"]->SetUI(m_interactUI);
}

void VillageScene::BulidRoomUI()
{
	m_roomUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.8f, 0.8f });
	m_roomUI->SetTexture("ROOMUI");

	auto roomCancelButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.95f, 0.9f}, XMFLOAT2{0.06f, 0.06f}) };
	roomCancelButtonUI->SetTexture("CANCELUI");
	roomCancelButtonUI->SetClickEvent([&]() {
		SendClosePartyUI();							// 파티창 닫았음을 전송
		ResetState(State::OutputRoomUI);
		if (m_roomUI) m_roomUI->SetDisable();
		m_roomPage = 0;
		});
	m_roomUI->SetChild(roomCancelButtonUI);

	auto createRoomButtonUI = make_shared<ButtonUI>(XMFLOAT2{ -0.3f, -0.75f }, XMFLOAT2{ 0.29f, 0.1f });
	createRoomButtonUI->SetTexture("BUTTONUI");
	createRoomButtonUI->SetClickEvent([&]() {
		SendCreateParty();	// 생성 전송
		});
	auto createRoomButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f},XMFLOAT2{120.f, 10.f}) };
	createRoomButtonTextUI->SetText(L"파티 생성");
	createRoomButtonTextUI->SetColorBrush("WHITE");
	createRoomButtonTextUI->SetTextFormat("KOPUB21");
	createRoomButtonUI->SetChild(createRoomButtonTextUI);
	m_roomUI->SetChild(createRoomButtonUI);

	auto joinRoomButtonUI = make_shared<ButtonUI>(XMFLOAT2{ 0.3f, -0.75f }, XMFLOAT2{ 0.29f, 0.1f });
	joinRoomButtonUI->SetTexture("BUTTONUI");
	joinRoomButtonUI->SetClickEvent([&]() {
		SendJoinParty();		// 방 참가 전송
		});
	auto joinRoomButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f},XMFLOAT2{120.f, 10.f}) };
	joinRoomButtonTextUI->SetText(L"파티 가입");
	joinRoomButtonTextUI->SetColorBrush("WHITE");
	joinRoomButtonTextUI->SetTextFormat("KOPUB21");
	joinRoomButtonUI->SetChild(joinRoomButtonTextUI);
	m_roomUI->SetChild(joinRoomButtonUI);

	m_leftArrowUI = make_shared<ButtonUI>(XMFLOAT2{ -0.8f, 0.f }, XMFLOAT2{ 0.1f, 0.2f });
	m_leftArrowUI->SetTexture("LEFTARROWUI");
	m_leftArrowUI->SetClickEvent([&]() {
		if (m_roomPage != 0) {
			m_roomPage -= 1;

			SendChangePage();		// 페이지 변경 전송
		}
		});
	m_roomUI->SetChild(m_leftArrowUI);

	m_rightArrowUI = make_shared<ButtonUI>(XMFLOAT2{ 0.8f, 0.f }, XMFLOAT2{ 0.1f, 0.2f });
	m_rightArrowUI->SetTexture("RIGHTARROWUI");
	m_rightArrowUI->SetClickEvent([&]() {
		m_roomPage += 1;

		SendChangePage();		// 페이지 변경 전송
		});
	m_roomUI->SetChild(m_rightArrowUI);

	for (size_t i = 0; auto & roomSwitchUI : m_roomSwitchUI) {
		roomSwitchUI = make_shared<SwitchUI>(XMFLOAT2{ 0.f, -0.4f + i * 0.2f }, XMFLOAT2{ 0.6f, 0.08f });
		roomSwitchUI->SetTexture("TEXTBARUI");

		// i번 방의 정보를 갱신하는 패킷이 날아왔을 때 m_roomSwitchTextUI[i]의 Text를 갱신해 주자.
		m_roomSwitchTextUI[i] = make_shared<TextUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 300.f, 20.f });
		m_roomSwitchTextUI[i]->SetText(TEXT("Empty"));
		m_roomSwitchTextUI[i]->SetColorBrush("WHITE");
		m_roomSwitchTextUI[i]->SetTextFormat("KOPUB21");

		roomSwitchUI->SetChild(m_roomSwitchTextUI[i]);
		m_roomUI->SetChild(roomSwitchUI);
		++i;
	}

	m_roomUI->SetDisable();
	m_shaders["UI"]->SetUI(m_roomUI);
}

void VillageScene::BuildPartyUI()
{
	m_partyUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.8f, 0.8f });
	m_partyUI->SetTexture("ROOMUI");

	auto partyCancelButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.95f, 0.9f}, XMFLOAT2{0.06f, 0.06f}) };
	partyCancelButtonUI->SetTexture("CANCELUI");
	partyCancelButtonUI->SetClickEvent([&]() {
		ResetState(State::OutputPartyUI);
		if (m_partyUI) m_partyUI->SetDisable();
		cout << "파티 UI 닫았다는 패킷 전송" << endl;

		});
	m_partyUI->SetChild(partyCancelButtonUI);

	auto enterDungeonButtonUI = make_shared<ButtonUI>(XMFLOAT2{ 0.f, -0.75f }, XMFLOAT2{ 0.29f, 0.1f });
	enterDungeonButtonUI->SetTexture("BUTTONUI");
	enterDungeonButtonUI->SetClickEvent([&]() {
		ResetState(State::OutputPartyUI);
		if (m_partyUI) m_partyUI->SetDisable();
		cout << "던전 입장 패킷 전송" << endl;
		cout << "파티원이 다같이 입장해야 함" << endl;
		});
	auto enterDungeonButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f},XMFLOAT2{120.f, 10.f}) };
	enterDungeonButtonTextUI->SetText(L"던전 입장");
	enterDungeonButtonTextUI->SetColorBrush("WHITE");
	enterDungeonButtonTextUI->SetTextFormat("KOPUB21");
	enterDungeonButtonUI->SetChild(enterDungeonButtonTextUI);
	m_partyUI->SetChild(enterDungeonButtonUI);

	for (int i = 0; i < 3; ++i) {
		auto characterFrameUI{ make_shared<ImageUI>(XMFLOAT2{ -0.6f + i * 0.6f, 0.1f }, XMFLOAT2{ 0.3f, 0.51f }) };
		characterFrameUI->SetTexture("CHARACTERFRAMEUI");

		m_partyPlayerUI[i] = make_shared<ImageUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.3f, 0.51f });
		characterFrameUI->SetChild(m_partyPlayerUI[i]);

		m_partyPlayerTextUI[i] = make_shared<TextUI>(XMFLOAT2{ 0.f, -0.8f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 300.f, 20.f });
		m_partyPlayerTextUI[i]->SetText(TEXT("Empty"));
		m_partyPlayerTextUI[i]->SetColorBrush("WHITE");
		m_partyPlayerTextUI[i]->SetTextFormat("KOPUB21");
		characterFrameUI->SetChild(m_partyPlayerTextUI[i]);

		m_partyUI->SetChild(characterFrameUI);
	}

	m_partyUI->SetDisable();
	m_shaders["UI"]->SetUI(m_partyUI);
}

void VillageScene::BuildSkillSettingUI()
{
	m_skillSettingUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.6f, 0.6f });
	m_skillSettingUI->SetTexture("ROOMUI");

	auto skillCancelButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.95f, 0.9f}, XMFLOAT2{0.06f, 0.06f}) };
	skillCancelButtonUI->SetTexture("CANCELUI");
	skillCancelButtonUI->SetClickEvent([&]() {
		ResetState(State::OutputSkillUI);
		if (m_skillSettingUI) m_skillSettingUI->SetDisable();
		if (m_skill1SwitchUI) m_skill1SwitchUI->SetNoActive();
		if (m_skill2SwitchUI) m_skill1SwitchUI->SetNoActive();
		if (m_ultimate1SwitchUI) m_ultimate1SwitchUI->SetNoActive();
		if (m_ultimate2SwitchUI) m_ultimate2SwitchUI->SetNoActive();
		m_skillNameUI->SetText(TEXT(""));
		m_skillInfoUI->SetText(TEXT(""));
		});
	m_skillSettingUI->SetChild(skillCancelButtonUI);

	m_skill1SwitchUI = make_shared<SwitchUI>(XMFLOAT2{ -0.65f, 0.4f }, XMFLOAT2{ 0.1f, 0.1f });
	m_skillSettingUI->SetChild(m_skill1SwitchUI);
	m_skill2SwitchUI = make_shared<SwitchUI>(XMFLOAT2{ -0.4f, 0.4f }, XMFLOAT2{ 0.1f, 0.1f });
	m_skillSettingUI->SetChild(m_skill2SwitchUI);
	m_ultimate1SwitchUI = make_shared<SwitchUI>(XMFLOAT2{ -0.65f, 0.f }, XMFLOAT2{ 0.1f, 0.1f });
	m_skillSettingUI->SetChild(m_ultimate1SwitchUI);
	m_ultimate2SwitchUI = make_shared<SwitchUI>(XMFLOAT2{ -0.4f, 0.f }, XMFLOAT2{ 0.1f, 0.1f });
	m_skillSettingUI->SetChild(m_ultimate2SwitchUI);
	m_skillNameUI = make_shared<TextUI>(XMFLOAT2{ 0.3f, 0.4f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 300.f, 20.f });
	m_skillNameUI->SetText(TEXT(""));
	m_skillNameUI->SetColorBrush("WHITE");
	m_skillNameUI->SetTextFormat("KOPUB24");
	m_skillSettingUI->SetChild(m_skillNameUI);
	m_skillInfoUI = make_shared<TextUI>(XMFLOAT2{ 0.3f, -0.4f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 500.f, 200.f });
	m_skillInfoUI->SetText(TEXT(""));
	m_skillInfoUI->SetColorBrush("WHITE");
	m_skillInfoUI->SetTextFormat("KOPUB21");
	m_skillSettingUI->SetChild(m_skillInfoUI);

	auto skillChangeButtonUI = make_shared<ButtonUI>(XMFLOAT2{ 0.f, -0.7f }, XMFLOAT2{ 0.29f, 0.1f });
	skillChangeButtonUI->SetTexture("BUTTONUI");
	skillChangeButtonUI->SetClickEvent([&]() {
		// 스킬을 변경했다는 패킷 보냄..? 필요한가?
		// 스킬 바꿀때마다 골드 깔거면 보내야 할 듯
		if (m_skill1SwitchUI->IsActive()) {
			g_playerInfo.skill[(size_t)g_playerInfo.playerType].first = 0;
		}
		if (m_skill2SwitchUI->IsActive()) {
			g_playerInfo.skill[(size_t)g_playerInfo.playerType].first = 1;
		}
		if (m_ultimate1SwitchUI->IsActive()) {
			g_playerInfo.skill[(size_t)g_playerInfo.playerType].second = 0;
		}
		if (m_ultimate2SwitchUI->IsActive()) {
			g_playerInfo.skill[(size_t)g_playerInfo.playerType].second = 1;
		}
		SetSkillUI();
		});
	auto skillChangeButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f},XMFLOAT2{120.f, 10.f}) };
	skillChangeButtonTextUI->SetText(L"스킬 변경");
	skillChangeButtonTextUI->SetColorBrush("WHITE");
	skillChangeButtonTextUI->SetTextFormat("KOPUB21");
	skillChangeButtonUI->SetChild(skillChangeButtonTextUI);
	m_skillSettingUI->SetChild(skillChangeButtonUI);

	SetSkillSettingUI(g_playerInfo.playerType);

	m_skillSettingUI->SetDisable();
	m_shaders["UI"]->SetUI(m_skillSettingUI);
}

inline void VillageScene::BuildInhenceUI()
{
	m_inhenceUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.6f, 0.6f });
	m_inhenceUI->SetTexture("ROOMUI");

	auto inhenceCancelButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.95f, 0.9f}, XMFLOAT2{0.06f, 0.06f}) };
	inhenceCancelButtonUI->SetTexture("CANCELUI");
	inhenceCancelButtonUI->SetClickEvent([&]() {
		ResetState(State::OutputInhenceUI);
		if (m_inhenceUI) m_inhenceUI->SetDisable();
		});
	m_inhenceUI->SetChild(inhenceCancelButtonUI);

	auto inhenceAttackUI{ make_shared<UI>(XMFLOAT2{-0.7f, 0.5f}, XMFLOAT2{0.2f, 0.2f}) };
	m_inhenceAttackSwitchUI = make_shared<SwitchUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.1f, 0.1f});
	m_inhenceAttackSwitchUI->SetTexture("INHENCEATTACK");
	inhenceAttackUI->SetChild(m_inhenceAttackSwitchUI);
	auto inhenceAttackTextUI{ make_shared<TextUI>(XMFLOAT2{ 0.f, -0.5f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 100.f, 20.f }) };
	inhenceAttackTextUI->SetText(TEXT("공격력"));
	inhenceAttackTextUI->SetColorBrush("WHITE");
	inhenceAttackTextUI->SetTextFormat("KOPUB21");
	inhenceAttackUI->SetChild(inhenceAttackTextUI);
	m_inhenceUI->SetChild(inhenceAttackUI);

	auto inhenceCritDamageUI{ make_shared<UI>(XMFLOAT2{-0.35f, 0.5f}, XMFLOAT2{0.2f, 0.2f}) };
	m_inhenceCritDamageSwtichUI = make_shared<SwitchUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{ 0.1f, 0.1f });
	m_inhenceCritDamageSwtichUI->SetTexture("INHENCECRITDAMAGE");
	inhenceCritDamageUI->SetChild(m_inhenceCritDamageSwtichUI);
	auto inhenceCritDamageTextUI{ make_shared<TextUI>(XMFLOAT2{ 0.f, -0.5f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 100.f, 20.f }) };
	inhenceCritDamageTextUI->SetText(TEXT("크리티컬 대미지"));
	inhenceCritDamageTextUI->SetColorBrush("WHITE");
	inhenceCritDamageTextUI->SetTextFormat("KOPUB21");
	inhenceCritDamageUI->SetChild(inhenceCritDamageTextUI);
	m_inhenceUI->SetChild(inhenceCritDamageUI);

	auto inhenceCritProbUI{ make_shared<UI>(XMFLOAT2{0.f, 0.5f}, XMFLOAT2{0.2f, 0.2f}) };
	m_inhenceCritProbSwitchUI = make_shared<SwitchUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{ 0.1f, 0.1f });
	m_inhenceCritProbSwitchUI->SetTexture("INHENCECRITPROB");
	inhenceCritProbUI->SetChild(m_inhenceCritProbSwitchUI);
	auto inhenceCritProbTextUI{ make_shared<TextUI>(XMFLOAT2{ 0.f, -0.5f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 100.f, 20.f }) };
	inhenceCritProbTextUI->SetText(TEXT("크리티컬 확률"));
	inhenceCritProbTextUI->SetColorBrush("WHITE");
	inhenceCritProbTextUI->SetTextFormat("KOPUB21");
	inhenceCritProbUI->SetChild(inhenceCritProbTextUI);
	m_inhenceUI->SetChild(inhenceCritProbUI);

	auto inhenceDefenceUI{ make_shared<UI>(XMFLOAT2{0.35f, 0.5f}, XMFLOAT2{0.2f, 0.2f}) };
	m_inhenceDefenceSwitchUI = make_shared<SwitchUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{ 0.1f, 0.1f });
	m_inhenceDefenceSwitchUI->SetTexture("INHENCEDEFENCE");
	inhenceDefenceUI->SetChild(m_inhenceDefenceSwitchUI);
	auto inhenceDefenceTextUI{ make_shared<TextUI>(XMFLOAT2{ 0.f, -0.5f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 100.f, 20.f }) };
	inhenceDefenceTextUI->SetText(TEXT("방어력"));
	inhenceDefenceTextUI->SetColorBrush("WHITE");
	inhenceDefenceTextUI->SetTextFormat("KOPUB21");
	inhenceDefenceUI->SetChild(inhenceDefenceTextUI);
	m_inhenceUI->SetChild(inhenceDefenceUI);

	auto inhenceHpUI{ make_shared<UI>(XMFLOAT2{0.7f, 0.5f}, XMFLOAT2{0.2f, 0.2f}) };
	m_inhenceHpSwitchUI = make_shared<SwitchUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{ 0.1f, 0.1f });
	m_inhenceHpSwitchUI->SetTexture("INHENCEHP");
	inhenceHpUI->SetChild(m_inhenceHpSwitchUI);
	auto inhenceHpTextUI{ make_shared<TextUI>(XMFLOAT2{ 0.f, -0.5f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 100.f, 20.f }) };
	inhenceHpTextUI->SetText(TEXT("최대 체력"));
	inhenceHpTextUI->SetColorBrush("WHITE");
	inhenceHpTextUI->SetTextFormat("KOPUB21");
	inhenceHpUI->SetChild(inhenceHpTextUI);
	m_inhenceUI->SetChild(inhenceHpUI);

	auto inhenceButtonUI = make_shared<ButtonUI>(XMFLOAT2{ 0.f, -0.7f }, XMFLOAT2{ 0.29f, 0.1f });
	inhenceButtonUI->SetTexture("BUTTONUI");
	inhenceButtonUI->SetClickEvent([&]() {
		if (m_inhenceAttackSwitchUI->IsActive()) {
			TryEnhancement(EnhancementType::ATK);
		}
		if (m_inhenceCritDamageSwtichUI->IsActive()) {
			TryEnhancement(EnhancementType::CRIT_DAMAGE);
		}
		if (m_inhenceCritProbSwitchUI->IsActive()) {
			TryEnhancement(EnhancementType::CRIT_RATE);
		}
		if (m_inhenceDefenceSwitchUI->IsActive()) {
			TryEnhancement(EnhancementType::DEF);
		}
		if (m_inhenceHpSwitchUI->IsActive()) {
			TryEnhancement(EnhancementType::HP);
		}
		});
	auto inhenceButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f},XMFLOAT2{120.f, 10.f}) };
	inhenceButtonTextUI->SetText(L"강화");
	inhenceButtonTextUI->SetColorBrush("WHITE");
	inhenceButtonTextUI->SetTextFormat("KOPUB21");
	inhenceButtonUI->SetChild(inhenceButtonTextUI);
	m_inhenceUI->SetChild(inhenceButtonUI);

	m_inhenceUI->SetDisable();
	m_shaders["UI"]->SetUI(m_inhenceUI);
}

inline void VillageScene::BuildMainUI()
{
	m_skillUI = make_shared<VertGaugeUI>(XMFLOAT2{ -0.60f, -0.75f }, XMFLOAT2{ 0.15f, 0.15f }, 0.f);
	m_ultimateUI = make_shared<VertGaugeUI>(XMFLOAT2{ -0.85f, -0.75f }, XMFLOAT2{ 0.15f, 0.15f }, 0.f);

	SetSkillUI();

	m_player->SetSkillGauge(m_skillUI);
	m_player->SetUltimateGauge(m_ultimateUI);

	m_shaders["UI"]->SetUI(m_skillUI);
	m_shaders["UI"]->SetUI(m_ultimateUI);
	m_player->ResetAllCooldown();

	auto goldUI = make_shared<UI>(XMFLOAT2{ 0.85f, 0.9f }, XMFLOAT2{ 0.1f, 0.05f });
	auto goldTextureUI{ make_shared<ImageUI>(XMFLOAT2{-0.5f, 0.f}, XMFLOAT2{0.05f, 0.05f}) };
	goldTextureUI->SetTexture("GOLDUI");
	goldUI->SetChild(goldTextureUI);
	m_goldTextUI = make_shared<TextUI>(XMFLOAT2{ 0.5f, 0.f }, XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 100.f, 20.f });
	m_goldTextUI->SetText(to_wstring(g_playerInfo.gold));
	m_goldTextUI->SetColorBrush("WHITE");
	m_goldTextUI->SetTextFormat("KOPUB27");
	goldUI->SetChild(m_goldTextUI);

	m_shaders["UI"]->SetUI(goldUI);
}

void VillageScene::BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
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

	m_lightSystem->CreateShaderVariable(device, commandlist);
}

void VillageScene::DestroyObjects()
{
	m_player.reset();
	m_camera.reset();
	m_terrain.reset();

	m_lightSystem.reset();
	m_shadow.reset();

	m_blurFilter.reset();
	m_fadeFilter.reset();

	m_quadtree.reset();
}


void VillageScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
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

	if (m_roomUI) m_roomUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	if (m_partyUI) m_partyUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	if (m_skillSettingUI) m_skillSettingUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	if (m_inhenceUI) m_inhenceUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
}

void VillageScene::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	if (!CheckState(State::CantPlayerControl)) {
		if (m_player) m_player->OnProcessingMouseMessage(message, lParam);
	}

	if (CheckState(State::OutputRoomUI)) {
		if (m_roomUI) m_roomUI->OnProcessingMouseMessage(message, lParam);
		if (!g_clickEventStack.empty()) {
			g_clickEventStack.top()();
			while (!g_clickEventStack.empty()) {
				g_clickEventStack.pop();
			}
		}
	}
	if (CheckState(State::OutputPartyUI)) {
		if (m_partyUI) m_partyUI->OnProcessingMouseMessage(message, lParam);
		if (!g_clickEventStack.empty()) {
			g_clickEventStack.top()();
			while (!g_clickEventStack.empty()) {
				g_clickEventStack.pop();
			}
		}
	}
	if (CheckState(State::OutputSkillUI)) {
		if (m_skillSettingUI) m_skillSettingUI->OnProcessingMouseMessage(message, lParam);
		if (!g_clickEventStack.empty()) {
			g_clickEventStack.top()();
			while (!g_clickEventStack.empty()) {
				g_clickEventStack.pop();
			}
		}
	}
	if (CheckState(State::OutputInhenceUI)) {
		if (m_inhenceUI) m_inhenceUI->OnProcessingMouseMessage(message, lParam);
		if (!g_clickEventStack.empty()) {
			g_clickEventStack.top()();
			while (!g_clickEventStack.empty()) {
				g_clickEventStack.pop();
			}
		}
	}
}

void VillageScene::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
	if (!CheckState(State::CantPlayerControl)) {
		if (m_player) m_player->OnProcessingKeyboardMessage(timeElapsed);

		if (GetAsyncKeyState('1') & 0x8000) {
			ChangeCharacter(PlayerType::WARRIOR, m_player);
		}
		if (GetAsyncKeyState('2') & 0x8000) {
			ChangeCharacter(PlayerType::ARCHER, m_player);
		}
	}

	if (GetAsyncKeyState(VK_TAB) & 0x8000) {
		SetState(State::SceneLeave);
	}
}

void VillageScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message)
	{
	case WM_CHAR:
		if (wParam == 'f' || wParam == 'F') {
			if (CheckState(State::DungeonInteract)) {
				ResetState(State::DungeonInteract);
				SetState(State::OutputRoomUI);
				m_roomUI->SetEnable();
				SendOpenPartyUI();
			}
			else if (CheckState(State::SkillInteract)) {
				ResetState(State::SkillInteract);
				SetState(State::OutputSkillUI);
				m_skillSettingUI->SetEnable();
			}
			else if (CheckState(State::InhenceInteract)) {
				ResetState(State::InhenceInteract);
				SetState(State::OutputInhenceUI);
				m_inhenceUI->SetEnable();
			}
		}
		break;
	}
}

void VillageScene::Update(FLOAT timeElapsed)
{
	if (CheckState(State::SceneLeave)) {
		g_GameFramework.ChangeScene(SCENETAG::TowerScene);
		return;
	}
#ifdef USE_NETWORK
	RecvPacket();
#endif

	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shaders)
		shader.second->Update(timeElapsed);
	m_fadeFilter->Update(timeElapsed);

	CollideWithMap();
	UpdateInteract(timeElapsed);

	// 프러스텀 컬링을 진행하는 셰이더에 바운딩 프러스텀 전달
	auto viewFrustum = m_camera->GetViewFrustum();
	static_pointer_cast<StaticObjectShader>(m_shaders["OBJECT1"])->SetBoundingFrustum(viewFrustum);
	static_pointer_cast<StaticObjectShader>(m_shaders["OBJECT2"])->SetBoundingFrustum(viewFrustum);
	static_pointer_cast<StaticObjectBlendShader>(m_shaders["OBJECTBLEND"])->SetBoundingFrustum(viewFrustum);
}

void VillageScene::UpdateInteract(FLOAT timeElapsed)
{
	m_interactUI->SetDisable();

	ResetState(State::DungeonInteract);
	ResetState(State::SkillInteract);
	ResetState(State::InhenceInteract);
	if (CheckState(State::OutputUI)) return;

	const auto position = m_player->GetPosition();
	bool isInteract = false;

	if ((position.x >= VillageSetting::NORTH_GATE_LEFT && position.x <= VillageSetting::NORTH_GATE_RIGHT && position.z >= VillageSetting::NORTH_GATE_FRONT) ||
		(position.x <= VillageSetting::SOUTH_GATE_LEFT && position.x >= VillageSetting::SOUTH_GATE_RIGHT && position.z <= VillageSetting::SOUTH_GATE_FRONT) ||
		(position.z >= VillageSetting::WEST_GATE_LEFT && position.z <= VillageSetting::WEST_GATE_RIGHT && position.x <= VillageSetting::WEST_GATE_FRONT) ||
		(position.z <= VillageSetting::EAST_GATE_LEFT && position.z >= VillageSetting::EAST_GATE_RIGHT && position.x >= VillageSetting::EAST_GATE_FRONT)) {
		isInteract = true;
	}

	if (isInteract) {
		m_interactUI->SetEnable();
		SetState(State::DungeonInteract);
		m_interactTextUI->SetText(TEXT("F : 파티 생성"));
		return;
	}

	if (position.x >= VillageSetting::SKILL_NPC.x - VillageSetting::SKILL_NPC_OFFSET && 
		position.x <= VillageSetting::SKILL_NPC.x + VillageSetting::SKILL_NPC_OFFSET && 
		position.z >= VillageSetting::SKILL_NPC.z - VillageSetting::SKILL_NPC_OFFSET &&
		position.z <= VillageSetting::SKILL_NPC.z + VillageSetting::SKILL_NPC_OFFSET) {
		isInteract = true;
	}

	if (isInteract) {
		m_interactUI->SetEnable();
		SetState(State::SkillInteract);
		m_interactTextUI->SetText(TEXT("F : 스킬 설정"));
		return;
	}

	if (position.x >= VillageSetting::INHENCE_NPC.x - VillageSetting::INHENCE_NPC_OFFSET &&
		position.x <= VillageSetting::INHENCE_NPC.x + VillageSetting::INHENCE_NPC_OFFSET &&
		position.z >= VillageSetting::INHENCE_NPC.z - VillageSetting::INHENCE_NPC_OFFSET &&
		position.z <= VillageSetting::INHENCE_NPC.z + VillageSetting::INHENCE_NPC_OFFSET) {
		isInteract = true;
	}

	if (isInteract) {
		m_interactUI->SetEnable();
		SetState(State::InhenceInteract);
		m_interactTextUI->SetText(TEXT("F : 능력치 강화"));
		return;
	}
}

void VillageScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) 
{
	if (CheckState(State::SceneLeave)) return;
	switch (threadIndex)
	{
	case 0:
	{
		m_shaders["OBJECT1"]->Render(commandList, m_shaders["SHADOW"]);
		break;
	}
	case 1:
	{
		m_shaders["OBJECT2"]->Render(commandList, m_shaders["SHADOW"]);
		break;
	}
	case 2:
	{
		m_shaders["OBJECTBLEND"]->Render(commandList, m_shaders["SHADOW"]);
		m_shaders["ANIMATION"]->Render(commandList, m_shaders["ANIMATIONSHADOW"]);
		break;
	}
	}
}

void VillageScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const
{
	if (CheckState(State::SceneLeave)) return;
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
	if (m_lightSystem) m_lightSystem->UpdateShaderVariable(commandList);

	switch (threadIndex)
	{
	case 0:
	{
		m_shaders.at("SKYBOX")->Render(commandList);
		m_shaders.at("TERRAIN")->Render(commandList);
		m_shaders.at("OBJECT1")->Render(commandList);
		break;
	}
	case 1:
	{
		m_shaders.at("OBJECT2")->Render(commandList);
		break;
	}
	case 2:
	{
		m_shaders.at("ANIMATION")->Render(commandList);
		m_shaders.at("OBJECTBLEND")->Render(commandList);
		//m_shaders["WIREFRAME"]->Render(commandList);
		m_shaders.at("UI")->Render(commandList);
		break;
	}
	}

}

void VillageScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex)
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

void VillageScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	if (CheckState(State::SceneLeave)) return;
	if (m_interactUI) m_interactUI->RenderText(deviceContext);
	if (m_roomUI) m_roomUI->RenderText(deviceContext);
	if (m_partyUI) m_partyUI->RenderText(deviceContext);
	if (m_skillSettingUI) m_skillSettingUI->RenderText(deviceContext);
	if (m_inhenceUI) m_inhenceUI->RenderText(deviceContext);
	if (m_goldTextUI) m_goldTextUI->RenderText(deviceContext);
}

void VillageScene::PostRenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	if (CheckState(State::SceneLeave)) return;
}

bool VillageScene::CheckState(State sceneState) const
{
	return m_sceneState & (INT)sceneState;
}

void VillageScene::SetState(State sceneState)
{
	m_sceneState |= (INT)sceneState;
}

void VillageScene::ResetState(State sceneState)
{
	m_sceneState &= ~(INT)sceneState;
}

void VillageScene::ProcessPacket(char* ptr)
{
	switch (ptr[1])
	{
	case SC_PACKET_PARTY_INFO:
		RecvPartyInfo(ptr);
		break;
	case SC_PACKET_JOIN_OK:
		RecvJoinOk(ptr);
		break;
	case SC_PACKET_JOIN_FAIL:
		RecvJoinFail(ptr);
		break;
	case SC_PACKET_CREATE_OK:
		RecvCreateOk(ptr);
		break;
	case SC_PACKET_CREATE_FAIL:
		RecvCreateFail(ptr);
		break;
	/*default:
		cout << "UnDefined Packet : " << (int)ptr[1] << endl;
		break;*/
	}
}

void VillageScene::RecvPartyInfo(char* ptr)
{
	SC_PARTY_INFO_PACKET* packet = reinterpret_cast<SC_PARTY_INFO_PACKET*>(ptr);

	for (INT i = 0; i < MAX_PARTIES_ON_PAGE; ++i) {
		if (0 == packet->info.current_player) {
			m_roomSwitchTextUI[i]->SetText(TEXT("빈 방"));
		}
		else {
			m_roomSwitchTextUI[static_cast<INT>(packet->info.party_num)]->SetText(
				packet->info.name + L"   " + to_wstring(packet->info.current_player) +
				to_wstring(MAX_INGAME_USER));
		}
	}
}

void VillageScene::RecvJoinOk(char* ptr)
{
	SC_JOIN_OK_PACKET* packet = reinterpret_cast<SC_JOIN_OK_PACKET*>(ptr);

	cout << "참가 성공!" << endl;
	// 파티원 추가

	ResetState(State::OutputRoomUI);
	if (m_roomUI) m_roomUI->SetDisable();
	SetState(State::OutputPartyUI);
	if (m_partyUI) m_partyUI->SetEnable();
}

void VillageScene::RecvJoinFail(char* ptr)
{
	SC_JOIN_FAIL_PACKET* packet = reinterpret_cast<SC_JOIN_FAIL_PACKET*>(ptr);

	cout << "참가 실패!" << endl;
}

void VillageScene::RecvCreateOk(char* ptr)
{
	SC_CREATE_OK_PACKET* packet = reinterpret_cast<SC_CREATE_OK_PACKET*>(ptr);

	cout << "생성 성공!" << endl;

	ResetState(State::OutputRoomUI);
	if (m_roomUI) m_roomUI->SetDisable();
	SetState(State::OutputPartyUI);
	if (m_partyUI) m_partyUI->SetEnable();
}

void VillageScene::RecvCreateFail(char* ptr)
{
	SC_CREATE_FAIL_PACKET* packet = reinterpret_cast<SC_CREATE_FAIL_PACKET*>(ptr);

	cout << "생성 실패!" << endl;
}

void VillageScene::SendChangePage()
{
#ifdef USE_NETWORK
	CS_CHANGE_PARTY_PAGE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CHANGE_PARTY_PAGE;
	packet.page = m_roomPage;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void VillageScene::SendOpenPartyUI()
{
#ifdef USE_NETWORK
	CS_OPEN_PARTY_UI_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_OPEN_PARTY_UI;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void VillageScene::SendClosePartyUI()
{
#ifdef USE_NETWORK
	CS_CLOSE_PARTY_UI_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CLOSE_PARTY_UI;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void VillageScene::SendCreateParty()
{
#ifdef USE_NETWORK
	CS_CREATE_PARTY_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CREATE_PARTY;
	//packet.party_num = m_roomPage * MAX_PARTIES_ON_PAGE + m_selectedRoom;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void VillageScene::SendJoinParty()
{
#ifdef USE_NETWORK
	CS_JOIN_PARTY_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_JOIN_PARTY;
	//packet.party_num = m_roomPage * MAX_PARTIES_ON_PAGE + m_selectedRoom;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void VillageScene::SendEnhancement(EnhancementType type)
{
#ifdef USE_NETWORK
	CS_ENHANCE_PACKET packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_ENHANCE;
	packet.enhancement_type = type;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void VillageScene::LoadSceneFromFile(wstring fileName, wstring sceneName)
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

		FLOAT pitch;
		in.read((CHAR*)(&pitch), sizeof(FLOAT));
		FLOAT yaw;
		in.read((CHAR*)(&yaw), sizeof(FLOAT));
		FLOAT roll;
		in.read((CHAR*)(&roll), sizeof(FLOAT));
		XMVECTOR quarternion = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);

		XMFLOAT3 scale;
		in.read((CHAR*)(&scale), sizeof(XMFLOAT3));

		if (objectName == "Nonblocking") continue;

		m_quadtree->SetGameObject(object);
		auto boundingBox = object->GetBoundingBox();
		boundingBox.Extents.x *= scale.x;
		boundingBox.Extents.y *= scale.y;
		boundingBox.Extents.z *= scale.z;
		object->SetBoundingBox(boundingBox);

		XMStoreFloat4(&boundingBox.Orientation, quarternion);
		DrawBoundingBox(boundingBox, roll, pitch, yaw);

		static int threadNum = 0;
		if (objectName == "Nonblocking" || objectName == "Blocking") continue;
		else if (IsBlendObject(objectName)) {
			m_shaders["OBJECTBLEND"]->SetObject(object);

		}
		else if (threadNum == 0) {
			m_shaders["OBJECT1"]->SetObject(object);
			threadNum = 1;
		}
		else {
			m_shaders["OBJECT2"]->SetObject(object);
			threadNum = 0;
		}
	}
}

void VillageScene::LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object)
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

void VillageScene::LoadPlayerFromFile(const shared_ptr<Player>& player)
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
		XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f} };
	player->SetBoundingBox(obb);

	player->SetAnimationSet(m_animationSets[animationSet], animationSet);
	player->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	player->GetAnimationController()->SetTrackEnable(1, false);
	player->GetAnimationController()->SetTrackEnable(2, false);
}

void VillageScene::LoadNPCFromFile(const shared_ptr<AnimationObject>& npc)
{
	wstring filePath = TEXT("./Resource/Model/Warrior.bin");
	string animationSet = "WarriorAnimation";

	LoadObjectFromFile(filePath, npc);

	BoundingOrientedBox obb = BoundingOrientedBox{ npc->GetPosition(),
		XMFLOAT3{0.37f, 0.65f, 0.37f}, XMFLOAT4{0.f, 0.f, 0.f, 1.f} };
	npc->SetBoundingBox(obb);

	npc->SetAnimationSet(m_animationSets[animationSet], animationSet);
	npc->SetAnimationOnTrack(0, ObjectAnimation::IDLE);
	npc->GetAnimationController()->SetTrackEnable(1, false);
	npc->GetAnimationController()->SetTrackEnable(2, false);
}

bool VillageScene::IsBlendObject(const string& objectName)
{
	if (objectName == "Flower_E_01") return true;
	if (objectName == "Flower_F_01") return true;
	if (objectName == "Flower_G_01") return true;
	if (objectName == "MV_Tree_A_01") return true;
	if (objectName == "MV_Tree_A_02") return true;
	if (objectName == "MV_Tree_A_03") return true;
	if (objectName == "MV_Tree_A_04") return true;
	if (objectName == "MV_Tree_B_01") return true;
	if (objectName == "MV_Tree_B_02") return true;
	if (objectName == "MV_Tree_B_03") return true;
	if (objectName == "MV_Tree_B_04") return true;
	if (objectName == "Ivy_A_01") return true;
	if (objectName == "Ivy_A_02") return true;
	if (objectName == "Ivy_A_03") return true;
	if (objectName == "Ivy_B_01") return true;
	if (objectName == "Ivy_B_02") return true;
	if (objectName == "Ivy_B_03") return true;
	if (objectName == "Decal_A") return true;
	return false;
}

bool VillageScene::IsCollideExceptObject(const string& objectName)
{
	// 충돌에서 예외 처리할 오브젝트들이다.
	if (objectName == "TownGroundTile_A") return true;
	if (objectName == "TownGroundTile_B") return true;
	if (objectName == "TownGroundTile_C") return true;
	if (objectName == "TownGroundTile_D") return true;
	if (objectName == "TownGroundTile_E") return true;
	if (objectName == "TownGroundTile_F") return true;
	if (objectName == "TownGroundTile_G") return true;
	if (objectName == "TownGroundTile_H") return true;
	if (objectName == "TownGroundTile_I") return true;
	if (objectName == "TownGroundTile_J") return true;
	if (objectName == "TownGroundTile_K") return true;
	if (objectName == "TownGroundTile_M") return true;
	if (objectName == "TownGroundTile_N") return true;
	if (objectName == "TownGroundTile_L") return true;
	if (objectName == "TownGroundTile_O") return true;
	if (objectName == "TownGroundTile_P") return true;
	if (objectName == "TownGroundTile_Q") return true;
	if (objectName == "TownGroundTile_R") return true;
	if (objectName == "TownGroundTile_S") return true;
	if (objectName == "TownGroundTile_T") return true;
	if (objectName == "TownGroundTile_U") return true;
	if (objectName == "TownGroundTile_V") return true;
	if (objectName == "TownGroundTile_W") return true;
	if (objectName == "BilldingStair_A") return true;
	if (objectName == "BilldingStair_B") return true;
	if (objectName == "BilldingStair_C") return true;
	if (objectName == "BilldingStair_D") return true;
	if (objectName == "BilldingStair_E") return true;
	if (objectName == "BilldingStair_F") return true;
	if (objectName == "BilldingStair_G") return true;
	if (objectName == "BilldingStair_H") return true;
	if (objectName == "Flower_A") return true;
	if (objectName == "Flower_B") return true;
	if (objectName == "Flower_C") return true;
	if (objectName == "Flower_D") return true;
	if (objectName == "Decal_A") return true;
	if (objectName == "Bridge_A") return true;

	return false;
}

void VillageScene::DrawBoundingBox(BoundingOrientedBox boundingBox, FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	auto wireFrame = make_shared<GameObject>();

	wireFrame->Rotate(roll, pitch, yaw);
	wireFrame->SetScale(boundingBox.Extents.x, boundingBox.Extents.y, boundingBox.Extents.z);
	wireFrame->SetPosition(boundingBox.Center);

	wireFrame->SetMesh("WIREFRAME");
	m_shaders["WIREFRAME"]->SetObject(wireFrame);
}

void VillageScene::CollideWithMap()
{
	if (MoveOnTerrain()) {}
	else if (MoveOnStairs()) {}
	for (auto& object : m_quadtree->GetGameObjects(m_player->GetBoundingBox())) {
		CollideByStaticOBB(m_player, object);
	}
}

void VillageScene::CollideByStaticOBB(const shared_ptr<GameObject>& object, const shared_ptr<GameObject>& staticObject)
{
	if (IsCollideExceptObject(staticObject->GetFrameName())) return;
	if (!object->GetBoundingBox().Intersects(staticObject->GetBoundingBox())) return;

	// GetCorners()
	// (-, -, +), (+, -, +), (+, +, +), (-, +, +)
	// (-, -, -), (+, -, -), (+, +, -), (-, +, -)

	XMFLOAT3 corners[8]{};

	BoundingOrientedBox playerBoundingBox = object->GetBoundingBox();
	playerBoundingBox.GetCorners(corners);

	XMFLOAT3 playerPoints[4] = {
		{corners[0].x, 0.f, corners[0].z},
		{corners[1].x, 0.f, corners[1].z},
		{corners[5].x, 0.f, corners[5].z},
		{corners[4].x, 0.f, corners[4].z} 
	};

	BoundingOrientedBox staticBoundingBox = staticObject->GetBoundingBox();
	staticBoundingBox.Center.y = 0.f;
	staticBoundingBox.GetCorners(corners);

	// 꼭짓점 시계방향 0,1,5,4
	XMFLOAT3 objectPoints[4] = {
		{corners[0].x, 0.f, corners[0].z}, // (-, -, +)
		{corners[1].x, 0.f, corners[1].z}, // (+, -, +)
		{corners[5].x, 0.f, corners[5].z}, // (+, -, -)
		{corners[4].x, 0.f, corners[4].z}	// (-, -, -)
	};

	for (const XMFLOAT3& playerPoint : playerPoints) {
		if (!staticBoundingBox.Contains(XMLoadFloat3(&playerPoint))) continue;
		cout << staticObject->GetFrameName() << " 충돌" << endl;

		std::array<float, 4> dist{};
		dist[0] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&objectPoints[0]), 
			XMLoadFloat3(&objectPoints[1]), XMLoadFloat3(&playerPoint)));
		dist[1] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&objectPoints[1]), 
			XMLoadFloat3(&objectPoints[2]), XMLoadFloat3(&playerPoint)));
		dist[2] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&objectPoints[2]), 
			XMLoadFloat3(&objectPoints[3]), XMLoadFloat3(&playerPoint)));
		dist[3] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&objectPoints[3]), 
			XMLoadFloat3(&objectPoints[0]), XMLoadFloat3(&playerPoint)));

		auto min = min_element(dist.begin(), dist.end());

		XMFLOAT3 v{};
		if (*min == dist[0]) v = Vector3::Normalize(Vector3::Sub(objectPoints[1], objectPoints[2]));
		else if (*min == dist[1]) v = Vector3::Normalize(Vector3::Sub(objectPoints[1], objectPoints[0]));
		else if (*min == dist[2]) v = Vector3::Normalize(Vector3::Sub(objectPoints[2], objectPoints[1]));
		else if (*min == dist[3]) v = Vector3::Normalize(Vector3::Sub(objectPoints[0], objectPoints[1]));
		v = Vector3::Mul(v, *min);
		object->SetPosition(Vector3::Add(object->GetPosition(), v));
		break;
	}
}

bool VillageScene::MoveOnTerrain()
{
	XMFLOAT3 pos = m_player->GetPosition();

	if (pos.x >= 42.44f) m_onTerrain = true;
	if (pos.x <= -184.8f) m_onTerrain = true;
	if (pos.z >= 241.7f) m_onTerrain = true;
	if (pos.z <= -19.f) m_onTerrain = true;

	if (pos.x < 42.44f && VillageSetting::STAIRS1_BACK <= pos.x &&
		pos.z >= VillageSetting::STAIRS1_LEFT && 
		pos.z <= VillageSetting::STAIRS1_RIGHT) m_onTerrain = false;
	if (pos.x > -184.8f && VillageSetting::STAIRS14_BACK >= pos.x &&
		pos.z <= VillageSetting::STAIRS14_LEFT && 
		pos.z >= VillageSetting::STAIRS14_RIGHT) m_onTerrain = false;
	if (pos.z < 241.7f && VillageSetting::STAIRS11_BACK <= pos.z &&
		pos.x <= VillageSetting::STAIRS11_LEFT && 
		pos.x >= VillageSetting::STAIRS11_RIGHT) m_onTerrain = false;
	if (pos.z > -19.f && VillageSetting::STAIRS15_BACK >= pos.z &&
		pos.x <= VillageSetting::STAIRS15_LEFT &&
		pos.x >= VillageSetting::STAIRS15_RIGHT) m_onTerrain = false;

	if (m_onTerrain) {
		pos.y = m_terrain->GetHeight(pos.x, pos.z);
		pos.y += 0.1f;
		m_player->SetPosition(pos);
		return true;
	}
	return false;
}

bool VillageScene::MoveOnStairs()
{
	XMFLOAT3 pos = m_player->GetPosition();

	float ratio{};
	if (pos.z >= VillageSetting::STAIRS1_LEFT &&
		pos.z <= VillageSetting::STAIRS1_RIGHT &&
		pos.x <= VillageSetting::STAIRS1_FRONT &&
		pos.x >= VillageSetting::STAIRS1_BACK) {
		ratio = (VillageSetting::STAIRS1_FRONT - pos.x) /
			(VillageSetting::STAIRS1_FRONT - VillageSetting::STAIRS1_BACK);
		pos.y = VillageSetting::STAIRS1_BOTTOM +
			(VillageSetting::STAIRS1_TOP - VillageSetting::STAIRS1_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS2_LEFT &&
		pos.z <= VillageSetting::STAIRS2_RIGHT &&
		pos.x <= VillageSetting::STAIRS2_FRONT &&
		pos.x >= VillageSetting::STAIRS2_BACK) {
		ratio = (VillageSetting::STAIRS2_FRONT - pos.x) /
			(VillageSetting::STAIRS2_FRONT - VillageSetting::STAIRS2_BACK);
		pos.y = VillageSetting::STAIRS2_BOTTOM +
			(VillageSetting::STAIRS2_TOP - VillageSetting::STAIRS2_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z <= VillageSetting::STAIRS3_LEFT &&
		pos.z >= VillageSetting::STAIRS3_RIGHT &&
		pos.x <= VillageSetting::STAIRS3_FRONT &&
		pos.x >= VillageSetting::STAIRS3_BACK) {
		ratio = (VillageSetting::STAIRS3_BACK - pos.x) /
			(VillageSetting::STAIRS3_BACK - VillageSetting::STAIRS3_FRONT);
		pos.y = VillageSetting::STAIRS3_BOTTOM +
			(VillageSetting::STAIRS3_TOP - VillageSetting::STAIRS3_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS4_LEFT &&
		pos.z <= VillageSetting::STAIRS4_RIGHT &&
		pos.x <= VillageSetting::STAIRS4_FRONT &&
		pos.x >= VillageSetting::STAIRS4_BACK) {
		ratio = (VillageSetting::STAIRS4_FRONT - pos.x) /
			(VillageSetting::STAIRS4_FRONT - VillageSetting::STAIRS4_BACK);
		pos.y = VillageSetting::STAIRS4_BOTTOM +
			(VillageSetting::STAIRS4_TOP - VillageSetting::STAIRS4_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS5_LEFT &&
		pos.z <= VillageSetting::STAIRS5_RIGHT &&
		pos.x <= VillageSetting::STAIRS5_FRONT &&
		pos.x >= VillageSetting::STAIRS5_BACK) {
		ratio = (VillageSetting::STAIRS5_FRONT - pos.x) /
			(VillageSetting::STAIRS5_FRONT - VillageSetting::STAIRS5_BACK);
		pos.y = VillageSetting::STAIRS5_BOTTOM +
			(VillageSetting::STAIRS5_TOP - VillageSetting::STAIRS5_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS6_LEFT &&
		pos.z <= VillageSetting::STAIRS6_RIGHT &&
		pos.x <= VillageSetting::STAIRS6_FRONT &&
		pos.x >= VillageSetting::STAIRS6_BACK) {
		ratio = (VillageSetting::STAIRS6_FRONT - pos.x) /
			(VillageSetting::STAIRS6_FRONT - VillageSetting::STAIRS6_BACK);
		pos.y = VillageSetting::STAIRS6_BOTTOM +
			(VillageSetting::STAIRS6_TOP - VillageSetting::STAIRS6_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS7_LEFT &&
		pos.x >= VillageSetting::STAIRS7_RIGHT &&
		pos.z <= VillageSetting::STAIRS7_FRONT &&
		pos.z >= VillageSetting::STAIRS7_BACK) {
		ratio = (VillageSetting::STAIRS7_FRONT - pos.z) /
			(VillageSetting::STAIRS7_FRONT - VillageSetting::STAIRS7_BACK);
		pos.y = VillageSetting::STAIRS7_BOTTOM +
			(VillageSetting::STAIRS7_TOP - VillageSetting::STAIRS7_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS8_LEFT &&
		pos.x >= VillageSetting::STAIRS8_RIGHT &&
		pos.z >= VillageSetting::STAIRS8_FRONT &&
		pos.z <= VillageSetting::STAIRS8_BACK) {
		ratio = (VillageSetting::STAIRS8_FRONT - pos.z) /
			(VillageSetting::STAIRS8_FRONT - VillageSetting::STAIRS8_BACK);
		pos.y = VillageSetting::STAIRS8_BOTTOM +
			(VillageSetting::STAIRS8_TOP - VillageSetting::STAIRS8_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS9_LEFT &&
		pos.x >= VillageSetting::STAIRS9_RIGHT &&
		pos.z >= VillageSetting::STAIRS9_FRONT &&
		pos.z <= VillageSetting::STAIRS9_BACK) {
		ratio = (VillageSetting::STAIRS9_FRONT - pos.z) /
			(VillageSetting::STAIRS9_FRONT - VillageSetting::STAIRS9_BACK);
		pos.y = VillageSetting::STAIRS9_BOTTOM +
			(VillageSetting::STAIRS9_TOP - VillageSetting::STAIRS9_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS10_LEFT &&
		pos.x >= VillageSetting::STAIRS10_RIGHT &&
		pos.z >= VillageSetting::STAIRS10_FRONT &&
		pos.z <= VillageSetting::STAIRS10_BACK) {
		ratio = (VillageSetting::STAIRS10_FRONT - pos.z) /
			(VillageSetting::STAIRS10_FRONT - VillageSetting::STAIRS10_BACK);
		pos.y = VillageSetting::STAIRS10_BOTTOM +
			(VillageSetting::STAIRS10_TOP - VillageSetting::STAIRS10_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS11_LEFT &&
		pos.x >= VillageSetting::STAIRS11_RIGHT &&
		pos.z <= VillageSetting::STAIRS11_FRONT &&
		pos.z >= VillageSetting::STAIRS11_BACK) {
		ratio = (VillageSetting::STAIRS11_FRONT - pos.z) /
			(VillageSetting::STAIRS11_FRONT - VillageSetting::STAIRS11_BACK);
		pos.y = VillageSetting::STAIRS11_BOTTOM +
			(VillageSetting::STAIRS11_TOP - VillageSetting::STAIRS11_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS12_LEFT &&
		pos.z <= VillageSetting::STAIRS12_RIGHT &&
		pos.x <= VillageSetting::STAIRS12_FRONT &&
		pos.x >= VillageSetting::STAIRS12_BACK) {
		ratio = (VillageSetting::STAIRS12_FRONT - pos.x) /
			(VillageSetting::STAIRS12_FRONT - VillageSetting::STAIRS12_BACK);
		pos.y = VillageSetting::STAIRS12_BOTTOM +
			(VillageSetting::STAIRS12_TOP - VillageSetting::STAIRS12_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x >= VillageSetting::STAIRS13_LEFT &&
		pos.x <= VillageSetting::STAIRS13_RIGHT &&
		pos.z >= VillageSetting::STAIRS13_FRONT &&
		pos.z <= VillageSetting::STAIRS13_BACK) {
		ratio = (VillageSetting::STAIRS13_FRONT - pos.z) /
			(VillageSetting::STAIRS13_FRONT - VillageSetting::STAIRS13_BACK);
		pos.y = VillageSetting::STAIRS13_BOTTOM +
			(VillageSetting::STAIRS13_TOP - VillageSetting::STAIRS13_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z <= VillageSetting::STAIRS14_LEFT &&
		pos.z >= VillageSetting::STAIRS14_RIGHT &&
		pos.x >= VillageSetting::STAIRS14_FRONT &&
		pos.x <= VillageSetting::STAIRS14_BACK) {
		ratio = (VillageSetting::STAIRS14_FRONT - pos.x) /
			(VillageSetting::STAIRS14_FRONT - VillageSetting::STAIRS14_BACK);
		pos.y = VillageSetting::STAIRS14_BOTTOM +
			(VillageSetting::STAIRS14_TOP - VillageSetting::STAIRS14_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS15_LEFT &&
		pos.x >= VillageSetting::STAIRS15_RIGHT &&
		pos.z >= VillageSetting::STAIRS15_FRONT &&
		pos.z <= VillageSetting::STAIRS15_BACK) {
		ratio = (VillageSetting::STAIRS15_FRONT - pos.z) /
			(VillageSetting::STAIRS15_FRONT - VillageSetting::STAIRS15_BACK);
		pos.y = VillageSetting::STAIRS15_BOTTOM +
			(VillageSetting::STAIRS15_TOP - VillageSetting::STAIRS15_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x >= VillageSetting::STAIRS16_LEFT &&
		pos.x <= VillageSetting::STAIRS16_RIGHT &&
		pos.z >= VillageSetting::STAIRS16_FRONT &&
		pos.z <= VillageSetting::STAIRS16_BACK) {
		ratio = (VillageSetting::STAIRS16_FRONT - pos.z) /
			(VillageSetting::STAIRS16_FRONT - VillageSetting::STAIRS16_BACK);
		pos.y = VillageSetting::STAIRS16_BOTTOM +
			(VillageSetting::STAIRS16_TOP - VillageSetting::STAIRS16_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x >= VillageSetting::STAIRS17_LEFT &&
		pos.x <= VillageSetting::STAIRS17_RIGHT &&
		pos.z >= VillageSetting::STAIRS17_FRONT &&
		pos.z <= VillageSetting::STAIRS17_BACK) {
		ratio = (VillageSetting::STAIRS17_FRONT - pos.z) /
			(VillageSetting::STAIRS17_FRONT - VillageSetting::STAIRS17_BACK);
		pos.y = VillageSetting::STAIRS17_BOTTOM +
			(VillageSetting::STAIRS17_TOP - VillageSetting::STAIRS17_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS18_LEFT &&
		pos.x >= VillageSetting::STAIRS18_RIGHT &&
		pos.z <= VillageSetting::STAIRS18_FRONT &&
		pos.z >= VillageSetting::STAIRS18_BACK) {
		ratio = (VillageSetting::STAIRS18_FRONT - pos.z) /
			(VillageSetting::STAIRS18_FRONT - VillageSetting::STAIRS18_BACK);
		pos.y = VillageSetting::STAIRS18_BOTTOM +
			(VillageSetting::STAIRS18_TOP - VillageSetting::STAIRS18_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x >= VillageSetting::STAIRS19_LEFT &&
		pos.x <= VillageSetting::STAIRS19_RIGHT &&
		pos.z >= VillageSetting::STAIRS19_FRONT &&
		pos.z <= VillageSetting::STAIRS19_BACK) {
		ratio = (VillageSetting::STAIRS19_FRONT - pos.z) /
			(VillageSetting::STAIRS19_FRONT - VillageSetting::STAIRS19_BACK);
		pos.y = VillageSetting::STAIRS19_BOTTOM +
			(VillageSetting::STAIRS19_TOP - VillageSetting::STAIRS19_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS20_LEFT &&
		pos.x >= VillageSetting::STAIRS20_RIGHT &&
		pos.z <= VillageSetting::STAIRS20_FRONT &&
		pos.z >= VillageSetting::STAIRS20_BACK) {
		ratio = (VillageSetting::STAIRS20_FRONT - pos.z) /
			(VillageSetting::STAIRS20_FRONT - VillageSetting::STAIRS20_BACK);
		pos.y = VillageSetting::STAIRS20_BOTTOM +
			(VillageSetting::STAIRS20_TOP - VillageSetting::STAIRS20_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS21_LEFT &&
		pos.x >= VillageSetting::STAIRS21_RIGHT &&
		pos.z <= VillageSetting::STAIRS21_FRONT &&
		pos.z >= VillageSetting::STAIRS21_BACK) {
		ratio = (VillageSetting::STAIRS21_FRONT - pos.z) /
			(VillageSetting::STAIRS21_FRONT - VillageSetting::STAIRS21_BACK);
		pos.y = VillageSetting::STAIRS21_BOTTOM +
			(VillageSetting::STAIRS21_TOP - VillageSetting::STAIRS21_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x >= VillageSetting::STAIRS22_LEFT &&
		pos.x <= VillageSetting::STAIRS22_RIGHT &&
		pos.z >= VillageSetting::STAIRS22_FRONT &&
		pos.z <= VillageSetting::STAIRS22_BACK) {
		ratio = (VillageSetting::STAIRS22_FRONT - pos.z) /
			(VillageSetting::STAIRS22_FRONT - VillageSetting::STAIRS22_BACK);
		pos.y = VillageSetting::STAIRS22_BOTTOM +
			(VillageSetting::STAIRS22_TOP - VillageSetting::STAIRS22_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x >= VillageSetting::STAIRS23_LEFT &&
		pos.x <= VillageSetting::STAIRS23_RIGHT &&
		pos.z >= VillageSetting::STAIRS23_FRONT &&
		pos.z <= VillageSetting::STAIRS23_BACK) {
		ratio = (VillageSetting::STAIRS23_FRONT - pos.z) /
			(VillageSetting::STAIRS23_FRONT - VillageSetting::STAIRS23_BACK);
		pos.y = VillageSetting::STAIRS23_BOTTOM +
			(VillageSetting::STAIRS23_TOP - VillageSetting::STAIRS23_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.x <= VillageSetting::STAIRS24_LEFT &&
		pos.x >= VillageSetting::STAIRS24_RIGHT &&
		pos.z <= VillageSetting::STAIRS24_FRONT &&
		pos.z >= VillageSetting::STAIRS24_BACK) {
		ratio = (VillageSetting::STAIRS24_FRONT - pos.z) /
			(VillageSetting::STAIRS24_FRONT - VillageSetting::STAIRS24_BACK);
		pos.y = VillageSetting::STAIRS24_BOTTOM +
			(VillageSetting::STAIRS24_TOP - VillageSetting::STAIRS24_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS25_LEFT &&
		pos.z <= VillageSetting::STAIRS25_RIGHT &&
		pos.x <= VillageSetting::STAIRS25_FRONT &&
		pos.x >= VillageSetting::STAIRS25_BACK) {
		ratio = (VillageSetting::STAIRS25_FRONT - pos.x) /
			(VillageSetting::STAIRS25_FRONT - VillageSetting::STAIRS25_BACK);
		pos.y = VillageSetting::STAIRS25_BOTTOM +
			(VillageSetting::STAIRS25_TOP - VillageSetting::STAIRS25_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z <= VillageSetting::STAIRS26_LEFT &&
		pos.z >= VillageSetting::STAIRS26_RIGHT &&
		pos.x >= VillageSetting::STAIRS26_FRONT &&
		pos.x <= VillageSetting::STAIRS26_BACK) {
		ratio = (VillageSetting::STAIRS26_FRONT - pos.x) /
			(VillageSetting::STAIRS26_FRONT - VillageSetting::STAIRS26_BACK);
		pos.y = VillageSetting::STAIRS26_BOTTOM +
			(VillageSetting::STAIRS26_TOP - VillageSetting::STAIRS26_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z >= VillageSetting::STAIRS27_LEFT &&
		pos.z <= VillageSetting::STAIRS27_RIGHT &&
		pos.x <= VillageSetting::STAIRS27_FRONT &&
		pos.x >= VillageSetting::STAIRS27_BACK) {
		ratio = (VillageSetting::STAIRS27_FRONT - pos.x) /
			(VillageSetting::STAIRS27_FRONT - VillageSetting::STAIRS27_BACK);
		pos.y = VillageSetting::STAIRS27_BOTTOM +
			(VillageSetting::STAIRS27_TOP - VillageSetting::STAIRS27_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	if (pos.z <= VillageSetting::STAIRS28_LEFT &&
		pos.z >= VillageSetting::STAIRS28_RIGHT &&
		pos.x >= VillageSetting::STAIRS28_FRONT &&
		pos.x <= VillageSetting::STAIRS28_BACK) {
		ratio = (VillageSetting::STAIRS28_FRONT - pos.x) /
			(VillageSetting::STAIRS28_FRONT - VillageSetting::STAIRS28_BACK);
		pos.y = VillageSetting::STAIRS28_BOTTOM +
			(VillageSetting::STAIRS28_TOP - VillageSetting::STAIRS28_BOTTOM) * ratio;
		m_player->SetPosition(pos);
		return true;
	}
	return false;
}

void VillageScene::ChangeCharacter(PlayerType type, shared_ptr<Player>& player)
{
	if (g_playerInfo.playerType == type) return;
	g_playerInfo.playerType = type;

	//auto id = player->GetId();
	auto position = player->GetPosition();

	player = make_shared<Player>();
	player->SetType(g_playerInfo.playerType);
	LoadPlayerFromFile(player);

	m_shaders["ANIMATION"]->SetPlayer(player);
	player->SetPosition(position);

	m_camera->SetPlayer(player);
	player->SetCamera(m_camera);

	SetSkillSettingUI(g_playerInfo.playerType);
	SetSkillUI();

	m_player->SetSkillGauge(m_skillUI);
	m_player->SetUltimateGauge(m_ultimateUI);
	m_player->ResetAllCooldown();
}

void VillageScene::SetSkillSettingUI(PlayerType type)
{
	switch (type)
	{
	case PlayerType::WARRIOR:
		m_skill1SwitchUI->SetTexture("WARRIORSKILL1");
		m_skill1SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("스킬1"));
			m_skillInfoUI->SetText(TEXT("전사스킬1에 대한 설명입니다."));
			});
		m_skill2SwitchUI->SetTexture("WARRIORSKILL2");
		m_skill2SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("스킬2"));
			m_skillInfoUI->SetText(TEXT("전사스킬2에 대한 설명입니다.\n전사스킬2에 대한 설명입니다.\n전사스킬2에 대한 설명입니다.\n"));
			});
		m_ultimate1SwitchUI->SetTexture("WARRIORULTIMATE1");
		m_ultimate1SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("궁극기1"));
			m_skillInfoUI->SetText(TEXT("전사궁1에 대한 설명입니다.\n전사궁1에 대한 설명입니다.\n전사궁1에 대한 설명입니다.\n"));
			});
		m_ultimate2SwitchUI->SetTexture("WARRIORULTIMATE2");
		m_ultimate2SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("궁극기2"));
			m_skillInfoUI->SetText(TEXT("전사궁2에 대한 설명입니다.\n전사궁2에 대한 설명입니다.\n전사궁2에 대한 설명입니다.\n"));
			});
		break;
	case PlayerType::ARCHER:
		m_skill1SwitchUI->SetTexture("ARCHERSKILL1");
		m_skill1SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("스킬1"));
			m_skillInfoUI->SetText(TEXT("궁수스킬1에 대한 설명입니다.\n궁수스킬1에 대한 설명입니다.\n궁수스킬1에 대한 설명입니다.\n"));
			});
		m_skill2SwitchUI->SetTexture("ARCHERSKILL2");
		m_skill2SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("스킬2"));
			m_skillInfoUI->SetText(TEXT("궁수스킬2에 대한 설명입니다.\n궁수스킬2에 대한 설명입니다.\n궁수스킬2에 대한 설명입니다.\n"));
			});
		m_ultimate1SwitchUI->SetTexture("ARCHERULTIMATE1");
		m_ultimate1SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("궁극기1"));
			m_skillInfoUI->SetText(TEXT("궁수궁1에 대한 설명입니다.\n궁수궁1에 대한 설명입니다.\n궁수궁1에 대한 설명입니다.\n"));
			});
		m_ultimate2SwitchUI->SetTexture("ARCHERULTIMATE2");
		m_ultimate2SwitchUI->SetClickEvent([&] {
			m_skillNameUI->SetText(TEXT("궁극기2"));
			m_skillInfoUI->SetText(TEXT("궁수궁2에 대한 설명입니다.\n궁수궁2에 대한 설명입니다.\n궁수궁2에 대한 설명입니다.\n"));
			});
		break;
	}
}

void VillageScene::SetSkillUI()
{
	switch (g_playerInfo.playerType)
	{
	case PlayerType::WARRIOR:
		if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].first == 0) {
			m_skillUI->SetTexture("WARRIORSKILL1");
		}
		else if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].first == 1) {
			m_skillUI->SetTexture("WARRIORSKILL2");
		}
		if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].second == 0) {
			m_ultimateUI->SetTexture("WARRIORULTIMATE1");
		}
		else if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].second == 1) {
			m_ultimateUI->SetTexture("WARRIORULTIMATE2");
		}
		break;
	case PlayerType::ARCHER:
		if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].first == 0) {
			m_skillUI->SetTexture("ARCHERSKILL1");
		}
		else if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].first == 1) {
			m_skillUI->SetTexture("ARCHERSKILL2");
		}
		if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].second == 0) {
			m_ultimateUI->SetTexture("ARCHERULTIMATE1");
		}
		else if (g_playerInfo.skill[(size_t)g_playerInfo.playerType].second == 1) {
			m_ultimateUI->SetTexture("ARCHERULTIMATE2");
		}
		break;
	}
}

void VillageScene::TryEnhancement(EnhancementType type)
{
	if (g_playerInfo.gold >= PlayerSetting::ENHANCE_COST) {
		g_playerInfo.gold -= PlayerSetting::ENHANCE_COST;
		m_goldTextUI->SetText(std::to_wstring(g_playerInfo.gold));

		SendEnhancement(type);
	}
}
