#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include "protocol.h"

// d3d12 헤더 파일입니다.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <array>
#include <vector>
#include <mutex>
#include "protocol.h"


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace std;
using namespace DirectX;

#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

#define SERVERPORT 9000
#define MAX_PLAYERS 3


