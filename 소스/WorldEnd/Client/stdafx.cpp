#include "stdafx.h"
#include "framework.h"

GameFramework       g_GameFramework(1280, 720);

SOCKET                  g_socket{};
string					g_serverIP{ "127.0.0.1" };
thread                  g_networkThread{};
mutex                   g_mutex{};


ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
	const void* data, UINT byte, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> resourceBuffer;

	if (data)
	{
		switch (heapType)
		{
		case D3D12_HEAP_TYPE_DEFAULT:
		{
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(byte),
				D3D12_RESOURCE_STATE_COPY_DEST,
				NULL,
				IID_PPV_ARGS(&resourceBuffer)));

			DX::ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(byte),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				NULL,
				IID_PPV_ARGS(&uploadBuffer)));

			D3D12_SUBRESOURCE_DATA bufferData{};
			bufferData.pData = data;
			bufferData.RowPitch = byte;
			bufferData.SlicePitch = bufferData.RowPitch;
			UpdateSubresources<1>(commandList.Get(), resourceBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &bufferData);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resourceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, resourceState));
			return resourceBuffer;
		}
		case D3D12_HEAP_TYPE_UPLOAD:
		{
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(byte),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				NULL,
				IID_PPV_ARGS(&resourceBuffer)));

			UINT8* pBufferDataBegin{ NULL };
			CD3DX12_RANGE readRange{ 0, 0 };
			DX::ThrowIfFailed(resourceBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pBufferDataBegin)));
			memcpy(pBufferDataBegin, data, byte);
			resourceBuffer->Unmap(0, NULL);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resourceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, resourceState));
			return resourceBuffer;
		}
		}
	}
	return nullptr;
}

// ?????? ???? ???? ?????????? ???? ????

void ErrorQuit(const char* msg)
{
	WCHAR* lp_msg_buf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lp_msg_buf), 0, nullptr);
	MessageBox(nullptr, reinterpret_cast<LPCTSTR>(lp_msg_buf), reinterpret_cast<LPCWSTR>(msg), MB_ICONERROR);
	LocalFree(lp_msg_buf);
	exit(1);
}

void ErrorDisplay(const char* msg)
{
	WCHAR* lp_msg_buf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lp_msg_buf), 0, nullptr);
	wcout << "[" << msg << "] " << lp_msg_buf << endl;
	LocalFree(lp_msg_buf);
}