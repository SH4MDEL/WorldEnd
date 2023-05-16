#include "text.h"
#include "framework.h"

unordered_map<string, ComPtr<ID2D1SolidColorBrush>>		Text::m_colorBrushes;
unordered_map<string, ComPtr<IDWriteTextFormat>>		Text::m_textFormats;

Text::Text() : m_position{ 0.f, 0.f }, m_width{ (FLOAT)g_GameFramework.GetWindowWidth() }, m_height{ (FLOAT)g_GameFramework.GetWindowHeight() },
m_layoutRect{ 0.f, (FLOAT)g_GameFramework.GetWindowHeight() / 2.f, m_width , m_height }, m_enable(true)
{
}

Text::Text(XMFLOAT2 position, FLOAT width, FLOAT height) :
	m_position{ position }, m_width{ width }, m_height{ height },
	m_layoutRect{ m_position.x - m_width, m_position.y - m_height, m_position.x + m_width , m_position.y + m_height }, m_enable(true)
{
}

void Text::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (m_enable) {
		device->SetTransform(D2D1::Matrix3x2F::Identity());
		device->DrawText(m_text.c_str(), m_text.size(), m_textFormats[m_textFormat].Get(), &m_layoutRect, m_colorBrushes[m_colorBrush].Get());
	}
}

void Text::Update(FLOAT timeElapsed) {}

void Text::SetPosition(XMFLOAT2 position)
{
	// ���� ��ǥ�迡�� ȭ��(��ġ) ��ǥ��� ��ȯ�ؾ� �Ѵ�.
	// ���� ��ǥ��� �Ʒ����� ���� �� ���� Ŀ������
	// ȭ�� ��ǥ��� ������ �Ʒ��� �� ���� Ŀ����.
	m_position.x = (position.x + 1) * g_GameFramework.GetWindowWidth() / 2;
	m_position.y = (1 - position.y) * g_GameFramework.GetWindowHeight() / 2;
	UpdateTransform();
}

void Text::UpdateTransform()
{
	m_layoutRect = { m_position.x - m_width, m_position.y - m_height, m_position.x + m_width , m_position.y + m_height };
}