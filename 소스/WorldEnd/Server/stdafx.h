// d3d12 헤더 파일입니다.
#pragma once
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <array>
#include <vector>
#include <mutex>
#include <random>
#include <concurrent_queue.h>
#include<concurrent_priority_queue.h>
#include<concurrent_unordered_set.h>
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

using namespace std;
using namespace DirectX;

#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

class Server;
extern Server           g_server;
extern SOCKET			g_socket;
extern HANDLE           g_h_iocp;
extern std::mt19937		g_random_engine;

void ErrorDisplay(const char* msg);
void ErrorDisplay(int err_no);

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
}

