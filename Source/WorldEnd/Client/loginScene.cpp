#include "loginScene.h"

LoginScene::LoginScene() : 
	m_sceneState{ (INT)State::Unused } 
{}
LoginScene::~LoginScene() 
{ 
	//OnDestroy();
	//m_globalMeshs.clear();
	//m_globalTextures.clear();
	//m_globalMaterials.clear();
	//m_globalAnimationSets.clear();
}

void LoginScene::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
	if (m_fadeFilter) m_fadeFilter->OnResize(device, width, height);
}

void LoginScene::OnCreate(
	const ComPtr<ID3D12Device>& device, 
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature) 
{
	g_selectedPlayerType = PlayerType::WARRIOR;
	m_sceneState = (INT)State::Unused;
	BuildObjects(device, commandList, rootSignature, postRootSignature);
}

void LoginScene::OnDestroy() 
{
	m_meshs.clear();
	m_textures.clear();
	m_materials.clear();
	m_animationSets.clear();

	for (auto& shader : m_globalShaders) shader.second->Clear();

	DestroyObjects();
}

void LoginScene::ReleaseUploadBuffer(){}

void LoginScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList){}
void LoginScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList){}

void LoginScene::BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
	const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature) 
{
	CreateShaderVariable(device, commandlist);

	auto windowWidth = g_GameFramework.GetWindowWidth();
	auto windowHeight = g_GameFramework.GetWindowHeight();
	m_blurFilter = make_unique<BlurFilter>(device, windowWidth, windowHeight);
	m_fadeFilter = make_unique<FadeFilter>(device, windowWidth, windowHeight);

	BuildUI(device, commandlist);
}

void LoginScene::BuildUI(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist)
{
	m_titleUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 1.f, 1.f });
	m_titleUI->SetTexture("TITLE");
	auto gameStartButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.3f}, XMFLOAT2{0.2f, 0.08f}) };
	gameStartButtonUI->SetTexture("BUTTONUI");
	gameStartButtonUI->SetClickEvent([&]() {
		m_fadeFilter->FadeOut([&]() {
			g_GameFramework.ChangeScene(SCENETAG::TowerLoadingScene);
			});
		});
	auto gameStartButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
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
	auto optionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
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
	auto gameExitButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
	gameExitButtonTextUI->SetText(L"게임 종료");
	gameExitButtonTextUI->SetColorBrush("WHITE");
	gameExitButtonTextUI->SetTextFormat("KOPUB18");
	gameExitButtonUI->SetChild(gameExitButtonTextUI);
	m_titleUI->SetChild(gameExitButtonUI);

	m_globalShaders["UI"]->SetUI(m_titleUI);

	BuildOptionUI(device, commandlist);

	m_characterSelectTextUI = make_shared<TextUI>(XMFLOAT2{ -0.7f, 0.8f }, XMFLOAT2{ 80.f, 15.f });
	m_characterSelectTextUI->SetText(L"WARRIOR 선택 중");
	m_characterSelectTextUI->SetColorBrush("WHITE");
	m_characterSelectTextUI->SetTextFormat("KOPUB18");
	m_globalShaders["UI"]->SetUI(m_characterSelectTextUI);
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
	auto  option1080x720ResolutionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{100.f, 10.f}) };
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
	auto  option1920x1080ResolutionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{100.f, 10.f}) };
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
	auto  option2560x1440ResolutionButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{100.f, 10.f}) };
	option2560x1440ResolutionButtonTextUI->SetText(L"2560 X 1440");
	option2560x1440ResolutionButtonTextUI->SetColorBrush("WHITE");
	option2560x1440ResolutionButtonTextUI->SetTextFormat("KOPUB18");
	option2560x1440ResolutionButtonUI->SetChild(option2560x1440ResolutionButtonTextUI);
	m_optionUI->SetChild(option2560x1440ResolutionButtonUI);

	m_globalShaders["UI"]->SetUI(m_optionUI);
}

void LoginScene::DestroyObjects()
{
	m_blurFilter.reset();
	m_fadeFilter.reset();

	m_titleUI.reset();
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
		g_selectedPlayerType = PlayerType::WARRIOR;
	}
	else if (GetAsyncKeyState('2') & 0x8000) {
		m_characterSelectTextUI->SetText(L"ARCHER 선택 중");
		g_selectedPlayerType = PlayerType::ARCHER;
	}
}

void LoginScene::Update(FLOAT timeElapsed) 
{
	for (const auto& shader : m_globalShaders)
		shader.second->Update(timeElapsed);
	m_fadeFilter->Update(timeElapsed);
}

void LoginScene::PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) {}

void LoginScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const 
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
		m_globalShaders.at("UI")->Render(commandList);
		break;
	}
	}
}

void LoginScene::PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) 
{
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
		//m_shaders["COMPOSITE"]->Render(commandList);
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

void LoginScene::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) 
{
	if (!CheckState(State::OutputOptionUI)) {
		if (m_titleUI) m_titleUI->RenderText(deviceContext);
	}
	else {
		if (m_optionUI) m_optionUI->RenderText(deviceContext);
	}
	if (m_characterSelectTextUI) m_characterSelectTextUI->RenderText(deviceContext);
}

void LoginScene::PostRenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
}

bool LoginScene::CheckState(State sceneState)
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