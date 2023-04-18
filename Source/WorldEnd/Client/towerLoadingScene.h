#pragma once
#include "scene.h"
#include "framework.h"

class TowerLoadingScene : public LoadingScene
{
public:
	TowerLoadingScene();
	~TowerLoadingScene() override;

	void OnCreate(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList,
		const ComPtr<ID3D12RootSignature>& rootSignature,
		const ComPtr<ID3D12RootSignature>& postRootSignature) override;
	void OnDestroy() override;

	void ReleaseUploadBuffer() override;

	void BuildObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandlist,
		const ComPtr<ID3D12RootSignature>& rootsignature, const ComPtr<ID3D12RootSignature>& postRootSignature) override;
	void DestroyObjects() override;

	void Update(FLOAT timeElapsed) override;

	void RenderText(const ComPtr< ID2D1DeviceContext2>& deviceContext) override;

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


