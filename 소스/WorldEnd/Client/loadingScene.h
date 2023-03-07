#pragma once
#include "scene.h"
#include "framework.h"

class LoadingScene : public Scene
{
public:
	LoadingScene() = default;
	LoadingScene(const ComPtr<ID3D12Device>& device);
	~LoadingScene() override;

	void OnCreate(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& mainCommandList,
		const array<ComPtr<ID3D12GraphicsCommandList>, MAX_THREAD>& threadCommandList,
		array<thread, MAX_THREAD>& subThread,
		const ComPtr<ID3D12RootSignature>& rootSignature) override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature) override;
	void BulidObjectsByThread1(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature);
	void BulidObjectsByThread2(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature);
	void BulidObjectsByThread3(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature);
	void BulidObjectsByThread4(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, const ComPtr<ID3D12RootSignature>& rootsignature);


	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
    void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;

	void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) const override;
	void RenderShadow(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;
 
	void LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName);
	void LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName);
	void LoadAnimationSetFromFile(wstring fileName, const string& animationSetName);

private:
	shared_ptr<LoadingText>				m_loadingText;

	//thread								m_loadingThread;
	BOOL								m_canNextScene[MAX_THREAD];
	//ComPtr<ID3D12GraphicsCommandList>	m_threadCommandList;
	//ComPtr<ID3D12CommandAllocator>		m_threadCommandAllocator;
	mutex								m_materialMutex;

};

