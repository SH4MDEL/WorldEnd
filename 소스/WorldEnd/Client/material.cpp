#include "material.h"

void Material::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	commandList->SetGraphicsRoot32BitConstants(0, 4, &(m_albedoColor), 16);
	commandList->SetGraphicsRoot32BitConstants(0, 4, &(m_emissiveColor), 20);
	commandList->SetGraphicsRoot32BitConstants(0, 4, &(m_specularColor), 24);
	commandList->SetGraphicsRoot32BitConstants(0, 4, &(m_ambientColor), 28);

	commandList->SetGraphicsRoot32BitConstants(0, 1, &(m_type), 32);

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
			in.read((char*)(&m_materials[materialIndex].m_metalic), sizeof(FLOAT));
		}
		else if (strToken == "<SpecularHighlight>:") {
			in.read((char*)(&m_materials[materialIndex].m_specularHighlight), sizeof(FLOAT));
		}
		else if (strToken == "<GlossyReflection>:") {
			in.read((char*)(&m_materials[materialIndex].m_glossyReflection), sizeof(FLOAT));
		}
		else if (strToken == "<AlbedoMap>:") {
			m_materials[materialIndex].m_albedoMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_albedoMap->LoadTextureFileHierarchy(device, commandList, in, 6)) {
				m_materials[materialIndex].m_type |= MATERIAL_ALBEDO_MAP;
				m_materials[materialIndex].m_albedoMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_albedoMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "<SpecularMap>:") {
			m_materials[materialIndex].m_specularMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_specularMap->LoadTextureFileHierarchy(device, commandList, in, 7)) {
				m_materials[materialIndex].m_type |= MATERIAL_SPECULAR_MAP;
				m_materials[materialIndex].m_specularMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_specularMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "<NormalMap>:") {
			m_materials[materialIndex].m_normalMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_normalMap->LoadTextureFileHierarchy(device, commandList, in, 8)) {
				m_materials[materialIndex].m_type |= MATERIAL_NORMAL_MAP;
				m_materials[materialIndex].m_normalMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_normalMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "<MetallicMap>:") {
			m_materials[materialIndex].m_metallicMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_metallicMap->LoadTextureFileHierarchy(device, commandList, in, 9)) {
				m_materials[materialIndex].m_type |= MATERIAL_METALLIC_MAP;
				m_materials[materialIndex].m_metallicMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_metallicMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "<EmissionMap>:") {
			m_materials[materialIndex].m_emissionMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_emissionMap->LoadTextureFileHierarchy(device, commandList, in, 10)) {
				m_materials[materialIndex].m_type |= MATERIAL_EMISSION_MAP;
				m_materials[materialIndex].m_emissionMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_emissionMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "<DetailAlbedoMap>:") {
			m_materials[materialIndex].m_detailAlbedoMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_detailAlbedoMap->LoadTextureFileHierarchy(device, commandList, in, 11)) {
				m_materials[materialIndex].m_type |= MATERIAL_DETAIL_ALBEDO_MAP;
				m_materials[materialIndex].m_detailAlbedoMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_detailAlbedoMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "<DetailNormalMap>:") {
			m_materials[materialIndex].m_detailNormalMap = make_shared<Texture>();
			if (m_materials[materialIndex].m_detailNormalMap->LoadTextureFileHierarchy(device, commandList, in, 12)) {
				m_materials[materialIndex].m_type |= MATERIAL_DETAIL_NORMAL_MAP;
				m_materials[materialIndex].m_detailNormalMap->CreateSrvDescriptorHeap(device);
				m_materials[materialIndex].m_detailNormalMap->CreateShaderResourceView(device, D3D12_SRV_DIMENSION_TEXTURE2D);
			}
		}
		else if (strToken == "</Materials>") {
			break;
		}
	}
}
