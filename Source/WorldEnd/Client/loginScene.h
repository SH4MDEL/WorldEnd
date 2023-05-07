#pragma once
#include "scene.h"
#include "framework.h"

class LoginScene : public Scene
{
public:
	enum class State {
		Unused = 0x00,
		OutputExitUI = 0x01,
		OutputResult = 0x02,
		Fading = 0x04,
		BlurLevel1 = Unused,
		BlurLevel2 = Unused,
		BlurLevel3 = OutputExitUI,
		BlurLevel4 = Unused,
		BlurLevel5 = OutputResult,
		CantPlayerControl = OutputExitUI | OutputResult | Fading
	};

	LoginScene();
	~LoginScene() override;

	void OnCreate(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList,
		const ComPtr<ID3D12RootSignature>& rootSignature,
		const ComPtr<ID3D12RootSignature>& postRootSignature) override;

	void OnDestroy() override;
	void ReleaseUploadBuffer() override;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist,
		const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature) override;
	void DestroyObjects() override;

	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) override;
	void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) override;

	void Update(FLOAT timeElapsed) override;
	void PreProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const override;
	void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, UINT threadIndex) override;
	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;
	void PostRenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	shared_ptr<Shadow> GetShadow() override { return nullptr; }

	bool CheckState(State sceneState);
	void SetState(State sceneState);
	void ResetState(State sceneState);

private:
	INT													m_sceneState;

	unique_ptr<BlurFilter>								m_blurFilter;
	unique_ptr<FadeFilter>								m_fadeFilter;

	shared_ptr<UI>										m_titleUI;
	shared_ptr<TextUI>									m_characterSelectTextUI;
};

