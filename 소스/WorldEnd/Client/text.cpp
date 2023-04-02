#include "text.h"
#include "framework.h"

Text::Text() : m_position{0.f, 0.f}, m_width { 1280.f }, m_height{ 720.f },
m_layoutRect{ 0.f, 360.f, m_width , m_height }, m_enable(true)
{
}

Text::Text(XMFLOAT2 position, FLOAT width, FLOAT height) :
	m_position{ position }, m_width { width }, m_height{ height },
	m_layoutRect{ m_position.x - m_width, m_position.y - m_height, m_position.x + m_width , m_position.y + m_height }, m_enable(true)
{
}

void Text::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (m_enable) {
		device->SetTransform(D2D1::Matrix3x2F::Identity());
		device->DrawText(m_text.c_str(), m_text.size(), m_textFormat.Get(), &m_layoutRect, m_textBrush.Get());
	}
}

void Text::Update(FLOAT timeElapsed) {}

void Text::SetPosition(XMFLOAT2 position)
{
	// 정규 좌표계에서 화면(장치) 좌표계로 변환해야 한다.
	// 정규 좌표계는 아래에서 위로 갈 수록 커지지만
	// 화면 좌표계는 위에서 아래로 갈 수록 커진다.
	m_position.x = (position.x + 1) * g_GameFramework.GetWindowWidth() / 2;
	m_position.y = (1 - position.y) * g_GameFramework.GetWindowHeight() / 2;
	UpdateTransform();
}

void Text::UpdateTransform()
{
	m_layoutRect = { m_position.x - m_width, m_position.y - m_height, m_position.x + m_width , m_position.y + m_height };
}


LoadingText::LoadingText(UINT maxFileNum) : m_maxFileNum{maxFileNum}, m_LoadedFileNum{0}
{
}

void LoadingText::Update(FLOAT timeElapsed)
{
	m_text = m_fileName + L" 로딩 중.. " + to_wstring(m_LoadedFileNum) + L" / " + to_wstring(m_maxFileNum);
}
