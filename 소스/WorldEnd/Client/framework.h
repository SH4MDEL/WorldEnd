#pragma once
#include "stdafx.h"
#include "timer.h"
#include "camera.h"
#include "object.h"
#include "scene.h"
#include "loadingScene.h"
#include "towerScene.h"

class GameFramework
{
public:
	GameFramework();
	GameFramework(UINT width, UINT height);
	~GameFramework();

	void OnCreate(HINSTANCE hInstance, HWND hWnd);
	void OnDestroy();
	void OnProcessingMouseMessage() const;
	void OnProcessingKeyboardMessage() const;
	void StartPipeline();
	
	// 1. 디바이스 생성
	void CreateDevice();

	// 2. 펜스 객체 생성
	void CreateFence();

	// 3. 4X MSAA 품질 수준 지원 여부 점검
	void Check4xMSAAMultiSampleQuality();

	// 4. 명령 큐, 명령 할당자, 명령 리스트 생성
	void CreateMainCommandQueueAndList();
	void CreateThreadCommandList();
	// 5. 스왑 체인 생성
	void CreateSwapChain();

	// 6. 서술자 힙 생성
	void CreateRtvDsvDescriptorHeap();

	// 7. 후면 버퍼에 대한 렌더 타겟 뷰 생성
	void CreateRenderTargetView();

	// 8. 깊이 스텐실 버퍼, 깊이 스텐실 뷰 생성
	void CreateDepthStencilView();

	// 9. 루트 시그니처 생성
	void CreateRootSignature();

	void Create11On12Device();

	void CreateD2DDevice();

	void CreateD2DRenderTarget();


	void BuildObjects();
	void ChangeScene(SCENETAG tag);

	void FrameAdvance();
	void Update(FLOAT timeElapsed);
	void Render();
	void RenderText();

	void WaitForPreviousFrame();
	void WaitForGpu();

	UINT GetWindowWidth() const { return m_width; }
	UINT GetWindowHeight() const { return m_height; }
	FLOAT GetAspectRatio() const { return m_aspectRatio; }
	void SetIsActive(BOOL isActive) { m_isActive = isActive; }

	ComPtr<ID3D12Device> GetDevice() const { return m_device; }
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return m_commandQueue; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return m_mainCommandList; }
	ComPtr<ID2D1DeviceContext2> GetD2DDeviceContext() const { return m_d2dDeviceContext; }
	ComPtr<IDWriteFactory> GetWriteFactory() const { return m_writeFactory; }

private:
	static const INT										SwapChainBufferCount = 2;

	// Window
	HINSTANCE												m_hInstance;
	HWND													m_hWnd;
	UINT													m_width;
	UINT													m_height;
	FLOAT													m_aspectRatio;
	BOOL													m_isActive;

	D3D12_VIEWPORT											m_viewport;
	D3D12_RECT												m_scissorRect;
	ComPtr<IDXGIFactory4>									m_factory;
	ComPtr<IDXGISwapChain3>									m_swapChain;
	ComPtr<ID3D12Device>									m_device;
	INT														m_MSAA4xQualityLevel;

	ComPtr<ID3D12CommandQueue>								m_commandQueue;
	ComPtr<ID3D12CommandAllocator>							m_mainCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>						m_mainCommandList;
	array<thread, MAX_THREAD>								m_thread;
	array<ComPtr<ID3D12CommandAllocator>, MAX_THREAD>		m_threadCommandAllocator;
	array<ComPtr<ID3D12GraphicsCommandList>, MAX_THREAD>	m_threadCommandList;

	ComPtr<ID3D12Resource>									m_renderTargets[SwapChainBufferCount];
	ComPtr<ID3D12DescriptorHeap>							m_rtvHeap;
	UINT													m_rtvDescriptorSize;
	ComPtr<ID3D12Resource>									m_depthStencil;
	ComPtr<ID3D12DescriptorHeap>							m_dsvHeap;
	ComPtr<ID3D12RootSignature>								m_rootSignature;

	// Text Write (UI Layer)
	ComPtr<ID3D11DeviceContext>								m_deviceContext;
	ComPtr<ID3D11On12Device>								m_11On12Device;
	ComPtr<IDWriteFactory>									m_writeFactory;
	ComPtr<ID2D1Factory3>									m_d2dFactory;
	ComPtr<ID2D1Device2>									m_d2dDevice;
	ComPtr<ID2D1DeviceContext2>								m_d2dDeviceContext;
	ComPtr<ID3D11Resource>									m_d3d11WrappedRenderTarget[SwapChainBufferCount];
	ComPtr<ID2D1Bitmap1>									m_d2dRenderTarget[SwapChainBufferCount];

	ComPtr<ID3D12Fence>										m_fence;
	UINT													m_frameIndex;
	UINT64													m_fenceValue;
	HANDLE													m_fenceEvent;

	Timer													m_timer;

	vector<unique_ptr<Scene>>								m_scenes;
	INT														m_sceneIndex;
};

