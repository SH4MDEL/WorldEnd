#include "ui.h"
#include "framework.h"

UI::UI(XMFLOAT2 position, XMFLOAT2 size) : 
	m_position{ position }, m_size{ size }, m_enable{ true }, m_clickEvent{ []() {} }
{

}

void UI::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	// 사용할 수 없다면 자식 UI까지 갈 것도 없이 return한다.
	if (!m_enable) return;

	// 충돌했다면 설정된 버튼의 함수를 stack에 넣는다.
	if (m_uiMatrix._11 - m_uiMatrix._21 + 1 <= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2 &&
		m_uiMatrix._11 + m_uiMatrix._21 + 1 >= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2 &&
		1 - m_uiMatrix._12 - m_uiMatrix._22 <= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2 &&
		1 - m_uiMatrix._12 + m_uiMatrix._22 >= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2) {
		g_clickEventStack.push(m_clickEvent);
	}

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->OnProcessingMouseMessage(message, lParam);
	}
}

void UI::Update(FLOAT timeElapsed) {}

void UI::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent)
{
	if (!m_enable) return;

	// UI만의 특수한 Matrix를 사용한다.
	// 반드시 UIShader가 세팅되어 있어야 한다.
	if (parent) {
		// 부모 UI가 존재한다면 부모 UI의 크기에 맞춰
		// 위치를 결정한다.
		XMFLOAT4X4 parentMatrix = parent->GetUIMatrix();
		m_uiMatrix._11 = parentMatrix._21 * m_position.x + parentMatrix._11;
		m_uiMatrix._12 = parentMatrix._22 * m_position.y + parentMatrix._12;

	}
	else {
		// 존재하지 않는다면 루트 UI라는 의미이다.
		m_uiMatrix._11 = m_position.x;
		m_uiMatrix._12 = m_position.y;
	}
	m_uiMatrix._21 = m_size.x;
	m_uiMatrix._22 = m_size.y;

	XMFLOAT4X4 uiMatrix;
	XMStoreFloat4x4(&uiMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_uiMatrix)));
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 16, &uiMatrix, 0);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &m_type, 19);

	if (m_texture) m_texture->UpdateShaderVariable(commandList);

	commandList->IASetVertexBuffers(0, 1, nullptr);
	commandList->IASetIndexBuffer(nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	commandList->DrawInstanced(1, 1, 0, 0);

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->Render(commandList, shared_from_this());
	}
}

void UI::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	if (!m_enable) return;

	for (auto& child : m_children) {
		child->RenderText(deviceContext);
	}
}

void UI::SetTexture(const shared_ptr<Texture>& texture) { m_texture = texture; }
void UI::SetText(const wstring& text) { if (m_text) m_text->SetText(text); }
void UI::SetChild(const shared_ptr<UI>& ui) { m_children.push_back(ui); }
void UI::SetClickEvent(function<void()> chickEvent) { m_clickEvent = chickEvent; }
XMFLOAT4X4 UI::GetUIMatrix() { return m_uiMatrix; }

StandardUI::StandardUI(XMFLOAT2 position, XMFLOAT2 size) : UI(position, size)
{
	m_type = Type::STANDARD;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();
}

BackgroundUI::BackgroundUI(XMFLOAT2 position, XMFLOAT2 size) : UI(position, size)
{
	m_type = Type::BACKGROUND;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
}

TextUI::TextUI(XMFLOAT2 position, XMFLOAT2 size) : UI(position, size)
{
	m_type = Type::TEXT;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();

	m_text = make_shared<Text>(position, size.x, size.y);

	auto textBrush = ComPtr<ID2D1SolidColorBrush>();
	DX::ThrowIfFailed(g_GameFramework.GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.f), &textBrush));
	m_text->SetTextBrush(textBrush);

	auto textFormat = ComPtr<IDWriteTextFormat>();
	DX::ThrowIfFailed(g_GameFramework.GetWriteFactory()->CreateTextFormat(
		TEXT("돋움체"), nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		16.f,
		TEXT("ko-kr"),
		&textFormat
	));

	textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	m_text->SetTextFormat(textFormat);
}

void TextUI::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	// Text는 충돌 검사하지 않는다.
	if (!m_enable) return;

	for (auto& child : m_children) {
		child->OnProcessingMouseMessage(message, lParam);
	}
}

void TextUI::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent)
{
	if (!m_enable) return;

	if (parent) {
		XMFLOAT4X4 parentMatrix = parent->GetUIMatrix();
		m_uiMatrix._11 = parentMatrix._21 * m_position.x + parentMatrix._11;
		m_uiMatrix._12 = parentMatrix._22 * m_position.y + parentMatrix._12;

	}
	else {
		m_uiMatrix._11 = m_position.x;
		m_uiMatrix._12 = m_position.y;
	}
	m_uiMatrix._21 = m_size.x;
	m_uiMatrix._22 = m_size.y;

	if (m_text) m_text->SetPosition(XMFLOAT2{ m_uiMatrix._11, m_uiMatrix._12 });

	for (auto& child : m_children) {
		child->Render(commandList, shared_from_this());
	}
}

void TextUI::RenderText(const ComPtr<ID2D1DeviceContext2>& deviceContext)
{
	if (!m_enable) return;

	if (m_text) m_text->Render(deviceContext);

	for (auto& child : m_children) {
		child->RenderText(deviceContext);
	}
}

ButtonUI::ButtonUI(XMFLOAT2 position, XMFLOAT2 size) : UI(position, size)
{
	m_type = Type::BUTTON;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();
}
