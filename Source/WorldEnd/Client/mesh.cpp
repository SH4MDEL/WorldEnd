#include "mesh.h"
#include "object.h"
#include "framework.h"

Mesh::Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const vector<TextureVertex>& vertices, const vector<UINT>& indices)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 정점 버퍼 생성
	m_nVertices = (UINT)vertices.size();
	const UINT vertexBufferSize = (UINT)sizeof(TextureVertex) * (UINT)vertices.size();

	Utiles::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		NULL,
		IID_PPV_ARGS(&m_vertexBuffer)));

	Utiles::ThrowIfFailed(device->CreateCommittedResource(
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

		Utiles::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(&m_indexBuffer)));

		Utiles::ThrowIfFailed(device->CreateCommittedResource(
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
void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& m_commandList, UINT subMeshIndex, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const { }

void Mesh::ReleaseUploadBuffer()
{
	if (m_vertexUploadBuffer) m_vertexUploadBuffer.Reset();
	if (m_indexUploadBuffer) m_indexUploadBuffer.Reset();
}

MeshFromFile::MeshFromFile()
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void MeshFromFile::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex) const
{
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

void MeshFromFile::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView) const
{
	commandList->IASetPrimitiveTopology(m_primitiveTopology);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetVertexBuffers(1, 1, &instanceBufferView);

	if (0 < m_nSubMeshes && subMeshIndex < m_nSubMeshes) {
		commandList->IASetIndexBuffer(&m_subsetIndexBufferViews[subMeshIndex]);
		commandList->DrawIndexedInstanced(m_vSubsetIndices[subMeshIndex], instanceBufferView.SizeInBytes / instanceBufferView.StrideInBytes, 0, 0, 0);
	}
	else if (m_nIndices) {
		commandList->IASetIndexBuffer(&m_indexBufferView);
		commandList->DrawIndexedInstanced(m_nIndices, instanceBufferView.SizeInBytes / instanceBufferView.StrideInBytes, 0, 0, 0);
	}
	else {
		commandList->DrawInstanced(m_nVertices, instanceBufferView.SizeInBytes / instanceBufferView.StrideInBytes, 0, 0);
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

AnimationMesh::AnimationMesh()
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void AnimationMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT subMeshIndex, GameObject* rootObject, const GameObject* object)
{
	UpdateShaderVariables(commandList, rootObject, object);

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

void AnimationMesh::CreateShaderVariables(GameObject* rootObject)
{
	if (m_animationTransformBuffers.contains(rootObject))
		return;
	
	UINT elementBytes = (((sizeof(XMFLOAT4X4) * AnimationSetting::MAX_BONE) + 255) & ~255);
	ComPtr<ID3D12Resource> buffer = CreateBufferResource(g_GameFramework.GetDevice(), 
		g_GameFramework.GetCommandList(), nullptr, elementBytes,
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ComPtr<ID3D12Resource>());

	XMFLOAT4X4* bufferPointer{};
	buffer->Map(0, nullptr, (void**)&bufferPointer);

	m_animationTransformBuffers.insert({ rootObject, make_pair(buffer, bufferPointer) });
}

void AnimationMesh::UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* rootObject, const GameObject* object)
{
	// 오프셋 행렬은 그대로 넘긴다 -> 고정된 값이므로

	// 애니메이션 변환행렬은 오브젝트에서 가져와서 넘긴다
	// -> 모델 별로 애니메이션이 다르기 때문에

	XMFLOAT4X4 worldMatrix;
	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&rootObject->GetWorldMatrix())));
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 16, &worldMatrix, 0);

	// 애니메이션 메쉬는 해시맵에서 이름을 통해 찾아 변환행렬을 채움
	if (AnimationSetting::ANIMATION_MESH == m_meshType) {
		D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_bindPoseBoneOffsetBuffers->GetGPUVirtualAddress();
		commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::BoneOffset, virtualAddress);

		for (size_t i = 0; const string & boneName : m_skinningBoneNames) {
			XMFLOAT4X4 transform;
			XMStoreFloat4x4(&transform, XMMatrixTranspose(XMLoadFloat4x4(
				&rootObject->GetAnimationController()->GetGameObject(boneName)->GetAnimationMatrix())));
			::memcpy(&m_animationTransformBuffers[rootObject].second[i], &transform, sizeof(XMFLOAT4X4));
			++i;
		}

		virtualAddress = m_animationTransformBuffers[rootObject].first->GetGPUVirtualAddress();
		commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::BoneTransform, virtualAddress);
	}

	// 일반 메쉬의 경우 넘겨받은 오브젝트로 직접 찾아서 해당 변환행렬만을 찾아옴
	else {
		XMFLOAT4X4 transform;
		XMStoreFloat4x4(&transform, XMMatrixTranspose(XMLoadFloat4x4(
			&rootObject->GetAnimationController()->GetGameObject(object->GetFrameName())->GetAnimationMatrix())));
		::memcpy(&m_animationTransformBuffers[rootObject].second[0], &transform, sizeof(XMFLOAT4X4));

		D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_animationTransformBuffers[rootObject].first->GetGPUVirtualAddress();
		commandList->SetGraphicsRootConstantBufferView((INT)ShaderRegister::BoneTransform, virtualAddress);
	}

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

		Utiles::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(&m_indexBuffer)));

		Utiles::ThrowIfFailed(device->CreateCommittedResource(
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

PlaneMesh::PlaneMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, XMFLOAT3 position, XMFLOAT2 size)
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