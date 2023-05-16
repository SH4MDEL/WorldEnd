#pragma once
#include "stdafx.h"
#include "timer.h"
#include "camera.h"
#include "object.h"
#include "scene.h"
#include "globalLoadingScene.h"
#include "villageLoadingScene.h"
#include "loginScene.h"
#include "towerLoadingScene.h"
#include "towerScene.h"

struct FrameworkInfo
{
	FLOAT		timeElapsed;
};

class GameFramework
{
public:
	GameFramework();
	GameFramework(UINT width, UINT height);
	~GameFramework();

	void OnResize(HWND hWnd);

	void OnCreate(HINSTANCE hInstance, HWND hWnd);
	void OnDestroy();
	void OnProcessingMouseMessage() const;
	void OnProcessingMouseMessage(UINT message, LPARAM lParam) const;
	void OnProcessingKeyboardMessage() const;
	void CreatePipeline();
	
	// 1. 디바이스 생성
	void CreateDevice();

	// 2. 펜스 객체 생성
	void CreateFence();

	// 3. 4X MSAA 품질 수준 지원 여부 점검
	void Check4xMSAAMultiSampleQuality();

	// 4. 명령 큐, 명령 할당자, 명령 리스트 생성
	void CreateMainCommandQueueAndList();

	// 5. 멀티쓰레드 렌더링 전용 명령 리스트 생성
	void CreateThreadCommandList();

	// 6. 스왑 체인 생성
	void CreateSwapChain();

	// 6. 서술자 힙 생성
	void CreateRtvSrvDsvDescriptorHeap();

	// 7. 후면 버퍼에 대한 렌더 타겟 뷰 생성
	void CreateRenderTargetView();

	// 8. 깊이 스텐실 버퍼, 깊이 스텐실 뷰 생성
	void CreateDepthStencilView();

	// 9. 루트 시그니처 생성
	void CreateRootSignature();

	// 10. 후처리 루트 시그니처 생성
	void CreatePostRootSignature();

	void Create11On12Device();

	void CreateD2DDevice();

	void CreateD2DRenderTarget();


	void CreateShaderVariable();
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void BuildObjects();
	void CreateThread();
	void ChangeScene(SCENETAG tag);
	void ResizeWindow(UINT width, UINT height);

	void FrameAdvance();
	void Update(FLOAT timeElapsed);
	void Render();
	void BeginFrame();
	void MidFrame();
	void PostFrame();
	void EndFrame();
	void WorkerThread(UINT threadIndex);
	void RenderText();
	void PostRenderText();

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
	static const INT											SwapChainBufferCount = 2;

	// Window
	HINSTANCE													m_hInstance;
	HWND														m_hWnd;
	UINT														m_width;
	UINT														m_height;
	FLOAT														m_aspectRatio;
	BOOL														m_isActive;
	BOOL														m_isGameEnd;

	D3D12_VIEWPORT												m_viewport;
	D3D12_RECT													m_scissorRect;
	ComPtr<IDXGIFactory4>										m_factory;
	ComPtr<IDXGISwapChain3>										m_swapChain;
	ComPtr<ID3D12Device>										m_device;
	INT															m_MSAA4xQualityLevel;

	ComPtr<ID3D12CommandQueue>									m_commandQueue;
	ComPtr<ID3D12CommandAllocator>								m_mainCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>							m_mainCommandList;

	// For Multithread Rendering
	array<ID3D12CommandList*, THREAD_NUM * 3 + COMMANDLIST_NUM>	m_batchSubmit;
	array<thread, THREAD_NUM>									m_thread;
	array<HANDLE, THREAD_NUM>									m_beginRender;
	array<HANDLE, THREAD_NUM>									m_finishShadowPass;
	array<HANDLE, THREAD_NUM>									m_finishRender;
	array<HANDLE, THREAD_NUM>									m_finishPostProcess;
	array<ComPtr<ID3D12CommandAllocator>, COMMANDLIST_NUM>		m_commandAllocators;
	array<ComPtr<ID3D12GraphicsCommandList>, COMMANDLIST_NUM>	m_commandLists;
	array<ComPtr<ID3D12CommandAllocator>, THREAD_NUM>			m_shadowCommandAllocators;
	array<ComPtr<ID3D12GraphicsCommandList>, THREAD_NUM>		m_shadowCommandLists;
	array<ComPtr<ID3D12CommandAllocator>, THREAD_NUM>			m_sceneCommandAllocators;
	array<ComPtr<ID3D12GraphicsCommandList>, THREAD_NUM>		m_sceneCommandLists;
	array<ComPtr<ID3D12CommandAllocator>, THREAD_NUM>			m_postCommandAllocators;
	array<ComPtr<ID3D12GraphicsCommandList>, THREAD_NUM>		m_postCommandLists;

	ComPtr<ID3D12Resource>										m_renderTargets[SwapChainBufferCount];
	ComPtr<ID3D12DescriptorHeap>								m_rtvHeap;
	UINT														m_rtvDescriptorSize;
	ComPtr<ID3D12Resource>										m_depthStencil;
	ComPtr<ID3D12DescriptorHeap>								m_dsvHeap;
	ComPtr<ID3D12RootSignature>									m_rootSignature;
	ComPtr<ID3D12RootSignature>									m_postRootSignature;

	// Text Write (UI Layer)
	ComPtr<ID3D11DeviceContext>									m_deviceContext;
	ComPtr<ID3D11On12Device>									m_11On12Device;
	ComPtr<IDWriteFactory5>										m_writeFactory;
	ComPtr<ID2D1Factory3>										m_d2dFactory;
	ComPtr<ID2D1Device2>										m_d2dDevice;
	ComPtr<ID2D1DeviceContext2>									m_d2dDeviceContext;
	ComPtr<ID3D11Resource>										m_d3d11WrappedRenderTarget[SwapChainBufferCount];
	ComPtr<ID2D1Bitmap1>										m_d2dRenderTarget[SwapChainBufferCount];

	ComPtr<ID3D12Resource>										m_frameworkBuffer;
	FrameworkInfo*												m_frameworkBufferPointer;

	ComPtr<ID3D12Fence>											m_fence;
	UINT														m_frameIndex;
	UINT64														m_fenceValue;
	HANDLE														m_fenceEvent;

	vector<unique_ptr<Scene>>									m_scenes;
	INT															m_sceneIndex;
};

