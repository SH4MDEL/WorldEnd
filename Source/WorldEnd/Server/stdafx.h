// d3d12 헤더 파일입니다.
#pragma once
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#define NOMINMAX

#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <array>
#include <span>
#include <list>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <random>
#include <queue>
#include <concurrent_priority_queue.h>
#include <concurrent_unordered_set.h>
#include "protocol.h"

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <DirectXCollision.h>

using namespace DirectX;

#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

extern std::mt19937     g_random_engine;

void ErrorDisplay(const char* msg);
void ErrorDisplay(int err_no);

enum AggroLevel : BYTE { NORMAL_AGGRO, HIT_AGGRO, MAX_AGGRO };

namespace Vector3
{
    inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMFLOAT3 result{};
        XMStoreFloat3(&result, XMVectorAdd(XMLoadFloat3(&a), XMLoadFloat3(&b)));
        return result;
    }
    inline XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMFLOAT3 result{};
        XMStoreFloat3(&result, XMVectorSubtract(XMLoadFloat3(&a), XMLoadFloat3(&b)));
        return result;
    }
    inline XMFLOAT3 Mul(const XMFLOAT3& a, const FLOAT& scalar)
    {
        XMFLOAT3 result{};
        XMStoreFloat3(&result, XMVectorScale(XMLoadFloat3(&a), scalar));
        return result;
    }
    inline FLOAT Dot(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMFLOAT3 result{};
        XMStoreFloat3(&result, XMVector3Dot(XMLoadFloat3(&a), XMLoadFloat3(&b)));
        return result.x;
    }
    inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMFLOAT3 result;
        XMStoreFloat3(&result, XMVector3Cross(XMLoadFloat3(&a), XMLoadFloat3(&b)));
        return result;
    }
    inline XMFLOAT3 Angle(const XMFLOAT3& a, const XMFLOAT3& b, bool isNormalized = true)
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
    inline FLOAT Distance(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return Vector3::Length(Vector3::Sub(a, b));
    }
    inline FLOAT Equal(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMVector3Equal(XMLoadFloat3(&a), XMLoadFloat3(&b));
    }
}

