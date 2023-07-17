// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#pragma warning (disable : 4996)
#pragma comment(lib, "ws2_32")

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#define NOMINMAX

//#define USE_NETWORK

// Windows 헤더 파일
#include <Windows.h>

// C/C++ 런타임 헤더 파일입니다.
#include <iostream>
#include <mutex>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stack>
#include <queue>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <shellapi.h>
#include <wrl.h>
#include <algorithm>
#include <array>
#include <functional>
#include <random>

// d3d12 헤더 파일입니다.
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <dxgidebug.h>
#include "d3dx12.h"
using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

#include <dwrite.h>
#include <dwrite_3.h>
#include <d3d11on12.h>
#include <d2d1_3.h>

#include "../Server/protocol.h"
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>

// 사운드 관련
#include "fmod.hpp"

class GameFramework;
class ParticleSystem;
class TowerObjectManager;

struct PlayerInfo {
    INT        id;
    INT        gold;
    XMFLOAT3   position;
    PlayerType playerType;
};


extern GameFramework                    g_GameFramework;
extern mt19937				            g_randomEngine;

extern SOCKET                           g_socket;                           // 소켓
extern string				            g_serverIP;							// 서버 아이피
extern mutex                            g_mutex;
extern PlayerInfo                       g_playerInfo;
extern wstring                          g_loadingText;
extern INT                              g_loadingIndex;
extern unique_ptr<ParticleSystem>       g_particleSystem;
extern stack<function<void()>>          g_clickEventStack;

extern POINT                            g_mousePosition;

constexpr int MAX_PLAYERS = 2;
constexpr int MAX_PARTICLE_COUNT = 10;
constexpr int MAX_PARTICLE_MESH = 50;
// Command list submissions from main thread.
constexpr int COMMANDLIST_NUM = 4;
constexpr int COMMANDLIST_PRE = 0;
constexpr int COMMANDLIST_MID = 1;
constexpr int COMMANDLIST_POST = 2;
constexpr int COMMANDLIST_END = 3;

constexpr int THREAD_NUM = 3;
constexpr int CASCADES_NUM = 3;

enum class ShaderRegister : INT { 
    GameObject, 
    Camera,
    Material,
    BoneOffset,
    BoneTransform,
    Light,
    Scene,
    Framework,
    BaseTexture,
    SubTexture,
    SkyboxTexture,
    ShadowMap,
    AlbedoTexture,
    SpecularTexture,
    NormalTexture,
    MetallicTexture,
    EmissionTexture,
    DetailAlbedoTexture,
    DetailNormalTexture,
    Count
};

enum class DescriptorRange : INT {
    BaseTexture,
    SubTexture,
    SkyboxTexture,
    ShadowMap,
    AlbedoTexture,
    SpecularTexture,
    NormalTexture,
    MetallicTexture,
    EmissionTexture,
    DetailAlbedoTexture,
    DetailNormalTexture,
    Count
};

enum class PostShaderRegister : INT {
    Filter,
    Fade,
    BaseTexture,
    SubTexture,
    OutputTexture,
    Count
};

enum class PostDescriptorRange : INT {
    BaseTexture,
    SubTexture,
    OutputTexture,
    Count
};

namespace Utiles
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception{};
        }
    }
    inline INT GetRandomINT(INT min, INT max)
    {
        uniform_int_distribution<INT> dis{ min, max };
        return dis(g_randomEngine);
    }
    inline FLOAT GetRandomFLOAT(FLOAT min, FLOAT max)
    {
        uniform_real_distribution<FLOAT> dis{ min, max };
        return dis(g_randomEngine);
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
    inline XMFLOAT3 Angle(const XMFLOAT3& a, const XMFLOAT3& b, BOOL isNormalized = true)
    {
        XMFLOAT3 result;
        if (isNormalized) XMStoreFloat3(&result, XMVector3AngleBetweenNormals(XMLoadFloat3(&a), XMLoadFloat3(&b)));
        else XMStoreFloat3(&result, XMVector3AngleBetweenVectors(XMLoadFloat3(&a), XMLoadFloat3(&b)));
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
    inline XMFLOAT3 TransformCoord(const XMFLOAT3& a, const XMFLOAT4X4& b)
    {
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVector3TransformCoord(XMLoadFloat3(&a), XMLoadFloat4x4(&b)));
        return result;
    }
}

namespace Vector4
{
    inline XMFLOAT4 Add(const XMFLOAT4& a, const XMFLOAT4& b)
    {
        return XMFLOAT4{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }
    inline XMFLOAT4 Sub(const XMFLOAT4& a, const XMFLOAT4& b)
    {
        return XMFLOAT4{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }
    inline XMFLOAT4 Mul(const XMFLOAT4& a, const FLOAT& scalar)
    {
        return XMFLOAT4{ a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar };
    }
    inline FLOAT Dot(const XMFLOAT4& a, const XMFLOAT4& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    inline XMFLOAT4 Normalize(const XMFLOAT4& a)
    {
        XMFLOAT4 result;
        XMStoreFloat4(&result, XMVector4Normalize(XMLoadFloat4(&a)));
        return result;
    }
    inline FLOAT Length(const XMFLOAT4& a)
    {
        XMFLOAT4 result;
        XMVECTOR v{ XMVector4Length(XMLoadFloat4(&a)) };
        XMStoreFloat4(&result, v);
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

    inline XMFLOAT4X4 Zero()
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixSet(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f));
        return result;
    }
}

ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
    const void* data, UINT byte, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& uploadBuffer);

ostream& operator<<(ostream& os, const XMFLOAT3& data)
{
    os << "(" << data.x << ", " << data.y << ", " << data.z << ")\n";
    return os;
}
ostream& operator<<(ostream& os, const XMFLOAT4& data)
{
    os << "(" << data.x << ", " << data.y << ", " << data.z << ", " << data.w << ")\n";
    return os;
}
ostream& operator<<(ostream& os, const XMFLOAT4X4& data)
{
    os << "|" << data._11 << ", " << data._12 << ", " << data._13 << ", " << data._14 << "|\n";
    os << "|" << data._21 << ", " << data._22 << ", " << data._23 << ", " << data._24 << "|\n";
    os << "|" << data._31 << ", " << data._32 << ", " << data._33 << ", " << data._34 << "|\n";
    os << "|" << data._41 << ", " << data._42 << ", " << data._43 << ", " << data._44 << "|\n";
    return os;
}

// 서버 관련

void ErrorQuit(const char* msg);
void ErrorDisplay(const char* msg);