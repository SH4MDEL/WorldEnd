﻿#include "mesh.h"
#include "object.h"

Mesh::Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const vector<TextureVertex>& vertices, const vector<UINT>& indices)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 정점 버퍼 생성
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

	// DEFAULT 버퍼에 UPLOAD 버퍼의 데이터 복사
	D3D12_SUBRESOURCE_DATA vertextData{};
	vertextData.pData = vertices.data();
	vertextData.RowPitch = vertexBufferSize;
	vertextData.SlicePitch = vertextData.RowPitch;
	UpdateSubresources<1>(commandList.Get(), m_vertexBuffer.Get(), m_vertexUploadBuffer.Get(), 0, 0, 1, &vertextData);

	// 정점 버퍼 상태 변경
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// 정점 버퍼 뷰 설정
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

	LoadMesh(device, commandList, in);
}

void MeshFromFile::LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nIndices = 0;

	vector<TextureHierarchyVertex> vertices;
	vector<UINT> indices;

	BYTE strLength;

	UINT positionNum, colorNum, texcoord0Num, texcoord1Num, normalNum, tangentNum, biTangentNum;

	//in.read((char*)(&m_nVertices), sizeof(INT));

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
	//지형의 좌표(x, z)는 이미지 좌표계이다.
	//높이 맵의 x-좌표와 z-좌표가 높이 맵의 범위를 벗어나면 지형의 높이는 0이다.
	if ((x < 0.0f) || (z < 0.0f) || (x >= m_width) || (z >= m_length)) return 0.0f;

	//높이 맵의 좌표의 정수 부분과 소수 부분을 계산한다. 
	int mx{ (int)x };
	int mz{ (int)z };
	float px{ x - mx };
	float pz{ z - mz };
	
	float bottomLeft{ (float)m_pixels[mx + mz * m_width] };
	float bottomRight{ (float)m_pixels[(mx + 1) + (mz * m_width)] };
	float topLeft{ (float)m_pixels[mx + (mz + 1) * m_width] };
	float topRight{ (float)m_pixels[(mx + 1) + ((mz + 1) * m_width)] };

	//사각형의 네 점을 보간하여 높이(픽셀 값)를 계산한다. 
	float fTopHeight{ topLeft * (1 - px) + topRight * px };
	float fBottomHeight{ bottomLeft * (1 - px) + bottomRight * px };
	return fBottomHeight * (1 - pz) + fTopHeight * pz;
}

XMFLOAT3 FieldMapImage::GetNormal(INT x, INT z) const
{
	//x-좌표와 z-좌표가 높이 맵의 범위를 벗어나면 지형의 법선 벡터는 y-축 방향 벡터이다. 
	if ((x < 0.0f) || (z < 0.0f) || (x >= m_width) || (z >= m_length))
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	/*높이 맵에서 (x, z) 좌표의 픽셀 값과 인접한 두 개의 점 (x+1, z), (z, z+1)에 대한 픽셀 값을 사용하여 법선 벡터를
	계산한다.*/
	int heightMapIndex{ x + z * m_width };
	int xAdd{ x < m_width - 1 ? 1 : -1 };
	int zAdd{ z < m_length - 1 ? m_width : -m_width };

	//(x, z), (x+1, z), (z, z+1)의 픽셀에서 지형의 높이를 구한다. 
	float y1{ (float)m_pixels[heightMapIndex] * m_scale.y };
	float y2{ (float)m_pixels[heightMapIndex + xAdd] * m_scale.y };
	float y3{ (float)m_pixels[heightMapIndex + zAdd] * m_scale.y };

	//xmf3Edge1은 (0, y3, m_xmf3Scale.z) - (0, y1, 0) 벡터이다.  
	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, m_scale.z);
	//xmf3Edge2는 (m_xmf3Scale.x, y2, 0) - (0, y1, 0) 벡터이다. 
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(m_scale.x, y2 - y1, 0.0f);
	//법선 벡터는 xmf3Edge1과 xmf3Edge2의 외적을 정규화하면 된다. 
	return Vector3::Cross(xmf3Edge1, xmf3Edge2);
}

FieldMapGridMesh::FieldMapGridMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	INT xStart, INT zStart, INT width, INT length, XMFLOAT3 scale, FieldMapImage* heightMapImage)
{
	//격자는 삼각형 스트립으로 구성한다. . 
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	m_width = width;
	m_length = length;
	m_scale = scale;

	vector<DetailVertex> vertices;

	/*xStart와 zStart는 격자의 시작 위치(x-좌표와 z-좌표)를 나타낸다. 커다란 지형은 격자들의 이차원 배열로 만들 필
	요가 있기 때문에 전체 지형에서 각 격자의 시작 위치를 나타내는 정보가 필요하다.*/
	for (int i = 0, z = zStart; z < (zStart + length); ++z) {
		for (int x = xStart; x < (xStart + width); ++x, ++i)
		{
			//정점의 높이와 색상을 높이 맵으로부터 구한다.
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
			//홀수 번째 줄이므로(z = 0, 2, 4, ...) 인덱스의 나열 순서는 왼쪽에서 오른쪽 방향이다. 
			for (int x = 0; x < width; x++)
			{
				//첫 번째 줄을 제외하고 줄이 바뀔 때마다(x == 0) 첫 번째 인덱스를 추가한다. 
				if (x == 0 && z > 0) indices.push_back(x + (z * width));
				//아래(x, z), 위(x, z+1)의 순서로 인덱스를 추가한다. 
				indices.push_back(x + (z * width));
				indices.push_back(x + (z * width) + width);
			}
		}
		else
		{
			//짝수 번째 줄이므로(z = 1, 3, 5, ...) 인덱스의 나열 순서는 오른쪽에서 왼쪽 방향이다. 
			for (int x = width - 1; x >= 0; --x)
			{
				//줄이 바뀔 때마다(x == (nWidth-1)) 첫 번째 인덱스를 추가한다. 
				if (x == width - 1) indices.push_back(x + (z * width));
				//아래(x, z), 위(x, z+1)의 순서로 인덱스를 추가한다. 
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

AnimationMesh::AnimationMesh()
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void AnimationMesh::Render(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, GameObject* rootObject, const GameObject* object)
{
	UpdateShaderVariables(device, commandList, rootObject, object);

	commandList->IASetPrimitiveTopology(m_primitiveTopology);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

	if (0 < m_nSubMeshes && subMeshIndex < m_nSubMeshes) {
		commandList->IASetIndexBuffer(&m_subsetIndexBufferViews[subMeshIndex]);
		commandList->DrawIndexedInstanced(m_vSubsetIndices[subMeshIndex], 1, 0, 0, 0);
	}
	else if (m_nIndices) {
		commandList->IASetIndexBuffer(&m_indexBufferView);
		commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else {
		commandList->DrawInstanced(m_nVertices, 1, 0, 0);
	}
}

void AnimationMesh::CreateShaderVariables(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* rootObject)
{
	if (m_animationTransformBuffer.contains(rootObject))
		return;

	UINT elementBytes = (((sizeof(XMFLOAT4X4) * AnimationSetting::MAX_BONE) + 255) & ~255);
	ComPtr<ID3D12Resource> buffer = CreateBufferResource(device, commandList, nullptr, elementBytes,
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ComPtr<ID3D12Resource>());

	XMFLOAT4X4* bufferPointer{};
	buffer->Map(0, nullptr, (void**)&bufferPointer);

	m_animationTransformBuffer.insert({ rootObject, make_pair(buffer, bufferPointer) });
}

void AnimationMesh::UpdateShaderVariables(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* rootObject, const GameObject* object)
{
	// 오프셋 행렬은 그대로 넘긴다 -> 고정된 값이므로

	// 애니메이션 변환행렬은 오브젝트에서 가져와서 넘긴다
	// -> 모델 별로 애니메이션이 다르기 때문에

	XMFLOAT4X4 worldMatrix;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&rootObject->GetWorldMatrix())));
	commandList->SetGraphicsRoot32BitConstants(0, 16, &worldMatrix, 0);

	// 바인드 포즈 넘기기
	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_bindPoseBoneOffsetBuffers->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(16, virtualAddress);


	// 애니메이션 메쉬는 해시맵에서 이름을 통해 찾아 변환행렬을 채움
	if (AnimationSetting::ANIMATION_MESH == m_meshType) {
		for (size_t i = 0; const string & boneName : m_skinningBoneNames) {
			XMFLOAT4X4 transform;
			XMStoreFloat4x4(&transform, XMMatrixTranspose(XMLoadFloat4x4(
				&rootObject->GetAnimationController()->GetGameObject(boneName)->GetAnimationMatrix())));
			::memcpy(&m_animationTransformBuffer[rootObject].second[i], &transform, sizeof(XMFLOAT4X4));
			++i;
		}
	}

	// 일반 메쉬의 경우 넘겨받은 오브젝트로 직접 찾아서 해당 변환행렬만을 찾아옴
	else {
		XMFLOAT4X4 transform;
		XMStoreFloat4x4(&transform, XMMatrixTranspose(XMLoadFloat4x4(
			&rootObject->GetAnimationController()->GetGameObject(object->GetFrameName())->GetAnimationMatrix())));
		::memcpy(&m_animationTransformBuffer[rootObject].second, &transform, sizeof(XMFLOAT4X4));
	}

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_animationTransformBuffer[rootObject].first->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(17, virtualAddress);
	
}

void AnimationMesh::ReleaseUploadBuffer()
{
	if (m_vertexUploadBuffer) m_vertexUploadBuffer.Reset();
	if (m_indexUploadBuffer) m_indexUploadBuffer.Reset();
	for (auto& subsetUploadBuffer : m_subsetIndexUploadBuffers) {
		subsetUploadBuffer.Reset();
	}
}

void AnimationMesh::LoadAnimationMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_nIndices = 0;

	vector<AnimationVertex> vertices;
	vector<UINT> indices;

	BYTE strLength{};

	INT positionNum{}, colorNum{}, texcoord0Num{}, texcoord1Num{}, normalNum{}, tangentNum{}, biTangentNum{};
	INT boneNameNum{}, boneOffsetNum{}, boneIndexNum{}, boneWeightNum{};

	in.read((char*)(&strLength), sizeof(BYTE));
	m_animationMeshName.resize(strLength, '\0');
	in.read(&m_animationMeshName[0], sizeof(char) * strLength);

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read(&strToken[0], sizeof(char) * strLength);

		if (strToken == "<BonesPerVertex>:") {
			INT dummy{};
			in.read((char*)(&dummy), sizeof(INT));
		}
		else if (strToken == "<Bounds>:") {
			in.read((char*)(&m_boundingBox.Center), sizeof(XMFLOAT3));
			in.read((char*)(&m_boundingBox.Extents), sizeof(XMFLOAT3));
		}
		else if (strToken == "<BoneNames>:") {
			in.read((char*)(&boneNameNum), sizeof(INT));

			m_skinningBoneNames.resize(boneNameNum);

			for (int i = 0; i < boneNameNum; ++i) {
				in.read((char*)(&strLength), sizeof(BYTE));
				m_skinningBoneNames[i].resize(strLength);
				in.read(&m_skinningBoneNames[i][0], sizeof(char) * strLength);
			}


		}
		else if (strToken == "<BoneOffsets>:") {
			in.read((char*)(&boneOffsetNum), sizeof(INT));

			if (boneOffsetNum > 0) {
				m_bindPoseBoneOffsets.resize(boneOffsetNum);

				for (int i = 0; i < boneOffsetNum; ++i)
					in.read((char*)(&m_bindPoseBoneOffsets[i]), sizeof(XMFLOAT4X4));

				//바인드포즈 오프셋 버퍼 정의
				UINT elementBytes = (((sizeof(XMFLOAT4X4) * AnimationSetting::MAX_BONE) + 255) & ~255);	// // XMFLOAT4X4 가 최대 뼈의 갯수만큼 있는것을 가정하였을 때의 256의 배수 구하기
				m_bindPoseBoneOffsetBuffers = CreateBufferResource(device, commandList, nullptr, elementBytes, D3D12_HEAP_TYPE_UPLOAD,
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ComPtr<ID3D12Resource>());
				m_bindPoseBoneOffsetBuffers->Map(0, nullptr, (void**)&m_bindPoseBoneOffsetBuffersPointer);

				for (int i = 0; i < boneOffsetNum; ++i) {
					XMStoreFloat4x4(&m_bindPoseBoneOffsetBuffersPointer[i], XMMatrixTranspose(XMLoadFloat4x4(&m_bindPoseBoneOffsets[i])));
				}
				
			}
		}
		else if (strToken == "<BoneIndices>:") {
			in.read((char*)(&boneIndexNum), sizeof(INT));
			if (vertices.size() < boneIndexNum) {
				m_nVertices = boneIndexNum;
				vertices.resize(boneIndexNum);
			}
			for (int i = 0; i < boneIndexNum; ++i) {
				in.read((char*)(&vertices[i].boneIndex), sizeof(XMINT4));
			}
		}
		else if (strToken == "<BoneWeights>:") {
			in.read((char*)(&boneWeightNum), sizeof(INT));
			if (vertices.size() < boneWeightNum) {
				m_nVertices = boneWeightNum;
				vertices.resize(boneWeightNum);
			}
			for (int i = 0; i < boneWeightNum; ++i) {
				in.read((char*)(&vertices[i].boneWeight), sizeof(XMFLOAT4));
			}
		}
		if (strToken == "<Mesh>:") {
			in.read((char*)(&strLength), sizeof(BYTE));
			m_meshName.resize(strLength, '\0');
			in.read(&m_meshName[0], sizeof(char) * strLength);
		}
		else if (strToken == "<Positions>:") {
			in.read((char*)(&positionNum), sizeof(INT));
			if (vertices.size() < positionNum) {
				m_nVertices = positionNum;
				vertices.resize(positionNum);
			}
			for (int i = 0; i < positionNum; ++i) {
				in.read((char*)(&vertices[i].position), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Colors>:") {
			XMFLOAT4 dummy;
			in.read((char*)(&colorNum), sizeof(INT));
			for (int i = 0; i < colorNum; ++i) {
				in.read((char*)(&dummy), sizeof(XMFLOAT4));
			}
		}
		else if (strToken == "<TextureCoords0>:") {
			in.read((char*)(&texcoord0Num), sizeof(INT));
			if (vertices.size() < texcoord0Num) {
				m_nVertices = texcoord0Num;
				vertices.resize(texcoord0Num);
			}
			for (int i = 0; i < texcoord0Num; ++i) {
				in.read((char*)(&vertices[i].uv0), sizeof(XMFLOAT2));
			}
		}
		else if (strToken == "<TextureCoords1>:") {
			XMFLOAT2 dummy;
			in.read((char*)(&texcoord1Num), sizeof(INT));
			for (int i = 0; i < texcoord1Num; ++i) {
				in.read((char*)(&dummy), sizeof(XMFLOAT2));
			}
		}
		else if (strToken == "<Normals>:") {
			in.read((char*)(&normalNum), sizeof(INT));
			if (vertices.size() < normalNum) {
				m_nVertices = normalNum;
				vertices.resize(normalNum);
			}
			for (int i = 0; i < normalNum; ++i) {
				in.read((char*)(&vertices[i].normal), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<Tangents>:") {
			in.read((char*)(&tangentNum), sizeof(INT));
			if (vertices.size() < tangentNum) {
				m_nVertices = tangentNum;
				vertices.resize(tangentNum);
			}
			for (int i = 0; i < tangentNum; ++i) {
				in.read((char*)(&vertices[i].tangent), sizeof(XMFLOAT3));
			}
		}
		else if (strToken == "<BiTangents>:") {
			in.read((char*)(&biTangentNum), sizeof(INT));
			if (vertices.size() < biTangentNum) {
				m_nVertices = biTangentNum;
				vertices.resize(biTangentNum);
			}
			for (int i = 0; i < biTangentNum; ++i) {
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
		sizeof(AnimationVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(AnimationVertex);
	m_vertexBufferView.SizeInBytes = sizeof(AnimationVertex) * vertices.size();

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

UIMesh::UIMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nVertices = 6;
	m_nIndices = 0;

	vector<TextureVertex> vertices;

	vertices.emplace_back(XMFLOAT3(-1.f, -1.f, +0.f), XMFLOAT2(0.f, 1.f));
	vertices.emplace_back(XMFLOAT3(-1.f, +1.f, +0.f), XMFLOAT2(0.f, 0.f));
	vertices.emplace_back(XMFLOAT3(+1.f, -1.f, +0.f), XMFLOAT2(1.f, 1.f));
	vertices.emplace_back(XMFLOAT3(+1.f, -1.f, +0.f), XMFLOAT2(1.f, 1.f));
	vertices.emplace_back(XMFLOAT3(-1.f, +1.f, +0.f), XMFLOAT2(0.f, 0.f));
	vertices.emplace_back(XMFLOAT3(+1.f, +1.f, +0.f), XMFLOAT2(1.f, 0.f));

	m_nVertices = vertices.size();
	m_vertexBuffer = CreateBufferResource(device, commandList, vertices.data(),
		sizeof(TextureVertex) * vertices.size(), D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(TextureVertex);
	m_vertexBufferView.SizeInBytes = sizeof(TextureVertex) * vertices.size();
}
