#include "loginScene.h"

LoginScene::LoginScene() : 
	m_sceneState{ (INT)State::Unused } 
{}
LoginScene::~LoginScene() {}

void LoginScene::OnCreate(
	const ComPtr<ID3D12Device>& device, 
	const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const ComPtr<ID3D12RootSignature>& rootSignature, 
	const ComPtr<ID3D12RootSignature>& postRootSignature) 
{
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

	m_titleUI = make_shared<BackgroundUI>(XMFLOAT2{ 0.f, 0.f }, XMFLOAT2{ 1.f, 1.f });
	m_titleUI->SetTexture("TITLE");
	auto gameStartButtonUI{ make_shared<ButtonUI>(XMFLOAT2{0.f, -0.7f}, XMFLOAT2{0.15f, 0.075f}) };
	gameStartButtonUI->SetTexture("BUTTONUI");
	gameStartButtonUI->SetClickEvent([&]() {
		m_fadeFilter->FadeOut([&]() {
				g_GameFramework.ChangeScene(SCENETAG::TowerLoadingScene);
			});
		});
	auto gameStartButtonTextUI{ make_shared<TextUI>(XMFLOAT2{0.f, 0.f}, XMFLOAT2{40.f, 10.f}) };
	gameStartButtonTextUI->SetText(L"게임 시작");
	gameStartButtonUI->SetChild(gameStartButtonTextUI);
	m_titleUI->SetChild(gameStartButtonUI);
	m_globalShaders["UI"]->SetUI(m_titleUI);
}

void LoginScene::DestroyObjects()
{
	m_blurFilter.reset();
	m_fadeFilter.reset();

	m_titleUI.reset();
}

void LoginScene::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) {}

void LoginScene::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	if (m_titleUI) m_titleUI->OnProcessingMouseMessage(message, lParam);
	if (!g_clickEventStack.empty()) {
		g_clickEventStack.top()();
		while (!g_clickEventStack.empty()) {
			g_clickEventStack.pop();
		}
	}
}

void LoginScene::OnProcessingKeyboardMessage(FLOAT timeElapsed) {}

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
	if (m_titleUI) m_titleUI->RenderText(deviceContext);
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