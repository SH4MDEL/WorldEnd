#include "sobelFilter.h"
#include "framework.h"

SobelFilter::SobelFilter(const ComPtr<ID3D12Device>& device, UINT width, UINT height, const ComPtr<ID3D12RootSignature>& postRootSignature) :
	m_width{ width }, m_height{ height }, m_format{ DXGI_FORMAT_R8G8B8A8_UNORM }, m_postRootSignature{ postRootSignature }
{
	CreateSobelMap(device);
	CreateSrvUavDescriptorHeap(device);
	CreateShaderResourceView(device);
	CreateUnorderedAccessView(device);
}

void SobelFilter::OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
	if (m_width != width || m_height != height) {
		CreateSobelMap(device);
		CreateShaderResourceView(device);
		CreateUnorderedAccessView(device);
	}
}

void SobelFilter::Execute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget)
{
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvDiscriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// ·»´õ Å¸°ÙÀ» COPY_SOURCE·Î ¼³Á¤.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// ¼Òº§ ¸Ê°ú ·»´õ ¸ÊÀ» COPY_DEST·Î ¼³Á¤.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_sobelMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

	// ¼Òº§ ¸Ê°ú ·»´õ ¸Ê¿¡ ·»´õ Å¸°ÙÀ» ±â·Ï
	commandList->CopyResource(m_sobelMap.Get(), renderTarget.Get());
	commandList->CopyResource(m_renderMap.Get(), renderTarget.Get());

	// ¼Òº§ ¸Ê°ú ·»´õ ¸ÊÀ» ¿ø·¡ »óÅÂ·Î µÇµ¹¸²
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_sobelMap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderMap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	commandList->SetPipelineState(Scene::m_shaders["SOBEL"]->GetPipelineState().Get());

	commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::BaseTexture, m_sobelGpuSrv);
	commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::OutputTexture, m_sobelGpuUav);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_sobelMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	// How many groups do we need to dispatch to cover image, where each
	// group covers 16x16 pixels.
	UINT numGroupsX = (UINT)ceilf(m_width / 16.0f);
	UINT numGroupsY = (UINT)ceilf(m_height / 16.0f);
	commandList->Dispatch(numGroupsX, numGroupsY, 1);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_sobelMap.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

	commandList->SetGraphicsRootSignature(m_postRootSignature.Get());

	commandList->SetGraphicsRootDescriptorTable((INT)PostShaderRegister::BaseTexture, m_renderGpuSrv);
	commandList->SetGraphicsRootDescriptorTable((INT)PostShaderRegister::SubTexture, m_sobelGpuSrv);
}

inline void SobelFilter::CreateSobelMap(const ComPtr<ID3D12Device>& device)
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

	Utiles::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_sobelMap)));

	Utiles::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_renderMap)));
}

inline void SobelFilter::CreateSrvUavDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Utiles::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvDiscriptorHeap)));

	auto descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Save references to the descriptors.
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{ m_srvDiscriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	m_sobelCpuSrv = cpuDescriptorHandle;
	m_sobelCpuUav = cpuDescriptorHandle.Offset(1, descriptorSize);
	m_renderCpuSrv = cpuDescriptorHandle.Offset(1, descriptorSize);
	m_renderCpuUav = cpuDescriptorHandle.Offset(1, descriptorSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{ m_srvDiscriptorHeap->GetGPUDescriptorHandleForHeapStart() };
	m_sobelGpuSrv = gpuDescriptorHandle;
	m_sobelGpuUav = gpuDescriptorHandle.Offset(1, descriptorSize);
	m_renderGpuSrv = gpuDescriptorHandle.Offset(1, descriptorSize);
	m_renderGpuUav = gpuDescriptorHandle.Offset(1, descriptorSize);
}

inline void SobelFilter::CreateShaderResourceView(const ComPtr<ID3D12Device>& device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(m_sobelMap.Get(), &srvDesc, m_sobelCpuSrv);
	device->CreateShaderResourceView(m_renderMap.Get(), &srvDesc, m_renderCpuSrv);
}

inline void SobelFilter::CreateUnorderedAccessView(const ComPtr<ID3D12Device>& device)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = m_format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(m_sobelMap.Get(), nullptr, &uavDesc, m_sobelCpuUav);
	device->CreateUnorderedAccessView(m_renderMap.Get(), nullptr, &uavDesc, m_renderCpuUav);
}
