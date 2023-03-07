﻿#include "framework.h"

GameFramework::GameFramework()
{

}

GameFramework::GameFramework(UINT width, UINT height) :
	m_width(width), 
	m_height(height), 
	m_frameIndex{0},
	m_viewport{0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f},
	m_scissorRect{0, 0, (LONG)width, (LONG)height}, 
	m_rtvDescriptorSize {0}, 
	m_sceneIndex{static_cast<int>(SCENETAG::LoadingScene)}
{
	m_aspectRatio = (FLOAT)width / (FLOAT)height;
	m_scenes.resize(static_cast<int>(SCENETAG::Count));
}

GameFramework::~GameFramework()
{
}

void GameFramework::OnCreate(HINSTANCE hInstance, HWND hWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hWnd;

	StartPipeline();
	BuildObjects(); 
}

void GameFramework::OnDestroy()
{
	WaitForPreviousFrame();

	::CloseHandle(m_fenceEvent);
}

void GameFramework::OnProcessingMouseMessage() const
{
	if (m_scenes[m_sceneIndex]) m_scenes[m_sceneIndex]->OnProcessingMouseMessage(m_hWnd, m_width, m_height, m_timer.GetDeltaTime());
}

void GameFramework::OnProcessingKeyboardMessage() const
{
	if (m_scenes[m_sceneIndex]) m_scenes[m_sceneIndex]->OnProcessingKeyboardMessage(m_timer.GetDeltaTime());
}

void GameFramework::StartPipeline()
{
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> DebugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
	{
		DebugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	ComPtr<IDXGIFactory4> m_factory;
	DX::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	// 1. 디바이스 생성
	CreateDevice();

	// 2. 펜스 객체 생성
	CreateFence();

	// 3. 4X MSAA 품질 수준 지원 여부 점검
	Check4xMSAAMultiSampleQuality();

	// 4. 명령 큐, 명령 할당자, 명령 리스트 생성
	CreateMainCommandQueueAndList();
	CreateThreadCommandList();

	// 5. 스왑 체인 생성
	CreateSwapChain();

	// 6. 서술자 힙 생성
	CreateRtvDsvDescriptorHeap();

	// 7. 후면 버퍼에 대한 렌더 타겟 뷰 생성
	CreateRenderTargetView();

	// 8. 깊이 스텐실 버퍼, 깊이 스텐실 뷰 생성
	CreateDepthStencilView();

	// 9. 루트 시그니처 생성
	CreateRootSignature();

	Create11On12Device();

	CreateD2DDevice();

	CreateD2DRenderTarget();
}

void GameFramework::CreateDevice()
{
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));

	// 하드웨어 어댑터를 나타내는 장치를 생성해 본다.
	// 최소 기능 수준은 D3D_FEATURE_LEVEL_12_0 이다.
	ComPtr<IDXGIAdapter1> Adapter;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(i, &Adapter); ++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		Adapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)))) break;
	}

	// 실패했다면 WARP 어댑터를 나타내는 장치를 생성한다.
	if (!m_device)
	{
		m_factory->EnumWarpAdapter(IID_PPV_ARGS(&Adapter));
		DX::ThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
	}
}

void GameFramework::CreateFence()
{
	// CPU와 GPU의 동기화를 위한 Fence 객체를 생성한다.
	DX::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	m_fenceValue = 1;
}

void GameFramework::Check4xMSAAMultiSampleQuality()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	DX::ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
	m_MSAA4xQualityLevel = msQualityLevels.NumQualityLevels;

	assert(m_MSAA4xQualityLevel > 0 && "Unexpected MSAA Quality Level..");
}

void GameFramework::CreateMainCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// 명령 큐 생성
	DX::ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
	// 명령 할당자 생성
	DX::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_mainCommandAllocator)));
	// 명령 리스트 생성
	DX::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_mainCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_mainCommandList)));
	// Reset을 호출하기 때문에 Close 상태로 시작
	DX::ThrowIfFailed(m_mainCommandList->Close());
}

void GameFramework::CreateThreadCommandList()
{
	for (int i = 0; i < MAX_THREAD; ++i) {
		DX::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_threadCommandAllocator[i])));
		// 명령 리스트 생성
		DX::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_threadCommandAllocator[i].Get(), nullptr, IID_PPV_ARGS(&m_threadCommandList[i])));
		// Reset을 호출하기 때문에 Close 상태로 시작
		DX::ThrowIfFailed(m_threadCommandList[i]->Close());
	}
}

void GameFramework::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.Width = m_width;
	sd.BufferDesc.Height = m_height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = m_MSAA4xQualityLevel > 1 ? 4 : 1;
	sd.SampleDesc.Quality = m_MSAA4xQualityLevel > 1 ? m_MSAA4xQualityLevel - 1 : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = m_hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain> swapChain;
	DX::ThrowIfFailed(m_factory->CreateSwapChain(m_commandQueue.Get(), &sd, &swapChain));
	DX::ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void GameFramework::CreateRtvDsvDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}

void GameFramework::CreateRenderTargetView()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart() };
	for (UINT i = 0; i < SwapChainBufferCount; ++i)
	{
		// 스왑 체인의 i번째 버퍼를 얻는다.
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));

		// 그 버퍼에 대한 렌더 타겟 뷰를 생성한다.
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), NULL, rtvHeapHandle);

		// 힙의 다음 항목으로 넘어간다.
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}
}

void GameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC depthStencilDesc{};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_width;
	depthStencilDesc.Height = m_height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = m_MSAA4xQualityLevel > 1 ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m_MSAA4xQualityLevel > 1 ? m_MSAA4xQualityLevel - 1 : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear{};
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	DX::ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(m_depthStencil.GetAddressOf())));

	// D3D12_DEPTH_STENCIL_VIEW_DESC 구조체는 깊이 스텐실 뷰를 서술하는데, 
	// 자원에 담긴 원소들의 자료 형식에 관한 멤버를 가지고 있다.
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilViewDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void GameFramework::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE descriptorRange[11];
	descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t0
	descriptorRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t1
	descriptorRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t2
	descriptorRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t3

	descriptorRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t4
	descriptorRange[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t5
	descriptorRange[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t6
	descriptorRange[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t7
	descriptorRange[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t8
	descriptorRange[9].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t9
	descriptorRange[10].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);	// t10

	CD3DX12_ROOT_PARAMETER rootParameter[18];

	// cbGameObject : 월드 변환 행렬(16) + float hp(1) + float maxHp(1)
	rootParameter[0].InitAsConstants(18, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

	rootParameter[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // cbCamera
	rootParameter[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL); // cbMaterial
	rootParameter[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL); // cbLight
	rootParameter[4].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL); // cbScene

	rootParameter[5].InitAsDescriptorTable(1, &descriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[6].InitAsDescriptorTable(1, &descriptorRange[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[7].InitAsDescriptorTable(1, &descriptorRange[2], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[8].InitAsDescriptorTable(1, &descriptorRange[3], D3D12_SHADER_VISIBILITY_PIXEL);

	rootParameter[9].InitAsDescriptorTable(1, &descriptorRange[4], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[10].InitAsDescriptorTable(1, &descriptorRange[5], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[11].InitAsDescriptorTable(1, &descriptorRange[6], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[12].InitAsDescriptorTable(1, &descriptorRange[7], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[13].InitAsDescriptorTable(1, &descriptorRange[8], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[14].InitAsDescriptorTable(1, &descriptorRange[9], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[15].InitAsDescriptorTable(1, &descriptorRange[10], D3D12_SHADER_VISIBILITY_PIXEL);
	
	rootParameter[16].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_VERTEX);		// 뼈대 오프셋
	rootParameter[17].InitAsConstantBufferView(6, 0, D3D12_SHADER_VISIBILITY_VERTEX);		// 뼈대 변환행렬


	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2];
	samplerDesc[0].Init(									// s0
		0,								 					// ShaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, 					// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 					// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 					// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 					// addressW
		0.0f,												// mipLODBias
		1,													// maxAnisotropy
		D3D12_COMPARISON_FUNC_ALWAYS,						// comparisonFunc
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,		// borderColor
		0.0f,												// minLOD
		D3D12_FLOAT32_MAX,									// maxLOD
		D3D12_SHADER_VISIBILITY_PIXEL,						// shaderVisibility
		0													// registerSpace
	);

	samplerDesc[1].Init(									// s1
		1,								 					// ShaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, 	// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 					// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 					// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 					// addressW
		0.0f,												// mipLODBias
		16,													// maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,					// comparisonFunc
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK			// borderColor
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameter), rootParameter, _countof(samplerDesc), samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature, error;
	DX::ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DX::ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void GameFramework::Create11On12Device()
{
	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG) || defined(DBG)
	d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ComPtr<ID3D11Device> d3d11Device;
	DX::ThrowIfFailed(D3D11On12CreateDevice(m_device.Get(),
		d3d11DeviceFlags, 
		nullptr, 
		0, 
		reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()),
		1,
		0, 
		&d3d11Device,
		&m_deviceContext,
		nullptr));

	DX::ThrowIfFailed(d3d11Device.As(& m_11On12Device));
}

void GameFramework::CreateD2DDevice()
{
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

#if defined(_DEBUG) || defined(DBG)
	d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	ComPtr<IDXGIDevice> dxgiDevice;
	m_11On12Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

	DX::ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory3), 
		&d2dFactoryOptions, 
		&m_d2dFactory));

	DX::ThrowIfFailed(m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice));
	DX::ThrowIfFailed(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, (ID2D1DeviceContext2**)&m_d2dDeviceContext));
	m_d2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

	DX::ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_writeFactory));
}

void GameFramework::CreateD2DRenderTarget()
{
	D2D1_BITMAP_PROPERTIES1 d2dBitmapPropertie = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, 
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

	for (UINT i = 0; i < SwapChainBufferCount; ++i)
	{
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };

		DX::ThrowIfFailed(m_11On12Device->CreateWrappedResource(m_renderTargets[i].Get(),
			&d3d11Flags, 
			D3D12_RESOURCE_STATE_RENDER_TARGET, 
			D3D12_RESOURCE_STATE_PRESENT, 
			IID_PPV_ARGS(&m_d3d11WrappedRenderTarget[i])));
		ComPtr<IDXGISurface> dxgiSurface;
		DX::ThrowIfFailed(m_d3d11WrappedRenderTarget[i]->QueryInterface(__uuidof(IDXGISurface), (void**)&dxgiSurface));
		DX::ThrowIfFailed(m_d2dDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &d2dBitmapPropertie, &m_d2dRenderTarget[i]));
	}
}

void GameFramework::BuildObjects()
{
	m_mainCommandList->Reset(m_mainCommandAllocator.Get(), nullptr);

	m_scenes[static_cast<int>(SCENETAG::LoadingScene)] = make_unique<LoadingScene>(m_device);
	m_scenes[static_cast<int>(SCENETAG::TowerScene)] = make_unique<TowerScene>();
	//m_scenes[static_cast<int>(SCENETAG::LoadingScene)]->BuildObjects(m_device, m_commandList, m_rootSignature);
	//m_scenes[static_cast<int>(SCENETAG::TowerScene)]->BuildObjects(m_device, m_commandList, m_rootSignature);

	for (int i = 0; i < MAX_THREAD; ++i) {
		m_threadCommandList[i]->Reset(m_threadCommandAllocator[i].Get(), nullptr);
	}

	m_scenes[m_sceneIndex]->OnCreate(m_device, m_mainCommandList, m_threadCommandList, m_thread, m_rootSignature);

	m_mainCommandList->Close();
	ID3D12CommandList* ppCommandList[] = { m_mainCommandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	WaitForPreviousFrame();

	for (const auto& scene : m_scenes) scene->ReleaseUploadBuffer();

	m_timer.Tick();
}

void GameFramework::ChangeScene(SCENETAG tag)
{
	WaitForGpu();

	m_mainCommandList->Reset(m_mainCommandAllocator.Get(), nullptr);

	m_scenes[m_sceneIndex]->OnDestroy();
	m_scenes[m_sceneIndex = static_cast<int>(tag)]->OnCreate(m_device, m_mainCommandList, m_threadCommandList, m_thread, m_rootSignature);

	m_mainCommandList->Close();
	ID3D12CommandList* ppCommandList[] = { m_mainCommandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	WaitForPreviousFrame();
}

void GameFramework::FrameAdvance()
{
	m_timer.Tick();

	if (m_isActive)
	{
		OnProcessingMouseMessage();
		OnProcessingKeyboardMessage();
	}
	Update(m_timer.GetDeltaTime());
	Render();

}

void GameFramework::Update(FLOAT timeElapsed)
{
	wstring title{ TEXT("세상끝 (") + to_wstring((int)(m_timer.GetFPS())) + TEXT("FPS)") };
	SetWindowText(m_hWnd, title.c_str());

	if (m_scenes[m_sceneIndex]) m_scenes[m_sceneIndex]->Update(timeElapsed);
}

void GameFramework::WaitForPreviousFrame()
{
	//GPU가 펜스의 값을 설정하는 명령을 명령 큐에 추가한다. 
	const UINT64 fence = m_fenceValue;
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	//CPU 펜스의 값을 증가시킨다. 
	++m_fenceValue;

	//펜스의 현재 값이 설정한 값보다 작으면 펜스의 현재 값이 설정한 값이 될 때까지 기다린다. 
	if (m_fence->GetCompletedValue() < fence)
	{
		DX::ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		::WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

// Wait for pending GPU work to complete.
void GameFramework::WaitForGpu()
{
	// Schedule a Signal command in the queue.
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

	// Wait until the fence has been processed.
	DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
}

void GameFramework::Render()
{
	// 명령 할당자와 명령 리스트를 리셋한다. 
	DX::ThrowIfFailed(m_mainCommandAllocator->Reset());
	DX::ThrowIfFailed(m_mainCommandList->Reset(m_mainCommandAllocator.Get(), nullptr));

	m_mainCommandList->SetGraphicsRootSignature(m_rootSignature.Get());
	if (m_scenes[m_sceneIndex]) {
		m_scenes[m_sceneIndex]->UpdateShaderVariable(m_mainCommandList);
		m_scenes[m_sceneIndex]->RenderShadow(m_device, m_mainCommandList);
	}

	// 자원 용도와 관련된 상태 전이를 Direct3D에 통지한다.
	m_mainCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//뷰포트와 씨저 사각형을 설정한다. 
	m_mainCommandList->RSSetViewports(1, &m_viewport);
	m_mainCommandList->RSSetScissorRects(1, &m_scissorRect);

	//현재의 렌더 타겟에 해당하는 서술자와 깊이 스텐실 서술자의 CPU 주소(핸들)를 계산한다. 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_frameIndex), m_rtvDescriptorSize };
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
	m_mainCommandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 원하는 색상으로 렌더 타겟을 지우고, 원하는 값으로 깊이 스텐실을 지운다.
	const FLOAT clearColor[]{ 0.f, 0.125f, 0.3f, 1.f };
	m_mainCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, NULL);
	m_mainCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// Scene을 Render한다.
	if (m_scenes[m_sceneIndex]) {
		m_scenes[m_sceneIndex]->Render(m_device, m_mainCommandList);
	}

	// 자원 용도와 관련된 상태 전이를 Direct3D에 통지한다.
	m_mainCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// 명령들의 기록을 마친다.
	DX::ThrowIfFailed(m_mainCommandList->Close());


	// 명령 실행을 위해 커맨드 리스트를 커맨드 큐에 추가한다.
	ID3D12CommandList* ppCommandList[] = { m_mainCommandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	RenderText();

	DX::ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void GameFramework::RenderText()
{
	m_11On12Device->AcquireWrappedResources(m_d3d11WrappedRenderTarget[m_frameIndex].GetAddressOf(),1);
	m_d2dDeviceContext->SetTarget(m_d2dRenderTarget[m_frameIndex].Get());
	m_d2dDeviceContext->BeginDraw();

	if (m_scenes[m_sceneIndex]) {
		m_scenes[m_sceneIndex]->RenderText(m_d2dDeviceContext);
	}

	m_d2dDeviceContext->EndDraw();

	m_11On12Device->ReleaseWrappedResources(m_d3d11WrappedRenderTarget[m_frameIndex].GetAddressOf(), 1);
	m_deviceContext->Flush();
}
