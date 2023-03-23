#pragma once
#include "scene.h"
#include "framework.h"

class LoadingScene : public Scene
{
public:
	LoadingScene();
	~LoadingScene() override;

	void OnCreate(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList,
		const ComPtr<ID3D12RootSignature>& rootSignature, 
		const ComPtr<ID3D12RootSignature>& postRootsignature) override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist, 
		const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootsignature) override;
	
	void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) const override;
	void OnProcessingClickMessage(LPARAM lParam) const override;
    void OnProcessingKeyboardMessage(FLOAT timeElapsed) const override;

	void Update(FLOAT timeElapsed) override;
	void RenderShadow(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) override;
	void Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT threadIndex) const override;
	void PostProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget) override;
	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

	shared_ptr<Shadow> GetShadow() override { return nullptr; }
 
	void LoadMeshFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName);
	void LoadMaterialFromFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, wstring fileName);
	void LoadAnimationSetFromFile(wstring fileName, const string& animationSetName);

private:
	shared_ptr<LoadingText>				m_loadingText;

	thread								m_loadingThread;
	ComPtr<ID3D12GraphicsCommandList>	m_threadCommandList;
	ComPtr<ID3D12CommandAllocator>		m_threadCommandAllocator;
	BOOL								m_loadEnd;
};

