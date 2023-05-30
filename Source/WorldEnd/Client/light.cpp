#include "light.h"

void LightSystem::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	Utiles::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightsInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_lightBuffer)));

	// 조명 버퍼 포인터
	m_lightBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_lightBufferPointer));
}

void LightSystem::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	::memcpy(&m_lightBufferPointer->m_lights, &m_lights, sizeof(Light) * m_numLight);
	::memcpy(&m_lightBufferPointer->m_numLight, &m_numLight, sizeof(UINT));

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_lightBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::Light, virtualAddress);
}
