#include "fadeFilter.h"

FadeFilter::FadeFilter(const ComPtr<ID3D12Device>& device, UINT width, UINT height) :
	m_width{ width }, m_height{ height }, m_format{ DXGI_FORMAT_R8G8B8A8_UNORM },
	m_fadeIn{false}, m_fadeOut{false}
{
	CreateFadeMap(device);
	CreateSrvUavDescriptorHeap(device);
	CreateShaderResourceView(device);
	CreateUnorderedAccessView(device);
}

void FadeFilter::OnResize(UINT width, UINT height)
{
	if (m_width != width || m_height != height) {

	}
}

void FadeFilter::Execute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget)
{
	if (m_fadeIn || m_fadeOut) {
		ID3D12DescriptorHeap* ppHeaps[] = { m_srvDiscriptorHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		commandList->SetComputeRoot32BitConstants((INT)PostShaderRegister::Fade, 1, &m_age, 0);
		commandList->SetComputeRoot32BitConstants((INT)PostShaderRegister::Fade, 1, &m_fadeLifetime, 1);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

		// ·»´õ ¸Ê¿¡ ·»´õ Å¸°ÙÀ» ±â·Ï
		commandList->CopyResource(m_renderMap.Get(), renderTarget.Get());

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderMap.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_fadeMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		// ÆÄÀÌÇÁ¶óÀÎ »óÅÂ ¼³Á¤
		commandList->SetPipelineState(Scene::m_shaders["FADE"]->GetPipelineState().Get());

		commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::BaseTexture, m_renderGpuSrv);
		commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::OutputTexture, m_fadeGpuUav);

		// ·»´õ ¸ÊÀ» ¹ÙÅÁÀ¸·Î ÆäÀÌµå ¸Ê¿¡ ÆäÀÌµå Á¤º¸ ±â·Ï
		UINT numGroupsX = (UINT)ceilf(m_width / 16.0f);
		UINT numGroupsY = (UINT)ceilf(m_height / 16.0f);
		commandList->Dispatch(numGroupsX, numGroupsY, 1);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_fadeMap.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

		// ·»´õ Å¸°Ù¿¡ ÆäÀÌµå ¸ÊÀ» ±â·Ï
		commandList->CopyResource(renderTarget.Get(), m_fadeMap.Get());

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_fadeMap.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
}

void FadeFilter::FadeIn(function<void()> event)
{
	m_event = event;
	m_age = 0.f;
	m_fadeIn = true;
	m_fadeOut = false;
}

void FadeFilter::FadeOut(function<void()> event)
{
	m_event = event;
	m_age = m_fadeLifetime;
	m_fadeOut = true;
	m_fadeIn = false;
}

void FadeFilter::Update(FLOAT timeElapsed)
{
	if (m_fadeIn) {
		m_age += timeElapsed;
		if (m_age >= m_fadeLifetime) {
			m_age = m_fadeLifetime;
			m_fadeIn = false;
			m_event();
		}
	}
	else if (m_fadeOut) {
		m_age -= timeElapsed;
		if (m_age <= 0.f) {
			m_age = 0.f;
			m_fadeOut = false;
			m_event();
		}
	}
}

inline void FadeFilter::CreateFadeMap(const ComPtr<ID3D12Device>& device)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_width;
	texDesc.Height = m_height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = m_format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_renderMap)));

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_fadeMap)));
}

inline void FadeFilter::CreateSrvUavDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvDiscriptorHeap)));

	auto descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Save references to the descriptors.
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{ m_srvDiscriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	m_renderCpuSrv = cpuDescriptorHandle;
	m_renderCpuUav = cpuDescriptorHandle.Offset(1, descriptorSize);
	m_fadeCpuSrv = cpuDescriptorHandle.Offset(1, descriptorSize);
	m_fadeCpuUav = cpuDescriptorHandle.Offset(1, descriptorSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{ m_srvDiscriptorHeap->GetGPUDescriptorHandleForHeapStart() };
	m_renderGpuSrv = gpuDescriptorHandle;
	m_renderGpuUav = gpuDescriptorHandle.Offset(1, descriptorSize);
	m_fadeGpuSrv = gpuDescriptorHandle.Offset(1, descriptorSize);
	m_fadeGpuUav = gpuDescriptorHandle.Offset(1, descriptorSize);
}

inline void FadeFilter::CreateShaderResourceView(const ComPtr<ID3D12Device>& device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(m_renderMap.Get(), &srvDesc, m_renderCpuSrv);
	device->CreateShaderResourceView(m_fadeMap.Get(), &srvDesc, m_fadeCpuSrv);
}

inline void FadeFilter::CreateUnorderedAccessView(const ComPtr<ID3D12Device>& device)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = m_format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(m_renderMap.Get(), nullptr, &uavDesc, m_renderCpuUav);
	device->CreateUnorderedAccessView(m_fadeMap.Get(), nullptr, &uavDesc, m_fadeCpuUav);
}
