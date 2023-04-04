#pragma once
#include "stdafx.h"

class Shadow
{
public:
	Shadow(const ComPtr<ID3D12Device>& device, UINT width, UINT height);
	Shadow(const Shadow& rhs) = delete;
	Shadow& operator=(const Shadow& rhs) = delete;
	~Shadow() = default;

	ComPtr<ID3D12DescriptorHeap> GetSrvDiscriptorHeap() const { return m_srvDiscriptorHeap; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv() const { return m_gpuSrv; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuDsv() const { return m_cpuDsv; }

	D3D12_VIEWPORT GetViewport() const { return m_viewport; }
	D3D12_RECT GetScissorRect() const { return m_scissorRect; }
	ComPtr<ID3D12Resource> GetShadowMap() const { return m_shadowMap; }
private:
	inline void CreateShadowMap(const ComPtr<ID3D12Device>& device);
	inline void CreateSrvDsvDescriptorHeap(const ComPtr<ID3D12Device>& device);
	inline void CreateShaderResourceView(const ComPtr<ID3D12Device>& device);
	inline void CreateDepthStencilView(const ComPtr<ID3D12Device>& device);

private:
	D3D12_VIEWPORT					m_viewport;
	D3D12_RECT						m_scissorRect;

	UINT							m_width;
	UINT							m_height;
	DXGI_FORMAT						m_format;

	ComPtr<ID3D12DescriptorHeap>	m_srvDiscriptorHeap;
	ComPtr<ID3D12DescriptorHeap>	m_dsvDiscriptorHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_cpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_gpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_cpuDsv;

	ComPtr<ID3D12Resource>			m_shadowMap;
};

