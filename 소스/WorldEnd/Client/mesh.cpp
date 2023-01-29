#include "mesh.h"
#include "object.h"

Mesh::Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const vector<TextureVertex>& vertices, const vector<UINT>& indices)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// ���� ���� ����
	m_nVertices = (UINT)vertices.size();
	const UINT vertexBufferSize = (UINT)sizeof(TextureVertex) * (UINT)vertices.size();

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		NULL,
		IID_PPV_ARGS(&m_vertexBuffer)));

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&m_vertexUploadBuffer)));

	// DEFAULT ���ۿ� UPLOAD ������ ������ ����
	D3D12_SUBRESOURCE_DATA vertextData{};
	vertextData.pData = vertices.data();
	vertextData.RowPitch = vertexBufferSize;
	vertextData.SlicePitch = vertextData.RowPitch;
	UpdateSubresources<1>(commandList.Get(), m_vertexBuffer.Get(), m_vertexUploadBuffer.Get(), 0, 0, 1, &vertextData);

	// ���� ���� ���� ����
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// ���� ���� �� ����
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = vertexBufferSize;
	m_vertexBufferView.StrideInBytes = sizeof(TextureVertex);

	//--------------------------------------------------------------------

	m_nIndices = (UINT)indices.size();
	if (m_nIndices) {
		const UINT indexBufferSize = (UINT)sizeof(UINT) * (UINT)indices.size();

		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(&m_indexBuffer)));

		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			NULL,
			IID_PPV_ARGS(&m_indexUploadBuffer)));

		D3D12_SUBRESOURCE_DATA indexData{};
		indexData.pData = indices.data();
		indexData.RowPitch = indexBufferSize;
		indexData.SlicePitch = indexData.RowPitch;
		UpdateSubresources<1>(commandList.Get(), m_indexBuffer.Get(), m_indexUploadBuffer.Get(), 0, 0, 1, &indexData);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSize;
	}
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList) const
{
	m_commandList->IASetPrimitiveTopology(m_primitiveTopology);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	if (m_nIndices) {
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else {
		m_commandList->DrawInstanced(m_nVertices, 1, 0, 0);
	}
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const
{
	m_commandList->IASetPrimitiveTopology(m_primitiveTopology);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetVertexBuffers(1, 1, &instanceBufferView);
	if (m_nIndices) {
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->DrawIndexedInstanced(m_nIndices, instanceBufferView.SizeInBytes / instanceBufferView.StrideInBytes, 0, 0, 0);
	}
	else {
		m_commandList->DrawInstanced(m_nVertices, instanceBufferView.SizeInBytes / instanceBufferView.StrideInBytes, 0, 0);
	}
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, UINT subMeshIndex) const { }

void Mesh::ReleaseUploadBuffer()
{
	if (m_vertexUploadBuffer) m_vertexUploadBuffer.Reset();
	if (m_indexUploadBuffer) m_indexUploadBuffer.Reset();
}

MeshFromFile::MeshFromFile()
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void MeshFromFile::Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, UINT subMeshIndex) const
{
	m_commandList->IASetPrimitiveTopology(m_primitiveTopology);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

	if (0 < m_nSubMeshes && subMeshIndex < m_nSubMeshes) {
		m_commandList->IASetIndexBuffer(&m_subsetIndexBufferViews[subMeshIndex]);
		m_commandList->DrawIndexedInstanced(m_vSubsetIndices[subMeshIndex], 1, 0, 0, 0);
	}
	else if (m_nIndices) {
		m_commandList->IASetIndexBuffer(&m_indexBufferView);
		m_commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else {
		m_commandList->DrawInstanced(m_nVertices, 1, 0, 0);
	}
}

void MeshFromFile::ReleaseUploadBuffer()
{
	if (m_vertexUploadBuffer) m_vertexUploadBuffer.Reset();
	if (m_indexUploadBuffer) m_indexUploadBuffer.Reset();
	for (auto& subsetUploadBuffer : m_subsetIndexUploadBuffers) {
		subsetUploadBuffer.Reset();
	}
}

void MeshFromFile::LoadFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const wstring& fileName)
{
	ifstream in{ fileName, std::ios::binary };
	if (!in) return;

	LoadFileMesh(device, commandList, in);
}

void MeshFromFile::LoadFileMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nIndices = 0;

	vector<TextureHierarchyVertex> vertices;
	vector<UINT> indices;

	BYTE strLength;

	UINT positionNum, colorNum, texcoord0Num, texcoord1Num, normalNum, tangentNum, biTangentNum;


	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<BoundingBox>:") {
			in.read((char*)(&m_boundingBox.Center), sizeof(XMFLOAT3));
			in.read((char*)(&m_boundingBox.Extents), sizeof(XMFLOAT3));
		}
		else if (strToken == "<Vertices>:") {
			in.read((char*)(&positionNum), sizeof(INT));
			if (vertices.size() < positionNum) {
				m_nVertices = positionNum;
				vertices.resize(positionNum);
			}
			for (UINT i = 0; i < positionNum; ++i) {
				in.read((char*)(&vertices[i].position), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Colors>:") {
			XMFLOAT4 dummy;
			in.read((char*)(&colorNum), sizeof(INT));
			for (UINT i = 0; i < colorNum; ++i) {
				in.read((char*)(&dummy), sizeof(XMFLOAT4));
			}
		}
		else if (strToken == "<TextureCoords0>:") {
			in.read((char*)(&texcoord0Num), sizeof(INT));
			if (vertices.size() < texcoord0Num) {
				m_nVertices = texcoord0Num;
				vertices.resize(texcoord0Num);
			}
			for (UINT i = 0; i < texcoord0Num; ++i) {
				in.read((char*)(&vertices[i].uv0), sizeof(XMFLOAT2));
			}
		}
		else if (strToken == "<TextureCoords1>:") {
			XMFLOAT2 dummy;
			in.read((char*)(&texcoord1Num), sizeof(INT));
			for (UINT i = 0; i < texcoord1Num; ++i) {
				in.read((char*)(&dummy), sizeof(XMFLOAT2));
			}
		}
		else if (strToken == "<Normals>:") {
			in.read((char*)(&normalNum), sizeof(INT));
			if (vertices.size() < normalNum) {
				m_nVertices = normalNum;
				vertices.resize(normalNum);
			}
			for (UINT i = 0; i < normalNum; ++i) {
				in.read((char*)(&vertices[i].normal), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Tangents>:") {
			in.read((char*)(&tangentNum), sizeof(INT));
			if (vertices.size() < tangentNum) {
				m_nVertices = tangentNum;
				vertices.resize(tangentNum);
			}
			for (UINT i = 0; i < tangentNum; ++i) {
				in.read((char*)(&vertices[i].tangent), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<BiTangents>:") {
			in.read((char*)(&biTangentNum), sizeof(INT));
			if (vertices.size() < biTangentNum) {
				m_nVertices = biTangentNum;
				vertices.resize(biTangentNum);
			}
			for (UINT i = 0; i < biTangentNum; ++i) {
				in.read((char*)(&vertices[i].biTangent), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Indices>:") {
			in.read((char*)(&m_nIndices), sizeof(INT));
			indices.resize(m_nIndices);
			for (UINT i = 0; i < m_nIndices; ++i) {
				in.read((char*)(&indices[i]), sizeof(UINT));
			}
		}
		else if (strToken == "<SubMeshes>:") {
			in.read((char*)(&m_nSubMeshes), sizeof(UINT));
			if (m_nSubMeshes > 0) {
				m_vSubsetIndices.resize(m_nSubMeshes);
				m_vvSubsetIndices.resize(m_nSubMeshes);
				m_subsetIndexBuffers.resize(m_nSubMeshes);
				m_subsetIndexUploadBuffers.resize(m_nSubMeshes);
				m_subsetIndexBufferViews.resize(m_nSubMeshes);

				for (UINT i = 0; i < m_nSubMeshes; ++i) {
					int index;
					in.read((char*)(&index), sizeof(UINT));
					in.read((char*)(&m_vSubsetIndices[i]), sizeof(UINT));
					in.read((char*)(&m_nIndices), sizeof(INT));
					if (m_vSubsetIndices[i] > 0) {
						m_vvSubsetIndices[i].resize(m_vSubsetIndices[i]);
						in.read((char*)(&m_vvSubsetIndices[i][0]), sizeof(UINT) * m_vSubsetIndices[i]);

						m_subsetIndexBuffers[i] = CreateBufferResource(device, commandList, m_vvSubsetIndices[i].data(),
							sizeof(UINT) * m_vSubsetIndices[i], D3D12_HEAP_TYPE_DEFAULT,
							D3D12_RESOURCE_STATE_INDEX_BUFFER, m_subsetIndexUploadBuffers[i]);

						m_subsetIndexBufferViews[i].BufferLocation = m_subsetIndexBuffers[i]->GetGPUVirtualAddress();
						m_subsetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
						m_subsetIndexBufferViews[i].SizeInBytes = sizeof(UINT) * m_vSubsetIndices[i];
					}
				}
			}
			break;
		}
		else if (strToken == "</Mesh>") {
			break;
		}
	}

	m_nVertices = (UINT)vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(TextureHierarchyVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(TextureHierarchyVertex);
	m_vertexBufferView.SizeInBytes = sizeof(TextureHierarchyVertex) * (UINT)vertices.size();

	m_nIndices = (UINT)indices.size();
	if (m_nIndices) {
		const UINT indexBufferSize = (UINT)sizeof(UINT) * (UINT)indices.size();

		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(&m_indexBuffer)));

		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			NULL,
			IID_PPV_ARGS(&m_indexUploadBuffer)));

		D3D12_SUBRESOURCE_DATA indexData{};
		indexData.pData = indices.data();
		indexData.RowPitch = indexBufferSize;
		indexData.SlicePitch = indexData.RowPitch;
		UpdateSubresources<1>(commandList.Get(), m_indexBuffer.Get(), m_indexUploadBuffer.Get(), 0, 0, 1, &indexData);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSize;
	}
}

void MeshFromFile::LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nIndices = 0;

	vector<TextureHierarchyVertex> vertices;
	vector<UINT> indices;

	BYTE strLength;

	UINT positionNum, colorNum, texcoord0Num, texcoord1Num, normalNum, tangentNum, biTangentNum;

	in.read((char*)(&m_nVertices), sizeof(INT));

	in.read((char*)(&strLength), sizeof(BYTE));
	m_meshName.resize(strLength, '\0');
	in.read(&m_meshName[0], sizeof(char) * strLength);

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<Bounds>:") {
			in.read((char*)(&m_boundingBox.Center), sizeof(XMFLOAT3));
			in.read((char*)(&m_boundingBox.Extents), sizeof(XMFLOAT3));
		}
		else if (strToken == "<Positions>:") {
			in.read((char*)(&positionNum), sizeof(INT));
			if (vertices.size() < positionNum) {
				m_nVertices = positionNum;
				vertices.resize(positionNum);
			}
			for (UINT i = 0; i < positionNum; ++i) {
				in.read((char*)(&vertices[i].position), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Colors>:") {
			XMFLOAT4 dummy;
			in.read((char*)(&colorNum), sizeof(INT));
			for (UINT i = 0; i < colorNum; ++i) {
				in.read((char*)(&dummy), sizeof(XMFLOAT4));
			}
		}
		else if (strToken == "<TextureCoords0>:") {
			in.read((char*)(&texcoord0Num), sizeof(INT));
			if (vertices.size() < texcoord0Num) {
				m_nVertices = texcoord0Num;
				vertices.resize(texcoord0Num);
			}
			for (UINT i = 0; i < texcoord0Num; ++i) {
				in.read((char*)(&vertices[i].uv0), sizeof(XMFLOAT2));
			}
		}
		else if (strToken == "<TextureCoords1>:") {
			XMFLOAT2 dummy;
			in.read((char*)(&texcoord1Num), sizeof(INT));
			for (UINT i = 0; i < texcoord1Num; ++i) {
				in.read((char*)(&dummy), sizeof(XMFLOAT2));
			}
		}
		else if (strToken == "<Normals>:") {
			in.read((char*)(&normalNum), sizeof(INT));
			if (vertices.size() < normalNum) {
				m_nVertices = normalNum;
				vertices.resize(normalNum);
			}
			for (UINT i = 0; i < normalNum; ++i) {
				in.read((char*)(&vertices[i].normal), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Tangents>:") {
			in.read((char*)(&tangentNum), sizeof(INT));
			if (vertices.size() < tangentNum) {
				m_nVertices = tangentNum;
				vertices.resize(tangentNum);
			}
			for (UINT i = 0; i < tangentNum; ++i) {
				in.read((char*)(&vertices[i].tangent), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<BiTangents>:") {
			in.read((char*)(&biTangentNum), sizeof(INT));
			if (vertices.size() < biTangentNum) {
				m_nVertices = biTangentNum;
				vertices.resize(biTangentNum);
			}
			for (UINT i = 0; i < biTangentNum; ++i) {
				in.read((char*)(&vertices[i].biTangent), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Indices>:") {
			in.read((char*)(&m_nIndices), sizeof(INT));
			indices.resize(m_nIndices);
			in.read((char*)(&indices), sizeof(UINT) * m_nIndices);
		}
		else if (strToken == "<SubMeshes>:") {
			in.read((char*)(&m_nSubMeshes), sizeof(UINT));
			if (m_nSubMeshes > 0) {
				m_vSubsetIndices.resize(m_nSubMeshes);
				m_vvSubsetIndices.resize(m_nSubMeshes);
				m_subsetIndexBuffers.resize(m_nSubMeshes);
				m_subsetIndexUploadBuffers.resize(m_nSubMeshes);
				m_subsetIndexBufferViews.resize(m_nSubMeshes);

				for (UINT i = 0; i < m_nSubMeshes; ++i) {
					in.read((char*)(&strLength), sizeof(BYTE));
					string strToken(strLength, '\0');
					in.read((&strToken[0]), sizeof(char) * strLength);

					if (strToken == "<SubMesh>:") {
						int index;
						in.read((char*)(&index), sizeof(UINT));
						in.read((char*)(&m_vSubsetIndices[i]), sizeof(UINT));
						if (m_vSubsetIndices[i] > 0) {
							m_vvSubsetIndices[i].resize(m_vSubsetIndices[i]);
							in.read((char*)(&m_vvSubsetIndices[i][0]), sizeof(UINT) * m_vSubsetIndices[i]);

							m_subsetIndexBuffers[i] = CreateBufferResource(device, commandList, m_vvSubsetIndices[i].data(),
								sizeof(UINT) * m_vSubsetIndices[i], D3D12_HEAP_TYPE_DEFAULT,
								D3D12_RESOURCE_STATE_INDEX_BUFFER, m_subsetIndexUploadBuffers[i]);

							m_subsetIndexBufferViews[i].BufferLocation = m_subsetIndexBuffers[i]->GetGPUVirtualAddress();
							m_subsetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
							m_subsetIndexBufferViews[i].SizeInBytes = sizeof(UINT) * m_vSubsetIndices[i];
						}
					}
				}
			}
		}
		else if (strToken == "</Mesh>") {
			break;
		}
	}

	m_nVertices = (UINT)vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(TextureHierarchyVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(TextureHierarchyVertex);
	m_vertexBufferView.SizeInBytes = sizeof(TextureHierarchyVertex) * vertices.size();
}

FieldMapImage::FieldMapImage(INT width, INT length, INT height, XMFLOAT3 scale) :
	m_width(width), m_length(length), m_scale(scale), m_pixels{ new BYTE[width * length] }
{
	for (int y = 0; y < m_length; ++y) {
		for (int x = 0; x < m_width; ++x) {
			m_pixels[x + (y * m_width)] = height;
		}
	}
}

FLOAT FieldMapImage::GetHeight(FLOAT x, FLOAT z) const
{
	//������ ��ǥ(x, z)�� �̹��� ��ǥ���̴�.
	//���� ���� x-��ǥ�� z-��ǥ�� ���� ���� ������ ����� ������ ���̴� 0�̴�.
	if ((x < 0.0f) || (z < 0.0f) || (x >= m_width) || (z >= m_length)) return 0.0f;

	//���� ���� ��ǥ�� ���� �κа� �Ҽ� �κ��� ����Ѵ�. 
	int mx{ (int)x };
	int mz{ (int)z };
	float px{ x - mx };
	float pz{ z - mz };
	
	float bottomLeft{ (float)m_pixels[mx + mz * m_width] };
	float bottomRight{ (float)m_pixels[(mx + 1) + (mz * m_width)] };
	float topLeft{ (float)m_pixels[mx + (mz + 1) * m_width] };
	float topRight{ (float)m_pixels[(mx + 1) + ((mz + 1) * m_width)] };

	//�簢���� �� ���� �����Ͽ� ����(�ȼ� ��)�� ����Ѵ�. 
	float fTopHeight{ topLeft * (1 - px) + topRight * px };
	float fBottomHeight{ bottomLeft * (1 - px) + bottomRight * px };
	return fBottomHeight * (1 - pz) + fTopHeight * pz;
}

XMFLOAT3 FieldMapImage::GetNormal(INT x, INT z) const
{
	//x-��ǥ�� z-��ǥ�� ���� ���� ������ ����� ������ ���� ���ʹ� y-�� ���� �����̴�. 
	if ((x < 0.0f) || (z < 0.0f) || (x >= m_width) || (z >= m_length))
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	/*���� �ʿ��� (x, z) ��ǥ�� �ȼ� ���� ������ �� ���� �� (x+1, z), (z, z+1)�� ���� �ȼ� ���� ����Ͽ� ���� ���͸�
	����Ѵ�.*/
	int heightMapIndex{ x + z * m_width };
	int xAdd{ x < m_width - 1 ? 1 : -1 };
	int zAdd{ z < m_length - 1 ? m_width : -m_width };

	//(x, z), (x+1, z), (z, z+1)�� �ȼ����� ������ ���̸� ���Ѵ�. 
	float y1{ (float)m_pixels[heightMapIndex] * m_scale.y };
	float y2{ (float)m_pixels[heightMapIndex + xAdd] * m_scale.y };
	float y3{ (float)m_pixels[heightMapIndex + zAdd] * m_scale.y };

	//xmf3Edge1�� (0, y3, m_xmf3Scale.z) - (0, y1, 0) �����̴�. 
	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, m_scale.z);
	//xmf3Edge2�� (m_xmf3Scale.x, y2, 0) - (0, y1, 0) �����̴�. 
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(m_scale.x, y2 - y1, 0.0f);
	//���� ���ʹ� xmf3Edge1�� xmf3Edge2�� ������ ����ȭ�ϸ� �ȴ�. 
	return Vector3::Cross(xmf3Edge1, xmf3Edge2);
}

FieldMapGridMesh::FieldMapGridMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	INT xStart, INT zStart, INT width, INT length, XMFLOAT3 scale, FieldMapImage* heightMapImage)
{
	//���ڴ� �ﰢ�� ��Ʈ������ �����Ѵ�. 
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	m_width = width;
	m_length = length;
	m_scale = scale;

	vector<DetailVertex> vertices;

	/*xStart�� zStart�� ������ ���� ��ġ(x-��ǥ�� z-��ǥ)�� ��Ÿ����. Ŀ�ٶ� ������ ���ڵ��� ������ �迭�� ���� ��
	�䰡 �ֱ� ������ ��ü �������� �� ������ ���� ��ġ�� ��Ÿ���� ������ �ʿ��ϴ�.*/
	for (int i = 0, z = zStart; z < (zStart + length); ++z) {
		for (int x = xStart; x < (xStart + width); ++x, ++i)
		{
			//������ ���̿� ������ ���� �����κ��� ���Ѵ�.
			XMFLOAT3 anormal = Vector3::Add(heightMapImage->GetNormal(x, z), heightMapImage->GetNormal(x + 1, z));
			anormal = Vector3::Add(heightMapImage->GetNormal(x + 1, z + 1), heightMapImage->GetNormal(x, z + 1));
			Vector3::Normalize(anormal);

			vertices.emplace_back(XMFLOAT3((x * m_scale.x), heightMapImage->GetHeight((FLOAT)x, (FLOAT)z), (z * m_scale.z)),
				XMFLOAT2((FLOAT)x / (FLOAT)heightMapImage->GetWidth(), 1.0f - ((FLOAT)z / (FLOAT)heightMapImage->GetLength())),
				XMFLOAT2(FLOAT(x) / FLOAT(m_scale.x * 0.5f), FLOAT(z) / FLOAT(m_scale.z * 0.5f)));
		}
	}

	m_nVertices = (UINT)vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(DetailVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(DetailVertex);
	m_vertexBufferView.SizeInBytes = sizeof(DetailVertex) * vertices.size();


	vector<UINT> indices;

	for (int j = 0, z = 0; z < length - 1; ++z)
	{
		if ((z % 2) == 0)
		{
			//Ȧ�� ��° ���̹Ƿ�(z = 0, 2, 4, ...) �ε����� ���� ������ ���ʿ��� ������ �����̴�. 
			for (int x = 0; x < width; x++)
			{
				//ù ��° ���� �����ϰ� ���� �ٲ� ������(x == 0) ù ��° �ε����� �߰��Ѵ�. 
				if (x == 0 && z > 0) indices.push_back(x + (z * width));
				//�Ʒ�(x, z), ��(x, z+1)�� ������ �ε����� �߰��Ѵ�. 
				indices.push_back(x + (z * width));
				indices.push_back(x + (z * width) + width);
			}
		}
		else
		{
			//¦�� ��° ���̹Ƿ�(z = 1, 3, 5, ...) �ε����� ���� ������ �����ʿ��� ���� �����̴�. 
			for (int x = width - 1; x >= 0; --x)
			{
				//���� �ٲ� ������(x == (nWidth-1)) ù ��° �ε����� �߰��Ѵ�. 
				if (x == width - 1) indices.push_back(x + (z * width));
				//�Ʒ�(x, z), ��(x, z+1)�� ������ �ε����� �߰��Ѵ�. 
				indices.push_back(x + (z * width));
				indices.push_back(x + (z * width) + width);
			}
		}
	}

	m_nIndices = indices.size();
	m_indexBuffer = CreateBufferResource(device, commandList, indices.data(),
		sizeof(UINT) * indices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_indexUploadBuffer);

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = sizeof(UINT) * indices.size();
}

TextureRectMesh::TextureRectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, XMFLOAT3 position, FLOAT width, FLOAT height, FLOAT depth)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nIndices = 0;
	m_nVertices = 6;

	vector<TextureVertex> vertices;
	FLOAT fx = (width * 0.5f), fy = (height * 0.5f), fz = (depth * 0.5f);

	if (width == 0.0f)
	{
		if (position.x > 0.0f)
		{
			vertices.emplace_back(XMFLOAT3(fx + position.x, +fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, -fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, -fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, -fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, +fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, +fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 0.0f));
		}
		else
		{
			vertices.emplace_back(XMFLOAT3(fx + position.x, +fy + position.y, +fz + position.z), XMFLOAT2(1.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, -fy + position.y, +fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, -fy + position.y, -fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, -fy + position.y, -fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, +fy + position.y, -fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(fx + position.x, +fy + position.y, +fz + position.z), XMFLOAT2(1.0f, 0.0f));
		}
	}
	else if (height == 0.0f)
	{
		if (position.y > 0.0f)
		{
			vertices.emplace_back(XMFLOAT3(+fx + position.x, fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, fy + position.y, +fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, fy + position.y, -fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 0.0f));
		}
		else
		{
			vertices.emplace_back(XMFLOAT3(+fx + position.x, fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, fy + position.y, -fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, fy + position.y, -fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, fy + position.y, +fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, fy + position.y, +fz + position.z), XMFLOAT2(1.0f, 0.0f));
		}
	}
	else if (depth == 0.0f)
	{
		if (position.z > 0.0f)
		{
			vertices.emplace_back(XMFLOAT3(+fx + position.x, +fy + position.y, fz + position.z), XMFLOAT2(1.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, -fy + position.y, fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, -fy + position.y, fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, -fy + position.y, fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, +fy + position.y, fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, +fy + position.y, fz + position.z), XMFLOAT2(1.0f, 0.0f));
		}
		else
		{
			vertices.emplace_back(XMFLOAT3(-fx + position.x, +fy + position.y, fz + position.z), XMFLOAT2(1.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, -fy + position.y, fz + position.z), XMFLOAT2(1.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, -fy + position.y, fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, -fy + position.y, fz + position.z), XMFLOAT2(0.0f, 1.0f));
			vertices.emplace_back(XMFLOAT3(+fx + position.x, +fy + position.y, fz + position.z), XMFLOAT2(0.0f, 0.0f));
			vertices.emplace_back(XMFLOAT3(-fx + position.x, +fy + position.y, fz + position.z), XMFLOAT2(1.0f, 0.0f));
		}
	}

	m_nVertices = vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(TextureVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(TextureVertex);
	m_vertexBufferView.SizeInBytes = sizeof(TextureVertex) * vertices.size();
}

SkyboxMesh::SkyboxMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height, FLOAT depth)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nIndices = 0;
	m_nVertices = 36;

	vector<SkyboxVertex> vertices;
	FLOAT fx = width * 0.5f, fy = height * 0.5f, fz = depth * 0.5f;

	vertices.emplace_back(XMFLOAT3(-fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, +fx));
		   
	vertices.emplace_back(XMFLOAT3(+fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, -fx));
		   
	vertices.emplace_back(XMFLOAT3(-fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, +fx));
		   
	vertices.emplace_back(XMFLOAT3(+fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, -fx));
		   
	vertices.emplace_back(XMFLOAT3(-fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(-fx, +fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, +fx, +fx));
		   
	vertices.emplace_back(XMFLOAT3(-fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, -fx));
	vertices.emplace_back(XMFLOAT3(-fx, -fx, -fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, +fx));
	vertices.emplace_back(XMFLOAT3(+fx, -fx, -fx));

	m_nVertices = vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(SkyboxVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(SkyboxVertex);
	m_vertexBufferView.SizeInBytes = sizeof(SkyboxVertex) * vertices.size();
}

SkinnedMesh::SkinnedMesh()
{
}

void SkinnedMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList) const
{
}

void SkinnedMesh::ReleaseUploadBuffer()
{
}

void SkinnedMesh::LoadSkinnedMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	BYTE strLength{};

	INT bonesPerVertex{}, boneNameNum{}, boneOffsetNum{}, boneIndexNum{}, boneWeightNum{};

	in.read((char*)(&strLength), sizeof(BYTE));
	in.read(&m_skinnedMeshName[0], sizeof(char) * strLength);

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<BonesPerVertex>:") {
			in.read((char*)(&bonesPerVertex), sizeof(INT));
			m_nBonesPerVertex = bonesPerVertex;
		}
		else if (strToken == "<Bounds>:") {
			in.read((char*)(&m_boundingBox.Center), sizeof(XMFLOAT3));
			in.read((char*)(&m_boundingBox.Extents), sizeof(XMFLOAT3));
		}
		else if (strToken == "<BoneNames>:") {
			in.read((char*)(&boneNameNum), sizeof(INT));
			m_nSkinningBones = boneNameNum;

			m_skinningBoneFrame.resize(boneNameNum);	// ������Ʈ ���ʹ� ����� �ø��� ���� X
			m_skinningBoneNames.resize(boneNameNum);

			for (int i = 0; i < boneNameNum; ++i) {
				in.read((char*)(&strLength), sizeof(BYTE));
				m_skinningBoneNames[i].resize(strLength);
				in.read(&m_skinningBoneNames[i][0], sizeof(char) * strLength);
			}
		}
		else if (strToken == "<BoneOffsets>:") {
			in.read((char*)(&boneOffsetNum), sizeof(INT));
			m_bindPoseBoneOffsets.resize(boneOffsetNum);

			for (int i = 0; i < boneOffsetNum; ++i) {
				in.read((char*)(&m_bindPoseBoneOffsets[i]), sizeof(XMFLOAT4X4));
				//XMStoreFloat4x4(&m_mappedBindPoseBoneOffsets[i], XMMatrixTranspose(XMLoadFloat4x4(&m_bindPoseBoneOffsets[i])));
			}

			// ���� ���ҽ� ���� ó�� �ʿ�, boneOffsetNum > 0 �϶�

		}
		else if (strToken == "<BoneIndices>:") {
			in.read((char*)(&boneIndexNum), sizeof(INT));
			m_boneIndices.resize(boneIndexNum);

			for (int i = 0; i < boneIndexNum; ++i) {
				in.read((char*)(&m_boneIndices[i]), sizeof(XMUINT4));
			}

			// �� �ε��� ����, ���� �� ó�� �ʿ�, boneIndexNum > 0 �϶�

		}
		else if (strToken == "<BoneWeights>:") {
			in.read((char*)(&boneWeightNum), sizeof(INT));
			m_boneWeights.resize(boneWeightNum);

			for (int i = 0; i < boneWeightNum; ++i) {
				in.read((char*)(&m_boneWeights[i]), sizeof(XMFLOAT4));
			}

			// �� ����ġ ����, ���� �� ó�� �ʿ�, boneWeightNum > 0 �϶�

		}
		else if (strToken == "</SkinningInfo>") {
			break;
		}
	}
}

BillboardMesh::BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, XMFLOAT3 position, XMFLOAT2 size)
{
	m_nIndices = 0;
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	TextureVertex vertex{ position, size };

	m_nVertices = 1;
	m_vertexBuffer = CreateBufferResource(device, commandList, &vertex,
		sizeof(TextureVertex), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizeof(TextureVertex);
	m_vertexBufferView.StrideInBytes = sizeof(TextureVertex);
}