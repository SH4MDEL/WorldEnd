#pragma once
#include "stdafx.h"

// UI
// 
// 두 가지를 설정한다.
// 1. 위치
// 2. 크기
// 
// 크기는 Mesh에서 일괄적으로 설정?
// Mesh를 하나만 만들고 공유하려면 UI에서 설정?
// 
// 위치는 상대 위치이다.
// 위치를 설정하면,
// 부모 UI의 상에서의 위치로 출력되어야 한다. 
// (단, 루트 UI는 화면상의 위치가 출력된다.)
// 
// 예를 들어 확인 / 취소 / 텍스트가 있는 UI가 있다고 치자
// 루트 UI의 크기를 (0.3, 0.6), 위치를 (0.0, 0.0) 이라고 한다면,
// 이 안내 UI는 화면의 중앙에 생성되고, 
// 가로 크기 30%, 세로 크기 60%를 차지하게 될 것이다.
// 
// 취소 UI의 크기를 (0.1, 0.1), 위치를 (0.5, 0.8) 로 한다면,
// 취소 UI의 크기는 루트 UI의 크기의 10%,
// 루트 UI에서 우측 하단에 생성된다.
// 
// 이러한 정보를 어떻게 셰이더로 보낼지 생각해보자.
// 중요한건 셰이더에 보내는 위치와 크기
// UI가 직접 가지고 있는 위치와 크기가 다르다는 것이다.
//

class UI
{
};

