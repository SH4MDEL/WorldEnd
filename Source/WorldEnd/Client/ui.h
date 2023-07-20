#pragma once
#include "stdafx.h"
#include "texture.h"
#include "text.h"

class UI : public enable_shared_from_this<UI>
{
public:
	enum class Type {
		IMAGE,
		BACKGROUND,
		TEXT,
		BUTTON_NOACTIVE,
		BUTTON_MOUSEON,
		BUTTON_ACTIVE,
		SWITCH_NOACTIVE,
		SWITCH_ACTIVE,
		HORZGAUGE,
		VERTGAUGE,
		TEXTBUTTON_NOACTIVE,
		TEXTBUTTON_ACTIVE
	};
	UI(XMFLOAT2 position, XMFLOAT2 size);
	~UI() = default;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime);
	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent);
	virtual void RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext);

	void SetEnable() { m_enable = true; }
	void SetDisable() { m_enable = false; }

	void SetTexture(const string& name);
	void SetChild(const shared_ptr<UI>& ui);
	void SetClickEvent(function<void()> chickEvent);

	XMFLOAT4X4 GetUIMatrix();

protected:
	BOOL m_enable;
	Type m_type;

	XMFLOAT4X4 m_uiMatrix;

	XMFLOAT2 m_position;
	XMFLOAT2 m_size;

	shared_ptr<Texture> m_texture;

	vector<shared_ptr<UI>> m_children;

	function<void()> m_clickEvent;
};

class ImageUI : public UI
{
public:
	ImageUI(XMFLOAT2 position, XMFLOAT2 size);
	~ImageUI() = default;

private:
};

class BackgroundUI : public UI
{
public:
	BackgroundUI(XMFLOAT2 position, XMFLOAT2 size);
	~BackgroundUI() = default;

private:
};

class TextUI : public UI
{
public:
	TextUI(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT2 textSize);
	~TextUI() = default;

	void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent) override;
	void RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) override;

	void SetText(const wstring& text);
	void SetColorBrush(const string& colorBrush);
	void SetTextFormat(const string& textFormat);

protected:
	shared_ptr<Text> m_text;
};

class ButtonUI : public UI
{
public:
	ButtonUI(XMFLOAT2 position, XMFLOAT2 size);
	~ButtonUI() = default;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) override;
	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;
};

class SwitchUI : public UI
{
public:
	SwitchUI(XMFLOAT2 position, XMFLOAT2 size);
	~SwitchUI() = default;

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime) override;
	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;

	bool IsActive();
	void SetNoActive();
};

class HorzGaugeUI : public UI
{
public:
	HorzGaugeUI(XMFLOAT2 position, XMFLOAT2 size, FLOAT border);
	~HorzGaugeUI() = default;
	
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent) override;

	void SetGauge(FLOAT gauge) { m_gauge = gauge; }
	void SetMaxGauge(FLOAT maxGauge) { m_maxGauge = maxGauge; }
private:
	FLOAT m_gauge;
	FLOAT m_maxGauge;

	FLOAT m_border;
};

class VertGaugeUI : public UI
{
public:
	VertGaugeUI(XMFLOAT2 position, XMFLOAT2 size, FLOAT border);
	~VertGaugeUI() = default;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent) override;

	void SetGauge(FLOAT gauge) { m_gauge = gauge; }
	void SetMaxGauge(FLOAT maxGauge) { m_maxGauge = maxGauge; }
private:
	FLOAT m_gauge;
	FLOAT m_maxGauge;

	FLOAT m_border;
};

class InputTextUI : public TextUI
{
public:
	InputTextUI(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT2 textSize, INT limit);
	virtual ~InputTextUI() = default;

	virtual void Update(FLOAT timeElapsed) override;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent) override;

	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

	void SetTextLimit(INT limit);

	wstring GetString();

private:
	void SetInputLogic(WPARAM wParam);
	void DeleteLastChar();

protected:
	wstringstream	m_texting;
	INT				m_limit;
	BOOL			m_mouseOn;

	const FLOAT		m_caretLifetime = 0.5f;
	FLOAT			m_caretTime;
	BOOL			m_caret;
};

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
// 
// Render 시 자식으로 '셰이더로 보낼' 위치와 크기를 보낸다.
// (루트 UI를 렌더할 때는 당연히 nullptr을 보낸다.)
// 셰이더로 보낼 위치와 크기, 내가 가진 위치와 크기를 기반으로
// 내가 셰이더로 보낼 위치와 크기를 결정한다.
// 
// 루트 UI
// 크기 (0.3, 0.6), 위치 (0.0, 0.0)
// 얘는 셰이더로 걍 보내면 된다.
// 
// 취소 UI 
// 크기 (0.1, 0.1), 위치 (0.5, 0.8)
// 부모 UI의 크기와 위치 역시 가지고 있다.
// 부모 UI의 시작 좌표, 끝 좌표를 구한다.
// 대략 (-0.3, -0.6) ~ (0.3, 0.6)이 될 것이다.
// 이를 기반으로 내 좌표를 구한다.
// 크기는 (0.03, 0.06) 위치는 (0.15, 0.36)
// 
// 딱봐도 문제가 있다.
// 크기가 부모 오브젝트의 스케일에 맞춰진다.
// 그렇다면 크기는 루트 UI의 크기를 따르지 말고
// 절대 크기를 정해두도록 하자.
// 위치는 문제가 없어 보인다. 이대로 가자.
//