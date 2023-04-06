#include "texture.h"

void Texture::LoadTextureFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const wstring& fileName, UINT rootParameterIndex)
{
	m_cbvsrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	ComPtr<ID3D12Resource> textureBuffer;
	ComPtr<ID3D12Resource> textureUploadHeap;

	unique_ptr<uint8_t[]> ddsData;
	vector<D3D12_SUBRESOURCE_DATA> subresources;
	DDS_ALPHA_MODE ddsAlphaMode{ DDS_ALPHA_MODE_UNKNOWN };
	DX::ThrowIfFailed(DirectX::LoadDDSTextureFromFileEx(device.Get(), fileName.c_str(), 0,
		D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, &textureBuffer, ddsData, subresources, &ddsAlphaMode));

	UINT nSubresources{ (UINT)subresources.size() };
	const UINT64 nBytes{ GetRequiredIntermediateSize(textureBuffer.Get(), 0, nSubresources) };

	// Create the GPU upload buffer.
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&textureUploadHeap)
	));

	UpdateSubresources(commandList.Get(), textureBuffer.Get(), textureUploadHeap.Get(), 0, 0, nSubresources, subresources.data());

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	m_textures.push_back(make_pair(textureBuffer, rootParameterIndex));
	m_textureUploadHeap.push_back(textureUploadHeap);
}

void Texture::CreateSrvDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
	// SRV�� �����ϱ� ���� SRV ������ ���� �����Ѵ�.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = (UINT)m_textures.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(
		&srvHeapDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)));
}

void Texture::CreateShaderResourceView(const ComPtr<ID3D12Device>& device, INT viewDemention)
{
	// SRV�� �����Ѵ�.

	// ���� ������ ����Ű�� �����͸� ��´�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDesciptor(
		m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (const auto& texture : m_textures) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texture.first->GetDesc().Format;

		switch (viewDemention)
		{
		case D3D12_SRV_DIMENSION_TEXTURE2D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = texture.first->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			break;
		case D3D12_SRV_DIMENSION_TEXTURECUBE:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = 1;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			break;
		}
		device->CreateShaderResourceView(texture.first.Get(), &srvDesc, hDesciptor);
		
		hDesciptor.Offset(1, m_cbvsrvDescriptorSize);
	}
}

void Texture::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE hDesciptor{ m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart() };
	for (const auto& index : m_textures)
	{
		commandList->SetGraphicsRootDescriptorTable(index.second, hDesciptor);
		hDesciptor.ptr += m_cbvsrvDescriptorSize;
	}
}

void Texture::ReleaseUploadBuffer()
{
	for (auto& textureUploadBuffer : m_textureUploadHeap)
		textureUploadBuffer.Reset();
}

bool Texture::LoadTextureFileHierarchy(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, ifstream& in, UINT rootParameterIndex)
{
	BYTE strLength;
	in.read((char*)(&strLength), sizeof(BYTE));
	string strToken(strLength, '\0');
	in.read((&strToken[0]), sizeof(char) * strLength);

	if (strToken != "null") {
		wstring wstrToken = L"";
		wstrToken.assign(strToken.begin(), strToken.end());
		wstring strPath = L"./Resource/Texture/" + wstrToken + L".dds";
		if (strToken[0] != '@') {
			LoadTextureFile(device, commandList, strPath, rootParameterIndex);
		}
		else {
			strPath.erase(find(strPath.begin(), strPath.end(), '@'));
			LoadTextureFile(device, commandList, strPath, rootParameterIndex);
		}
		return true;
	}
	return false;
}