#pragma once
#include "stdafx.h"
#include "scene.h"

class FadeFilter
{
public:
	FadeFilter(const ComPtr<ID3D12Device>& device, UINT width, UINT height);
	FadeFilter(const FadeFilter& rhs) = delete;
	FadeFilter& operator=(const FadeFilter& rhs) = delete;
	~FadeFilter() = default;

	void OnResize(UINT width, UINT height);

	void Execute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& renderTarget);

	ComPtr<ID3D12Resource> GetFadeMap() { return m_fadeMap; }

	void FadeIn(function<void()> event);
	void FadeOut(function<void()> event);

	void Update(FLOAT timeElapsed);

private:
	inline void CreateFadeMap(const ComPtr<ID3D12Device>& device);
	inline void CreateSrvUavDescriptorHeap(const ComPtr<ID3D12Device>& device);
	inline void CreateShaderResourceView(const ComPtr<ID3D12Device>& device);
	inline void CreateUnorderedAccessView(const ComPtr<ID3D12Device>& device);

private:
	const FLOAT m_fadeLifetime = 1.f;

	ComPtr<ID3D12Resource> m_renderMap;
	ComPtr<ID3D12Resource> m_fadeMap;

	UINT m_width;
	UINT m_height;
	DXGI_FORMAT m_format;

	ComPtr<ID3D12DescriptorHeap>	m_srvDiscriptorHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_renderCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_renderCpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_fadeCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_fadeCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_renderGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_renderGpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_fadeGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_fadeGpuUav;

	ComPtr<ID3D12RootSignature> m_postRootSignature;

	function<void()>			m_event;
	BOOL						m_fadeIn;
	BOOL						m_fadeOut;
	FLOAT						m_age;
};

