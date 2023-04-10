#pragma once
#include "stdafx.h"
#include "texture.h"
#include "text.h"

class UI : public enable_shared_from_this<UI>
{
public:
	enum class Type {
		STANDARD,
		BACKGROUND,
		TEXT,
		BUTTON,
		HORZGAUGE,
		VERTGAUGE
	};
	UI(XMFLOAT2 position, XMFLOAT2 size);
	~UI() = default;

	virtual void OnProcessingMouseMessage(UINT message, LPARAM lParam);

	virtual void Update(FLOAT timeElapsed);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent);
	virtual void RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext);

	void SetEnable() { m_enable = true; }
	void SetDisable() { m_enable = false; }

	void SetTexture(const shared_ptr<Texture>& texture);
	void SetText(const wstring& text);
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
	shared_ptr<Text> m_text;

	vector<shared_ptr<UI>> m_children;

	function<void()> m_clickEvent;
};

class StandardUI : public UI
{
public:
	StandardUI(XMFLOAT2 position, XMFLOAT2 size);
	~StandardUI() = default;

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
	TextUI(XMFLOAT2 position, XMFLOAT2 size);
	~TextUI() = default;

	void OnProcessingMouseMessage(UINT message, LPARAM lParam) override;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent) override;
	void RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext) override;
private:
};

class ButtonUI : public UI
{
public:
	ButtonUI(XMFLOAT2 position, XMFLOAT2 size);
	~ButtonUI() = default;

private:
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

// UI
// 
// �� ������ �����Ѵ�.
// 1. ��ġ
// 2. ũ��
// 
// ũ��� Mesh���� �ϰ������� ����?
// Mesh�� �ϳ��� ����� �����Ϸ��� UI���� ����?
// 
// ��ġ�� ��� ��ġ�̴�.
// ��ġ�� �����ϸ�,
// �θ� UI�� �󿡼��� ��ġ�� ��µǾ�� �Ѵ�. 
// (��, ��Ʈ UI�� ȭ����� ��ġ�� ��µȴ�.)
// 
// ���� ��� Ȯ�� / ��� / �ؽ�Ʈ�� �ִ� UI�� �ִٰ� ġ��
// ��Ʈ UI�� ũ�⸦ (0.3, 0.6), ��ġ�� (0.0, 0.0) �̶�� �Ѵٸ�,
// �� �ȳ� UI�� ȭ���� �߾ӿ� �����ǰ�, 
// ���� ũ�� 30%, ���� ũ�� 60%�� �����ϰ� �� ���̴�.
// 
// ��� UI�� ũ�⸦ (0.1, 0.1), ��ġ�� (0.5, 0.8) �� �Ѵٸ�,
// ��� UI�� ũ��� ��Ʈ UI�� ũ���� 10%,
// ��Ʈ UI���� ���� �ϴܿ� �����ȴ�.
// 
// �̷��� ������ ��� ���̴��� ������ �����غ���.
// �߿��Ѱ� ���̴��� ������ ��ġ�� ũ��
// UI�� ���� ������ �ִ� ��ġ�� ũ�Ⱑ �ٸ��ٴ� ���̴�.
// 
// 
// Render �� �ڽ����� '���̴��� ����' ��ġ�� ũ�⸦ ������.
// (��Ʈ UI�� ������ ���� �翬�� nullptr�� ������.)
// ���̴��� ���� ��ġ�� ũ��, ���� ���� ��ġ�� ũ�⸦ �������
// ���� ���̴��� ���� ��ġ�� ũ�⸦ �����Ѵ�.
// 
// ��Ʈ UI
// ũ�� (0.3, 0.6), ��ġ (0.0, 0.0)
// ��� ���̴��� �� ������ �ȴ�.
// 
// ��� UI 
// ũ�� (0.1, 0.1), ��ġ (0.5, 0.8)
// �θ� UI�� ũ��� ��ġ ���� ������ �ִ�.
// �θ� UI�� ���� ��ǥ, �� ��ǥ�� ���Ѵ�.
// �뷫 (-0.3, -0.6) ~ (0.3, 0.6)�� �� ���̴�.
// �̸� ������� �� ��ǥ�� ���Ѵ�.
// ũ��� (0.03, 0.06) ��ġ�� (0.15, 0.36)
// 
// ������ ������ �ִ�.
// ũ�Ⱑ �θ� ������Ʈ�� �����Ͽ� ��������.
// �׷��ٸ� ũ��� ��Ʈ UI�� ũ�⸦ ������ ����
// ���� ũ�⸦ ���صε��� ����.
// ��ġ�� ������ ���� ���δ�. �̴�� ����.
//