#pragma once
#include "stdafx.h"
#include "scene.h"

class BlurFilter
{
public:
	BlurFilter(const ComPtr<ID3D12Device>& device, UINT width, UINT height);
	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator=(const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

	void OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height);

	void Execute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget, INT blurCount);

	void ResetResourceBarrier(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	ComPtr<ID3D12Resource> GetBlurMap() { return m_horzBlurMap; }

private:
	inline void CreateBlurMap(const ComPtr<ID3D12Device>& device);
	inline void CreateSrvUavDescriptorHeap(const ComPtr<ID3D12Device>& device);
	inline void CreateShaderResourceView(const ComPtr<ID3D12Device>& device);
	inline void CreateUnorderedAccessView(const ComPtr<ID3D12Device>& device);

private:
	const int maxBlurRadius = 5;

	ComPtr<ID3D12Resource> m_horzBlurMap;
	ComPtr<ID3D12Resource> m_vertBlurMap;

	UINT m_width;
	UINT m_height;
	DXGI_FORMAT m_format;

	ComPtr<ID3D12DescriptorHeap>	m_srvDiscriptorHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_horzBlurCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_horzBlurCpuUav;
								   
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_vertBlurCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_vertBlurCpuUav;
								   
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_horzBlurGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_horzBlurGpuUav;
								   
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_vertBlurGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_vertBlurGpuUav;
};

