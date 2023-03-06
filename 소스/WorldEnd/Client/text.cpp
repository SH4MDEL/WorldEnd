#include "text.h"

Text::Text() : m_width{ 1280 }, m_height{ 720 }, 
m_layoutRect{ 0.f, 0.f, m_width, m_height }, m_enable(true)
{

}

void Text::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (m_enable) {
		device->DrawText(m_text.c_str(), m_text.size(), m_textFormat.get(), &m_layoutRect, m_textBrush.get());
	}
}

void Text::Update(FLOAT timeElapsed) {}


LoadingText::LoadingText(UINT maxFileNum) : m_maxFileNum{maxFileNum}
{
}

void LoadingText::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (m_enable) {
		device->DrawText(m_text.c_str(), m_text.size(), m_textFormat.get(), &m_layoutRect, m_textBrush.get());
	}
}

void LoadingText::Update(FLOAT timeElapsed)
{
}
