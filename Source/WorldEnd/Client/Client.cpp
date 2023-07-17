#include "stdafx.h"
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

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

#ifdef _DEBUG
void CheckMemoryLeak()
{
    HMODULE dxgidebugdll = GetModuleHandleW(L"dxgidebug.dll");
    decltype(&DXGIGetDebugInterface) GetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(dxgidebugdll, "DXGIGetDebugInterface"));

    IDXGIDebug* debug;
    GetDebugInterface(IID_PPV_ARGS(&debug));

    OutputDebugString(L"▼▼▼▼▼▼▼▼▼▼▼▼▼▼ 메모리 누수 체크 ▼▼▼▼▼▼▼▼▼▼▼▼▼▼\n");
    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_SUMMARY);
    OutputDebugString(L"▲▲▲▲▲ 반환되지 않은 IUnknown 객체가 있을 경우 표시합니다. ▲▲▲▲▲\n");

    debug->Release();
}
#endif // !_DEBUG

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
   
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.
    ifstream in{ "serverip.txt" };
    if (in) { while (in >> g_serverIP) {} }

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

#ifdef _DEBUG
    CheckMemoryLeak();
#endif // !_DEBUG
    return (int)msg.wParam;
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

    RECT rect{ 0, 0, (LONG)g_GameFramework.GetWindowWidth(), (LONG)g_GameFramework.GetWindowHeight() };
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
    RECT rect; GetWindowRect(hWnd, &rect);
    GetCursorPos(&g_mousePosition);
    g_mousePosition.x -= rect.left;
    g_mousePosition.y -= rect.top;

    switch (message)
    {
    case WM_ACTIVATE:
        g_GameFramework.SetIsActive((BOOL)wParam);
        break;
    case WM_SIZE:
        g_GameFramework.OnResize(hWnd);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        g_GameFramework.OnProcessingMouseMessage(message, lParam);
        break;
    case WM_CHAR:
        g_GameFramework.OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}