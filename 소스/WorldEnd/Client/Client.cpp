﻿#include "stdafx.h"
#include "client.h"
#include "framework.h"
#include "scene.h"
#include "object.h"

// 다음을 정의하면 프로그램 실행시 콘솔이 출력됩니다.
#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE           hInst;                          // 현재 인스턴스입니다.
WCHAR               szTitle[MAX_LOADSTRING];        // 제목 표시줄 텍스트입니다.
WCHAR               szWindowClass[MAX_LOADSTRING];  // 기본 창 클래스 이름입니다.
GameFramework       g_GameFramework(1280, 720);     // 게임프레임워크

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


void DoSend();
void DoRecv();

//Connect net;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

#ifdef USE_NETWORK
    //Init();
    //ConnectTo();

    //CS_LOGIN_PACKET login_packet;
    //login_packet.size = sizeof(CS_LOGIN_PACKET);
    //login_packet.type = CS_LOGIN;
    //strcpy_s(login_packet.name, "SU");
    ////net.SendPacket(&login_packet);
    //send(m_c_socket, reinterpret_cast<char*>(&login_packet),sizeof(login_packet),0);

#endif // USE_NETWORK
   
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return false;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

    MSG msg;
  
   
    // 기본 메시지 루프입니다:
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            g_GameFramework.FrameAdvance();
        }

    }

    g_GameFramework.OnDestroy();
    return (int)msg.wParam;
}

void DoSend() {

   //// Scene::Get_Instatnce()->GetPlayerInfo()->fx = GameObject::GetPosition()

    //GameObject* obj = new GameObject;
    //XMFLOAT3 pos = obj->GetPosition();

    //PLAYERINFO packet;
    //packet.id = g_clientId;
    //WSAOVERLAPPED* c_over = new WSAOVERLAPPED;


  
}

void DoRecv() {

}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    RECT rect{ 0, 0, g_GameFramework.GetWindowWidth(), g_GameFramework.GetWindowHeight() };
    DWORD dwStyle{ WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER };
    AdjustWindowRect(&rect, dwStyle, false);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);

    g_GameFramework.OnCreate(hInst, hWnd);

    if (!hWnd)
    {
        return false;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ACTIVATE:
        g_GameFramework.SetIsActive((BOOL)wParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}