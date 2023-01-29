// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#pragma warning (disable : 4996)
#pragma comment(lib, "ws2_32")

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

//#define USE_NETWORK

// Windows 헤더 파일
#include <Windows.h>

// C/C++ 런타임 헤더 파일입니다.
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <shellapi.h>
#include <wrl.h>
#include <algorithm>
#include <array>


// d3d12 헤더 파일입니다.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "d3dx12.h"
using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

#include "../Server/protocol.h"

class GameFramework;

extern GameFramework       g_GameFramework;

extern SOCKET               g_socket;                           // 소켓
extern string				g_serverIP;							// 서버 아이피
extern thread               g_networkThread;
extern mutex                g_mutex;

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception{};
        }
    }
}

namespace Vector3
{
    inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMFLOAT3{ a.x + b.x, a.y + b.y, a.z + b.z };
    }
    inline XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMFLOAT3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }
    inline XMFLOAT3 Mul(const XMFLOAT3& a, const FLOAT& scalar)
    {
        return XMFLOAT3{ a.x * scalar, a.y * scalar, a.z * scalar };
    }
    inline FLOAT Dot(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVector3Cross(XMLoadFloat3(&a), XMLoadFloat3(&b)));
        return result;
    }
    inline XMFLOAT3 Normalize(const XMFLOAT3& a)
    {
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVector3Normalize(XMLoadFloat3(&a)));
        return result;
    }
    inline FLOAT Length(const XMFLOAT3& a)
    {
        XMFLOAT3 result;
        XMVECTOR v{ XMVector3Length(XMLoadFloat3(&a)) };
        XMStoreFloat3(&result, v);
        return result.x;
    }
}

namespace Matrix
{
    inline XMFLOAT4X4 Scale(const XMFLOAT4X4& a, float scale)
    {
        XMFLOAT4X4 result;
        XMMATRIX x{ XMLoadFloat4x4(&a) };
        XMStoreFloat4x4(&result, x * scale);
        return result;
    }
    inline XMFLOAT4X4 Add(const XMFLOAT4X4& a, const XMFLOAT4X4& b)
    {
        XMFLOAT4X4 result;
        XMMATRIX x{ XMLoadFloat4x4(&a) };
        XMMATRIX y{ XMLoadFloat4x4(&b) };
        XMStoreFloat4x4(&result, x + y);
        return result;
    }
    inline XMFLOAT4X4 Add(const XMMATRIX& a, const XMFLOAT4X4& b)
    {
        XMFLOAT4X4 result;
        XMMATRIX x{ a };
        XMMATRIX y{ XMLoadFloat4x4(&b) };
        XMStoreFloat4x4(&result, x + y);
        return result;
    }
    inline XMFLOAT4X4 Mul(const XMFLOAT4X4& a, const XMFLOAT4X4& b)
    {
        XMFLOAT4X4 result;
        XMMATRIX x{ XMLoadFloat4x4(&a) };
        XMMATRIX y{ XMLoadFloat4x4(&b) };
        XMStoreFloat4x4(&result, x * y);
        return result;
    }
    inline XMFLOAT4X4 Mul(const XMMATRIX& a, const XMFLOAT4X4& b)
    {
        XMFLOAT4X4 result;
        XMMATRIX x{ a };
        XMMATRIX y{ XMLoadFloat4x4(&b) };
        XMStoreFloat4x4(&result, x * y);
        return result;
    }
    inline XMFLOAT4X4 Transpose(const XMFLOAT4X4& a)
    {
        XMFLOAT4X4 result;
        XMMATRIX m{ XMMatrixTranspose(XMLoadFloat4x4(&a)) };
        XMStoreFloat4x4(&result, m);
        return result;
    }

    inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& a)
    {
        XMFLOAT4X4 result;
        XMMATRIX m{ XMMatrixInverse(NULL, XMLoadFloat4x4(&a)) };
        XMStoreFloat4x4(&result, m);
        return result;
    }

    inline XMFLOAT4X4 Interpolate(const XMFLOAT4X4& a, const XMFLOAT4X4& b, float t) 
    {
        XMFLOAT4X4 result;
        XMVECTOR S0, R0, T0;
        XMVECTOR S1, R1, T1;
        XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&a));
        XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&b));
        XMVECTOR S = XMVectorLerp(S0, S1, t);
        XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
        XMVECTOR T = XMVectorLerp(T0, T1, t);
        XMStoreFloat4x4(&result, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
        return result;
    }
}

ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
    const void* data, UINT byte, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& uploadBuffer);

// 서버 관련

void ErrorQuit(const char* msg);
void ErrorDisplay(const char* msg);

namespace Setting
{
    constexpr auto MAX_PLAYERS = 2;    // 최대 플레이어 수(본인 제외)
}