#include "particleMesh.h"

ParticleMesh::ParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_nIndices = 0;
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	m_isBinding = false;
}

EmitterParticleMesh::EmitterParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) 
{
	m_nIndices = 0;
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	m_isBinding = false;

	vector<EmitterParticleVertex> vertices;

	for (int i = 0; i < MAX_PARTICLE_MESH; ++i) {
		vertices.emplace_back(
			XMFLOAT3{ DX::GetRandomFLOAT(-1.f, 1.f), DX::GetRandomFLOAT(-1.f, 1.f), DX::GetRandomFLOAT(-1.f, 1.f) },
			XMFLOAT3{ DX::GetRandomFLOAT(-3.f, 3.f), DX::GetRandomFLOAT(-3.f, 3.f), DX::GetRandomFLOAT(-3.f, 3.f) },
			0.f, 1.5f);
	}
	m_nVertices = (UINT)vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(EmitterParticleVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(EmitterParticleVertex);
	m_vertexBufferView.SizeInBytes = sizeof(EmitterParticleVertex) * vertices.size();

	CreateStreamOutputBuffer(device, commandList);
}

void EmitterParticleMesh::CreateStreamOutputBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ComPtr<ID3D12Resource> streamOutputUploadBuffer;
	m_streamOutputBuffer = CreateBufferResource(device, commandList, nullptr,
		sizeof(EmitterParticleVertex) * MAX_PARTICLE_MESH, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_STREAM_OUT, streamOutputUploadBuffer);

	m_filledSizeBuffer = CreateBufferResource(device, commandList, nullptr,
		sizeof(UINT64), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_STREAM_OUT, streamOutputUploadBuffer);

	m_filledSizeUploadBuffer = CreateBufferResource(device, commandList, nullptr,
		sizeof(UINT64), D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ, streamOutputUploadBuffer);
	m_filledSizeUploadBuffer->Map(0, NULL, reinterpret_cast<void**>(&m_filledSizeUploadBufferSize));

	m_streamOutputBufferView.BufferLocation = m_streamOutputBuffer->GetGPUVirtualAddress();
	m_streamOutputBufferView.SizeInBytes = sizeof(EmitterParticleVertex) * MAX_PARTICLE_MESH;
	m_streamOutputBufferView.BufferFilledSizeLocation = m_filledSizeBuffer->GetGPUVirtualAddress();

	m_filledSizeReadbackBuffer = CreateBufferResource(device, commandList, nullptr,
		sizeof(UINT64), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, streamOutputUploadBuffer);
	m_filledSizeReadbackBuffer->Map(0, NULL, reinterpret_cast<void**>(&m_filledSizeReadbackBufferSize));

	m_drawBuffer = CreateBufferResource(device, commandList, nullptr,
		sizeof(EmitterParticleVertex) * MAX_PARTICLE_MESH, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, streamOutputUploadBuffer);
}

void EmitterParticleMesh::RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (!m_isBinding) {
		m_isBinding = true;
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(EmitterParticleVertex);
		m_vertexBufferView.SizeInBytes = sizeof(EmitterParticleVertex) * m_nVertices;
	}

	*m_filledSizeUploadBufferSize = 0;
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->CopyResource(m_filledSizeBuffer.Get(), m_filledSizeUploadBuffer.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT));

	D3D12_STREAM_OUTPUT_BUFFER_VIEW streamOutputBufferViews[]{ m_streamOutputBufferView };
	commandList->SOSetTargets(0, _countof(streamOutputBufferViews), streamOutputBufferViews);
	Mesh::Render(commandList);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_filledSizeReadbackBuffer.Get(), m_filledSizeBuffer.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_filledSizeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT));

	UINT64* filledSize = NULL;
	m_filledSizeReadbackBuffer->Map(0, NULL, reinterpret_cast<void**>(&filledSize));
	m_nVertices = UINT(*filledSize) / sizeof(EmitterParticleVertex);
	m_filledSizeReadbackBuffer->Unmap(0, NULL);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_drawBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamOutputBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_drawBuffer.Get(), m_streamOutputBuffer.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_drawBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamOutputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void EmitterParticleMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_vertexBufferView.BufferLocation = m_drawBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(EmitterParticleVertex);
	m_vertexBufferView.SizeInBytes = sizeof(EmitterParticleVertex) * m_nVertices;

	commandList->SOSetTargets(0, 1, NULL);
	Mesh::Render(commandList);
}
