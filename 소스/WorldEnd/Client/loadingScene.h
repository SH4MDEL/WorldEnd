#pragma once
#include "scene.h"
#include "framework.h"

class LoadingScene : public Scene
{
public:
	LoadingScene() = default;
	~LoadingScene() override;

	void OnCreate() override;
	void OnDestroy() override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature, FLOAT	aspectRatio) override;
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;

	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
};

