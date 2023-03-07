#include "text.h"

Text::Text() : m_width{ 1280.f }, m_height{ 720.f }, 
m_layoutRect{ 0.f, 360.f, m_width , m_height }, m_enable(true)
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


LoadingText::LoadingText(UINT maxFileNum) : m_maxFileNum{maxFileNum}, m_LoadedFileNum{0}
{
}

void LoadingText::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (m_enable) {
		device->SetTransform(D2D1::Matrix3x2F::Identity());
		device->DrawText(m_text.c_str(), m_text.size(), m_textFormat.Get(), &m_layoutRect, m_textBrush.Get());
	}
}

void LoadingText::Update(FLOAT timeElapsed)
{
	m_text = m_fileName + L" ·Îµù Áß.. " + to_wstring(m_LoadedFileNum) + L" / " + to_wstring(m_maxFileNum);
}
