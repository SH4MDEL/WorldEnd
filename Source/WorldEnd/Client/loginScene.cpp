#include "loginScene.h"

LoginScene::LoginScene(const ComPtr<ID3D12Device>& device, 
	const ComPtr<ID3D12GraphicsCommandList>& commandList, 
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature) :
	m_NDCspace(0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f),
	m_sceneState{ (INT)State::Unused }
{
	OnCreate(device, commandList, rootSignature, postRootSignature);
}

LoginScene::~LoginScene()
{ 
	//OnDestroy();
}

void LoginScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
	if (m_blurFilter) m_blurFilter->OnResize(device, width, height);
	if (m_fadeFilter) m_fadeFilter->OnResize(device, width, height);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 100.0f));
	if (m_camera) m_camera->SetProjMatrix(projMatrix);
}

void LoginScene::OnCreate(
	const ComPtr<ID3D12Device>& device, 
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature) 
{
	g_playerInfo.playerType = PlayerType::WARRIOR;

	m_sceneState = (INT)State::Unused;
	BuildObjects(device, commandList, rootSignature, postRootSignature);
}

void LoginScene::OnDestroy() 
{
	for (auto& shader : m_shaders) shader.second->Clear();

	DestroyObjects();
}

void LoginScene::ReleaseUploadBuffer(){}

void LoginScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
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

void LoginScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
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

void LoginScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature) 
{
	CreateShaderVariable(device, commandlist);

	// 카메라 생성
	m_camera = make_shared<ViewingCamera>();
	m_camera->CreateShaderVariable(device, commandlist);

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, g_GameFramework.GetAspectRatio(), 0.1f, 300.0f));
	m_camera->SetProjMatrix(projMatrix);
	m_shaders["OBJECTBLEND"]->SetCamera(m_camera);

	m_shadow = make_shared<Shadow>(device, 1024, 1024);

	// 씬 로드
	LoadSceneFromFile(TEXT("Resource/Scene/VillageScene.bin"), TEXT("VillageScene"));

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

	BuildUI(device, commandlist);

	// 필터 생성
	auto windowWidth = g_GameFramework.GetWindowWidth();
	auto windowHeight = g_GameFramework.GetWindowHeight();
	m_blurFilter = make_unique<BlurFilter>(device, windowWidth, windowHeight);
	m_fadeFilter = make_unique<FadeFilter>(device, windowWidth, windowHeight);

	// UI 생성
	BuildUI(device, commandlist);

	// 조명 생성
	BuildLight(device, commandlist);
}

void LoginScene::BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	m_titleUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 1.f, 1.f });

	auto titleUI{ make_shared<UI>(XMFLOAT2{ 0.f, 0.5f }, XMFLOAT2{ 0.25f, 0.25f }) };
	titleUI->SetTexture("TITLE");
	m_titleUI->SetChild(titleUI);

	m_idBox = make_shared<InputTextUI>(XMFLOAT2{ -0.3f, 0.f }, XMFLOAT2{ 0.2f, 0.08f }, XMFLOAT2{ 120.f, 10.f }, 20);
	m_idBox->SetColorBrush("WHITE");
	m_idBox->SetTextFormat("KOPUB18");
	m_idBox->SetTexture("BUTTONUI");
	m_titleUI->SetChild(m_idBox);

	m_passwordBox = make_shared<InputTextUI>(XMFLOAT2{ 0.3f, 0.f }, XMFLOAT2{ 0.2f, 0.08f }, XMFLOAT2{ 120.f, 10.f }, 20);
	m_passwordBox->SetColorBrush("WHITE");
	m_passwordBox->SetTextFormat("KOPUB18");
	m_passwordBox->SetTexture("BUTTONUI");
	m_titleUI->SetChild(m_passwordBox);

	auto gameStartButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.3f}, XMFLOAT2{0.2f, 0.08f}) };
	gameStartButtonUI->SetTexture("BUTTONUI");
#ifdef USE_NETWORK
	gameStartButtonUI->SetClickEvent([&]() {
		InitServer();
		TryLogin();
		});
#else
	gameStartButtonUI->SetClickEvent([&]() {
		m_fadeFilter->FadeOut([&]() {
			SetState(State::SceneLeave);
			});
	});
#endif
	auto gameStartButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
	gameStartButtonTextUI->SetText(L"게임 시작");
	gameStartButtonTextUI->SetColorBrush("WHITE");
	gameStartButtonTextUI->SetTextFormat("KOPUB18");
	gameStartButtonUI->SetChild(gameStartButtonTextUI);
	m_titleUI->SetChild(gameStartButtonUI);

	auto optionButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.5f}, XMFLOAT2{0.2f, 0.08f}) };
	optionButtonUI->SetTexture("BUTTONUI");
	optionButtonUI->SetClickEvent([&]() {
		SetState(State::OutputOptionUI);
		if (m_optionUI) m_optionUI->SetEnable();
		});
	auto optionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
	optionButtonTextUI->SetText(L"옵션");
	optionButtonTextUI->SetColorBrush("WHITE");
	optionButtonTextUI->SetTextFormat("KOPUB18");
	optionButtonUI->SetChild(optionButtonTextUI);
	m_titleUI->SetChild(optionButtonUI);

	auto gameExitButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.7f}, XMFLOAT2{0.2f, 0.08f}) };
	gameExitButtonUI->SetTexture("BUTTONUI");
	gameExitButtonUI->SetClickEvent([&]() {
		m_fadeFilter->FadeOut([&]() {
			PostMessage(NULL, WM_QUIT, 0, 0);
			});
		});
	auto gameExitButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
	gameExitButtonTextUI->SetText(L"게임 종료");
	gameExitButtonTextUI->SetColorBrush("WHITE");
	gameExitButtonTextUI->SetTextFormat("KOPUB18");
	gameExitButtonUI->SetChild(gameExitButtonTextUI);
	m_titleUI->SetChild(gameExitButtonUI);

	m_shaders["POSTUI"]->SetUI(m_titleUI);

	BuildOptionUI(device, commandlist);

	m_characterSelectTextUI = make_shared<TextUI>(XMFLOAT2{ -0.7f, 0.8f }, XMFLOAT2{ 0.f, 0.f },  XMFLOAT2{ 80.f, 15.f });
	m_characterSelectTextUI->SetText(L"WARRIOR 선택 중");
	m_characterSelectTextUI->SetColorBrush("WHITE");
	m_characterSelectTextUI->SetTextFormat("KOPUB18");
	m_shaders["POSTUI"]->SetUI(m_characterSelectTextUI);
}

void LoginScene::BuildOptionUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	m_optionUI = make_shared<StandardUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 0.5f, 0.7f });
	m_optionUI->SetTexture("FRAMEUI");
	m_optionUI->SetDisable();

	auto optionCancelButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.9f, 0.9f}, XMFLOAT2{0.06f, 0.06f}) };
	optionCancelButtonUI->SetTexture("CANCELUI");
	optionCancelButtonUI->SetClickEvent([&]() {
		ResetState(State::OutputOptionUI);
		if (m_optionUI) m_optionUI->SetDisable();
	});
	m_optionUI->SetChild(optionCancelButtonUI);

	auto option1080x720ResolutionButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, 0.3f}, XMFLOAT2{0.2f, 0.08f}) };
	option1080x720ResolutionButtonUI->SetTexture("BUTTONUI");
	option1080x720ResolutionButtonUI->SetClickEvent([&]() {
		g_GameFramework.ResizeWindow(1080, 720);
		});
	auto  option1080x720ResolutionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f}, XMFLOAT2{100.f, 10.f}) };
	option1080x720ResolutionButtonTextUI->SetText(L"1080 X 720");
	option1080x720ResolutionButtonTextUI->SetColorBrush("WHITE");
	option1080x720ResolutionButtonTextUI->SetTextFormat("KOPUB18");
	option1080x720ResolutionButtonUI->SetChild(option1080x720ResolutionButtonTextUI);
	m_optionUI->SetChild(option1080x720ResolutionButtonUI);

	auto option1920x1080ResolutionButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.2f, 0.08f}) };
	option1920x1080ResolutionButtonUI->SetTexture("BUTTONUI");
	option1920x1080ResolutionButtonUI->SetClickEvent([&]() {
		g_GameFramework.ResizeWindow(1920, 1080);
	});
	auto  option1920x1080ResolutionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f}, XMFLOAT2{100.f, 10.f}) };
	option1920x1080ResolutionButtonTextUI->SetText(L"1920 X 1080");
	option1920x1080ResolutionButtonTextUI->SetColorBrush("WHITE");
	option1920x1080ResolutionButtonTextUI->SetTextFormat("KOPUB18");
	option1920x1080ResolutionButtonUI->SetChild(option1920x1080ResolutionButtonTextUI);
	m_optionUI->SetChild(option1920x1080ResolutionButtonUI);

	auto option2560x1440ResolutionButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.3f}, XMFLOAT2{0.2f, 0.08f}) };
	option2560x1440ResolutionButtonUI->SetTexture("BUTTONUI");
	option2560x1440ResolutionButtonUI->SetClickEvent([&]() {
		g_GameFramework.ResizeWindow(2560, 1440);
		});
	auto  option2560x1440ResolutionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{0.f, 0.f}, XMFLOAT2{100.f, 10.f}) };
	option2560x1440ResolutionButtonTextUI->SetText(L"2560 X 1440");
	option2560x1440ResolutionButtonTextUI->SetColorBrush("WHITE");
	option2560x1440ResolutionButtonTextUI->SetTextFormat("KOPUB18");
	option2560x1440ResolutionButtonUI->SetChild(option2560x1440ResolutionButtonTextUI);
	m_optionUI->SetChild(option2560x1440ResolutionButtonUI);

	m_shaders["POSTUI"]->SetUI(m_optionUI);
}

void LoginScene::BuildLight(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
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

void LoginScene::DestroyObjects()
{
	m_camera.reset();
	m_terrain.reset();

	m_lightSystem.reset();
	m_shadow.reset();

	m_blurFilter.reset();
	m_fadeFilter.reset();

	m_titleUI.reset();
	m_optionUI.reset();
	m_characterSelectTextUI.reset();
}

void LoginScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) 
{
	if (m_titleUI) m_titleUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	if (m_optionUI) m_optionUI->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
}

void LoginScene::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	if (m_titleUI) m_titleUI->OnProcessingMouseMessage(message, lParam);
	if (m_optionUI) m_optionUI->OnProcessingMouseMessage(message, lParam);
	if (!g_clickEventStack.empty()) {
		g_clickEventStack.top()();
		while (!g_clickEventStack.empty()) {
			g_clickEventStack.pop();
		}
	}
}

void LoginScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) 
{
	if (GetAsyncKeyState('1') & 0x8000) {
		m_characterSelectTextUI->SetText(L"WARRIOR 선택 중");
		g_playerInfo.playerType = PlayerType::WARRIOR;
	}
	else if (GetAsyncKeyState('2') & 0x8000) {
		m_characterSelectTextUI->SetText(L"ARCHER 선택 중");
		g_playerInfo.playerType = PlayerType::ARCHER;
	}
}

void LoginScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_titleUI) m_titleUI->OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
}

void LoginScene::Update(FLOAT timeElapsed) 
{
#ifdef USE_NETWORK
	if (CheckState(State::SceneLeave)) {
		g_GameFramework.ChangeScene(SCENETAG::VillageScene);
		return;
	}
	RecvPacket();
#else
	if (CheckState(State::SceneLeave)) {
		g_GameFramework.ChangeScene(SCENETAG::VillageScene);
		return;
	}
#endif

	
	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shaders)
		shader.second->Update(timeElapsed);
	m_fadeFilter->Update(timeElapsed);

	// 프러스텀 컬링을 진행하는 셰이더에 바운딩 프러스텀 전달
	auto viewFrustum = m_camera->GetViewFrustum();
	static_pointer_cast<StaticObjectShader>(m_shaders["OBJECT1"])->SetBoundingFrustum(viewFrustum);
	static_pointer_cast<StaticObjectShader>(m_shaders["OBJECT2"])->SetBoundingFrustum(viewFrustum);
	static_pointer_cast<StaticObjectBlendShader>(m_shaders["OBJECTBLEND"])->SetBoundingFrustum(viewFrustum);

}

void LoginScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) 
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
		break;
	}
	}
}

void LoginScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const 
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
		m_shaders["WIREFRAME"]->Render(commandList);
		m_shaders.at("UI")->Render(commandList);
		break;
	}
	}
}

void LoginScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) 
{
	if (CheckState(State::SceneLeave)) return;
	switch (threadIndex)
	{
	case 0:
	{
		m_blurFilter->Execute(commandList, renderTarget, 5);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
		commandList->CopyResource(renderTarget.Get(), m_blurFilter->GetBlurMap().Get());
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
		m_blurFilter->ResetResourceBarrier(commandList);

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

void LoginScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) 
{
	if (CheckState(State::SceneLeave)) return;
}

void LoginScene::PostRenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	if (CheckState(State::SceneLeave)) return;
	if (!CheckState(State::OutputOptionUI)) {
		if (m_titleUI) m_titleUI->RenderText(deviceContext);
	}
	else {
		if (m_optionUI) m_optionUI->RenderText(deviceContext);
	}
	if (m_characterSelectTextUI) m_characterSelectTextUI->RenderText(deviceContext);
}

bool LoginScene::CheckState(State sceneState) const
{
	return m_sceneState & (INT)sceneState;
}

void LoginScene::SetState(State sceneState)
{
	m_sceneState |= (INT)sceneState;
}

void LoginScene::ResetState(State sceneState)
{
	m_sceneState &= ~(INT)sceneState;
}

void LoginScene::LoadSceneFromFile(wstring fileName, wstring sceneName)
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

void LoginScene::LoadObjectFromFile(wstring fileName, const shared_ptr<GameObject>& object)
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

bool LoginScene::IsBlendObject(const string& objectName)
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

void LoginScene::InitServer()
{
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
}

void LoginScene::ProcessPacket(char* ptr)
{
	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
		RecvLoginOk(ptr);
		break;
	case SC_PACKET_LOGIN_FAIL:
		RecvLoginFail(ptr);
		break;
	default:
		cout << "UnDefined Packet!!" << endl;
		break;
	}
}

bool LoginScene::TryLogin()
{
#ifdef USE_NETWORK
	CS_LOGIN_PACKET login_packet{};
	login_packet.size = sizeof(login_packet);
	login_packet.type = CS_PACKET_LOGIN;
	login_packet.id = m_idBox->GetString();
	login_packet.password = m_passwordBox->GetString();
	send(g_socket, reinterpret_cast<char*>(&login_packet), sizeof(login_packet), NULL);
#endif
}

void LoginScene::RecvLoginOk(char* ptr)
{
	SC_LOGIN_OK_PACKET* packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(ptr);
	g_playerInfo.id = packet->id;
	g_playerInfo.x = packet->pos.x;
	g_playerInfo.y = packet->pos.y;
	g_playerInfo.z = packet->pos.z;
	g_playerInfo.playerType = packet->player_type;

	m_fadeFilter->FadeOut([&]() {
		SetState(State::SceneLeave);
		});
	cout << "OK" << endl;
}

void LoginScene::RecvLoginFail(char* ptr)
{
	SC_LOGIN_FAIL_PACKET* packet = reinterpret_cast<SC_LOGIN_FAIL_PACKET*>(ptr);

	cout << "FAIL" << endl;
	// 로그인 실패 창 띄우기
}


