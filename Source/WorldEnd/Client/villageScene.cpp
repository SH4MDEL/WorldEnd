#include "villageScene.h"

VillageScene::VillageScene(const ComPtr<ID3D12Device>& device, 
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
	m_player->SetType(g_selectedPlayerType);
	LoadPlayerFromFile(m_player);

	m_shaders["ANIMATION"]->SetPlayer(m_player);
	// 데이터베이스에서 플레이어의 시작 위치로 수정
	m_player->SetPosition(XMFLOAT3{25.f, 5.65f, 66.f});

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

	BuildUI(device, commandlist);

	// 필터 생성
	auto windowWidth = g_GameFramework.GetWindowWidth();
	auto windowHeight = g_GameFramework.GetWindowHeight();
	m_blurFilter = make_unique<BlurFilter>(device, windowWidth, windowHeight);
	m_fadeFilter = make_unique<FadeFilter>(device, windowWidth, windowHeight);

	// 조명 생성
	BuildLight(device, commandlist);
}

void VillageScene::BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{

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

void VillageScene::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	if (m_player) m_player->OnProcessingMouseMessage(message, lParam);
	if (!g_clickEventStack.empty()) {
		g_clickEventStack.top()();
		while (!g_clickEventStack.empty()) {
			g_clickEventStack.pop();
		}
	}
}

void VillageScene::OnProcessingKeyboardMessage(FLOAT timeElapsed)
{
	if (m_player) m_player->OnProcessingKeyboardMessage(timeElapsed);

	if (GetAsyncKeyState(VK_TAB) & 0x8000) {
		SetState(State::SceneLeave);
	}
}

void VillageScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void VillageScene::Update(FLOAT timeElapsed)
{
	if (CheckState(State::SceneLeave)) {
		g_GameFramework.ChangeScene(SCENETAG::TowerScene);
		return;
	}
	m_camera->Update(timeElapsed);
	if (m_shaders["SKYBOX"]) for (auto& skybox : m_shaders["SKYBOX"]->GetObjects()) skybox->SetPosition(m_camera->GetEye());
	for (const auto& shader : m_shaders)
		shader.second->Update(timeElapsed);
	m_fadeFilter->Update(timeElapsed);

	CollideWithMap();
	//UpdateLightSystem(timeElapsed);

	// 프러스텀 컬링을 진행하는 셰이더에 바운딩 프러스텀 전달
	auto viewFrustum = m_camera->GetViewFrustum();
	static_pointer_cast<StaticObjectShader>(m_shaders["OBJECT1"])->SetBoundingFrustum(viewFrustum);
	static_pointer_cast<StaticObjectShader>(m_shaders["OBJECT2"])->SetBoundingFrustum(viewFrustum);
	static_pointer_cast<StaticObjectBlendShader>(m_shaders["OBJECTBLEND"])->SetBoundingFrustum(viewFrustum);
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
