#include "material.h"

void Material::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(MaterialInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_materialBuffer)));

	// 재질 버퍼 포인터
	m_materialBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_materialBufferPointer));
}

void Material::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	XMFLOAT4 albedoColor = m_albedoColor;
	::memcpy(&m_materialBufferPointer->albedoColor, &albedoColor, sizeof(XMFLOAT4));

	XMFLOAT4 emissiveColor = m_emissiveColor;
	::memcpy(&m_materialBufferPointer->emissiveColor, &emissiveColor, sizeof(XMFLOAT4));

	XMFLOAT4 specularColor = m_specularColor;
	::memcpy(&m_materialBufferPointer->specularColor, &specularColor, sizeof(XMFLOAT4));

	XMFLOAT4 ambientColor = m_ambientColor;
	::memcpy(&m_materialBufferPointer->ambientColor, &ambientColor, sizeof(XMFLOAT4));

	FLOAT glossiness = m_glossiness;
	::memcpy(&m_materialBufferPointer->glossiness, &glossiness, sizeof(FLOAT));

	FLOAT smoothness = m_smoothness;
	::memcpy(&m_materialBufferPointer->smoothness, &smoothness, sizeof(FLOAT));

	FLOAT metallic = m_metallic;
	::memcpy(&m_materialBufferPointer->metallic, &metallic, sizeof(FLOAT));

	FLOAT specularHighlight = m_specularHighlight;
	::memcpy(&m_materialBufferPointer->specularHighlight, &specularHighlight, sizeof(FLOAT));

	FLOAT glossyReflection = m_glossyReflection;
	::memcpy(&m_materialBufferPointer->glossyReflection, &glossyReflection, sizeof(FLOAT));

	UINT type = m_type;
	::memcpy(&m_materialBufferPointer->type, &type, sizeof(UINT));

	D3D12_GPU_VIRTUAL_ADDRESS virtualAddress = m_materialBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(2, virtualAddress);

	if (m_type & MATERIAL_ALBEDO_MAP) m_albedoMap->UpdateShaderVariable(commandList);
	if (m_type & MATERIAL_SPECULAR_MAP) m_specularMap->UpdateShaderVariable(commandList);
	if (m_type & MATERIAL_NORMAL_MAP) m_normalMap->UpdateShaderVariable(commandList);
	if (m_type & MATERIAL_METALLIC_MAP) m_metallicMap->UpdateShaderVariable(commandList);
	if (m_type & MATERIAL_EMISSION_MAP) m_emissionMap->UpdateShaderVariable(commandList);
}

void Materials::LoadMaterials(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in)
{
	BYTE strLength;
	INT materialCount, materialIndex;

	while (1) {
		in.read((char*)(&strLength), sizeof(BYTE));
		string strToken(strLength, '\0');
		in.read((&strToken[0]), sizeof(char) * strLength);

		if (strToken == "<Materials>:") {
			in.read((char*)(&materialCount), sizeof(INT));
			m_materials.resize(materialCount);
		}
		else if (strToken == "<Material>:") {
			in.read((char*)(&materialIndex), sizeof(INT));
			m_materials[materialIndex].CreateShaderVariable(device, commandList);
		}
		else if (strToken == "<AlbedoColor>:") {
			in.read((char*)(&m_materials[materialIndex].m_albedoColor), sizeof(XMFLOAT4));
		}
		else if (strToken == "<EmissiveColor>:") {
			in.read((char*)(&m_materials[materialIndex].m_emissiveColor), sizeof(XMFLOAT4));
		}
		else if (strToken == "<SpecularColor>:") {
			in.read((char*)(&m_materials[materialIndex].m_specularColor), sizeof(XMFLOAT4));
		}
		else if (strToken == "<Glossiness>:") {
			in.read((char*)(&m_materials[materialIndex].m_glossiness), sizeof(FLOAT));
		}
		else if (strToken == "<Smoothness>:") {
			in.read((char*)(&m_materials[materialIndex].m_smoothness), sizeof(FLOAT));
		}
		else if (strToken == "<Metallic>:") {
			in.read((char*)(&m_materials[materialIndex].m_metallic), sizeof(FLOAT));
		}
		else if (strToken == "<SpecularHighlight>:") {
			in.read((char*)(&m_materials[materialIndex].m_specularHighlight), sizeof(FLOAT));
		}
		else if (strToken == "<GlossyReflection>:") {
			in.read((char*)(&m_materials[materialIndex].m_glossyReflection), sizeof(FLOAT));
		}
		else if (strToken == "<AlbedoMap>:") {
			m_materials[materialIndex].m_albedoMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_albedoMap->LoadTextureFileHierarchy(device, commandList, in, 7)) {
				m_materials[materialIndex].m_type |= MATERIAL_ALBEDO_MAP;
				m_materials[materialIndex].m_albedoMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_albedoMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_albedoMap = nullptr;
		}
		else if (strToken == "<SpecularMap>:") {
			m_materials[materialIndex].m_specularMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_specularMap->LoadTextureFileHierarchy(device, commandList, in, 8)) {
				m_materials[materialIndex].m_type |= MATERIAL_SPECULAR_MAP;
				m_materials[materialIndex].m_specularMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_specularMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_specularMap = nullptr;
		}
		else if (strToken == "<NormalMap>:") {
			m_materials[materialIndex].m_normalMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_normalMap->LoadTextureFileHierarchy(device, commandList, in, 9)) {
				m_materials[materialIndex].m_type |= MATERIAL_NORMAL_MAP;
				m_materials[materialIndex].m_normalMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_normalMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_normalMap = nullptr;
		}
		else if (strToken == "<MetallicMap>:") {
			m_materials[materialIndex].m_metallicMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_metallicMap->LoadTextureFileHierarchy(device, commandList, in, 10)) {
				m_materials[materialIndex].m_type |= MATERIAL_METALLIC_MAP;
				m_materials[materialIndex].m_metallicMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_metallicMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_metallicMap = nullptr;
		}
		else if (strToken == "<EmissionMap>:") {
			m_materials[materialIndex].m_emissionMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_emissionMap->LoadTextureFileHierarchy(device, commandList, in, 11)) {
				m_materials[materialIndex].m_type |= MATERIAL_EMISSION_MAP;
				m_materials[materialIndex].m_emissionMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_emissionMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_emissionMap = nullptr;
		}
		else if (strToken == "<DetailAlbedoMap>:") {
			m_materials[materialIndex].m_detailAlbedoMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_detailAlbedoMap->LoadTextureFileHierarchy(device, commandList, in, 12)) {
				m_materials[materialIndex].m_type |= MATERIAL_DETAIL_ALBEDO_MAP;
				m_materials[materialIndex].m_detailAlbedoMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_detailAlbedoMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_detailAlbedoMap = nullptr;
		}
		else if (strToken == "<DetailNormalMap>:") {
			m_materials[materialIndex].m_detailNormalMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_detailNormalMap->LoadTextureFileHierarchy(device, commandList, in, 13)) {
				m_materials[materialIndex].m_type |= MATERIAL_DETAIL_NORMAL_MAP;
				m_materials[materialIndex].m_detailNormalMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_detailNormalMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
			else m_materials[materialIndex].m_detailNormalMap = nullptr;
		}
		else if (strToken == "</Materials>") {
			break;
		}
	}
}
