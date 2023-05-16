#pragma once
#include "stdafx.h"
#include "scene.h"

class SobelFilter
{
public:
	SobelFilter(const ComPtr<ID3D12Device>& device, UINT width, UINT height, const ComPtr<ID3D12RootSignature>& postRootSignature);
	SobelFilter(const SobelFilter& rhs) = delete;
	SobelFilter& operator=(const SobelFilter& rhs) = delete;
	~SobelFilter() = default;

	void OnResize(const ComPtr<ID3D12Device>& device, UINT width, UINT height);

	void Execute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget);

	ComPtr<ID3D12Resource> GetSobelMap() { return m_sobelMap; }

private:
	inline void CreateSobelMap(const ComPtr<ID3D12Device>& device);
	inline void CreateSrvUavDescriptorHeap(const ComPtr<ID3D12Device>& device);
	inline void CreateShaderResourceView(const ComPtr<ID3D12Device>& device);
	inline void CreateUnorderedAccessView(const ComPtr<ID3D12Device>& device);

private:
	ComPtr<ID3D12Resource> m_sobelMap;
	ComPtr<ID3D12Resource> m_renderMap;

	UINT m_width;
	UINT m_height;
	DXGI_FORMAT m_format;

	ComPtr<ID3D12DescriptorHeap>	m_srvDiscriptorHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_sobelCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_sobelCpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_renderCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_renderCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_sobelGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_sobelGpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_renderGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_renderGpuUav;

	ComPtr<ID3D12RootSignature> m_postRootSignature;
};

