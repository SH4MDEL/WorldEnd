#include "blurFilter.h"

BlurFilter::BlurFilter(const ComPtr<ID3D12Device>& device, UINT width, UINT height) :
	m_width{width}, m_height{height}, m_format{ DXGI_FORMAT_R8G8B8A8_UNORM }
{
	CreateBlurMap(device);
	CreateSrvUavDescriptorHeap(device);
	CreateShaderResourceView(device);
	CreateUnorderedAccessView(device);
}

void BlurFilter::OnResize(UINT width, UINT height)
{
	if (m_width != width || m_height != height) {

	}
}

void BlurFilter::Execute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, INT blurCount)
{
	int blurRadius = 5;

	ID3D12DescriptorHeap* ppHeaps[] = { m_srvDiscriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	commandList->SetComputeRoot32BitConstants((INT)PostShaderRegister::Filter, 1, &blurRadius, 0);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_horzBlurMap.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	// Copy the input (back-buffer in this example) to horzBlurMap.
	commandList->CopyResource(m_horzBlurMap.Get(), renderTarget.Get());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_horzBlurMap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertBlurMap.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	for (int i = 0; i < blurCount; ++i)
	{
		//
		// Horizontal Blur pass.
		//

		commandList->SetPipelineState(Scene::m_shaders["HORZBLUR"]->GetPipelineState().Get());

		commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::InputTexture, m_horzBlurGpuSrv);
		commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::OutputTexture, m_vertBlurGpuUav);

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		UINT numGroupsX = (UINT)ceilf(m_width / 256.0f);
		commandList->Dispatch(numGroupsX, m_height, 1);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_horzBlurMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertBlurMap.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		//
		// Vertical Blur pass.
		//

		commandList->SetPipelineState(Scene::m_shaders["VERTBLUR"]->GetPipelineState().Get());

		commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::InputTexture, m_vertBlurGpuSrv);
		commandList->SetComputeRootDescriptorTable((INT)PostShaderRegister::OutputTexture, m_horzBlurGpuUav);

		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		UINT numGroupsY = (UINT)ceilf(m_height / 256.0f);
		commandList->Dispatch(m_width, numGroupsY, 1);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_horzBlurMap.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertBlurMap.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
}

void BlurFilter::ResetResourceBarrier(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_horzBlurMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertBlurMap.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON));
}

inline void BlurFilter::CreateBlurMap(const ComPtr<ID3D12Device>& device)
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
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_horzBlurMap)));

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_vertBlurMap)));
}

inline void BlurFilter::CreateSrvUavDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvDiscriptorHeap)));

	auto descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Save references to the descriptors.
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle{ m_srvDiscriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	m_horzBlurCpuSrv = cpuDescriptorHandle;
	m_horzBlurCpuUav = cpuDescriptorHandle.Offset(1, descriptorSize);
	m_vertBlurCpuSrv = cpuDescriptorHandle.Offset(1, descriptorSize);
	m_vertBlurCpuUav = cpuDescriptorHandle.Offset(1, descriptorSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{ m_srvDiscriptorHeap->GetGPUDescriptorHandleForHeapStart() };
	m_horzBlurGpuSrv = gpuDescriptorHandle;
	m_horzBlurGpuUav = gpuDescriptorHandle.Offset(1, descriptorSize);
	m_vertBlurGpuSrv = gpuDescriptorHandle.Offset(1, descriptorSize);
	m_vertBlurGpuUav = gpuDescriptorHandle.Offset(1, descriptorSize);
}

inline void BlurFilter::CreateShaderResourceView(const ComPtr<ID3D12Device>& device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(m_horzBlurMap.Get(), &srvDesc, m_horzBlurCpuSrv);
	device->CreateShaderResourceView(m_vertBlurMap.Get(), &srvDesc, m_vertBlurCpuSrv);
}

inline void BlurFilter::CreateUnorderedAccessView(const ComPtr<ID3D12Device>& device)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = m_format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(m_horzBlurMap.Get(), nullptr, &uavDesc, m_horzBlurCpuUav);
	device->CreateUnorderedAccessView(m_vertBlurMap.Get(), nullptr, &uavDesc, m_vertBlurCpuUav);

}
