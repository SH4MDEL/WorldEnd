#include "ui.h"
#include "framework.h"

UI::UI(XMFLOAT2 position, XMFLOAT2 size) : 
	m_position{ position }, m_size{ size }, m_enable{ true }, m_clickEvent{ []() {} }
{

}

void UI::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
	if (!m_enable) return;

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	}
}

void UI::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	// 사용할 수 없다면 자식 UI까지 갈 것도 없이 return한다.
	if (!m_enable) return;

	// 충돌했다면 설정된 버튼의 함수를 stack에 넣는다.
	if (message == WM_LBUTTONDOWN) {
		if (m_uiMatrix._11 - m_uiMatrix._21 + 1.f <= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
			m_uiMatrix._11 + m_uiMatrix._21 + 1.f >= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
			1.f - m_uiMatrix._12 - m_uiMatrix._22 <= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f &&
			1.f - m_uiMatrix._12 + m_uiMatrix._22 >= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f) {
			g_clickEventStack.push(m_clickEvent);
		}
	}

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->OnProcessingMouseMessage(message, lParam);
	}
}

void UI::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!m_enable) return;

	for (auto& child : m_children) {
		child->OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
	}
}

void UI::Update(FLOAT timeElapsed) 
{
	if (!m_enable) return;

	for (auto& child : m_children) {
		child->Update(timeElapsed);
	}
}

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

	if (m_texture) {
		m_texture->UpdateShaderVariable(commandList);

		commandList->IASetVertexBuffers(0, 1, nullptr);
		commandList->IASetIndexBuffer(nullptr);
		// Geometry Shader를 거친다.
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		commandList->DrawInstanced(1, 1, 0, 0);
	}

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

void UI::SetTexture(const string& name) 
{ 
	if (Scene::m_textures[name]) m_texture = Scene::m_textures[name];
}
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

TextUI::TextUI(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT2 textSize) : UI(position, size)
{
	m_type = Type::TEXT;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();

	m_text = make_shared<Text>(position, textSize.x, textSize.y);
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

void TextUI::SetText(const wstring& text) { if (m_text) m_text->SetText(text); }
void TextUI::SetColorBrush(const string& colorBrush) { if (m_text) m_text->SetColorBrush(colorBrush); }
void TextUI::SetTextFormat(const string& textFormat) { if (m_text) m_text->SetTextFormat(textFormat); }


ButtonUI::ButtonUI(XMFLOAT2 position, XMFLOAT2 size) : UI(position, size)
{
	m_type = Type::BUTTON_NOACTIVE;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();
}

void ButtonUI::OnProcessingMouseMessage(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
	// 사용할 수 없다면 자식 UI까지 갈 것도 없이 return한다.
	if (!m_enable) return;

	if (m_uiMatrix._11 - m_uiMatrix._21 + 1.f <= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
		m_uiMatrix._11 + m_uiMatrix._21 + 1.f >= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
		1.f - m_uiMatrix._12 - m_uiMatrix._22 <= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f &&
		1.f - m_uiMatrix._12 + m_uiMatrix._22 >= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f) {
		if (m_type != Type::BUTTON_ACTIVE) {
			m_type = Type::BUTTON_MOUSEON;
		}
	}
	else {
		m_type = Type::BUTTON_NOACTIVE;
	}

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->OnProcessingMouseMessage(hWnd, width, height, deltaTime);
	}
}

void ButtonUI::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	// 사용할 수 없다면 자식 UI까지 갈 것도 없이 return한다.
	if (!m_enable) return;

	// 충돌했다면 설정된 버튼의 함수를 stack에 넣는다.
	if (message == WM_LBUTTONDOWN) {
		if (m_uiMatrix._11 - m_uiMatrix._21 + 1.f <= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
			m_uiMatrix._11 + m_uiMatrix._21 + 1.f >= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
			1.f - m_uiMatrix._12 - m_uiMatrix._22 <= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f &&
			1.f - m_uiMatrix._12 + m_uiMatrix._22 >= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f) {
			g_clickEventStack.push(m_clickEvent);
			m_type = Type::BUTTON_ACTIVE;
		}
	}
	if (message == WM_LBUTTONUP) {
		m_type = Type::BUTTON_NOACTIVE;
	}

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->OnProcessingMouseMessage(message, lParam);
	}
}

HorzGaugeUI::HorzGaugeUI(XMFLOAT2 position, XMFLOAT2 size, FLOAT border) : 
	UI(position, size), m_border{ border }
{
	m_type = Type::HORZGAUGE;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();
}

void HorzGaugeUI::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent)
{
	if (!m_enable) return;

	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_gauge), 16);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_maxGauge), 17);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_border), 18);

	UI::Render(commandList, parent);
}

VertGaugeUI::VertGaugeUI(XMFLOAT2 position, XMFLOAT2 size, FLOAT border) : 
	UI(position, size), m_border{ border }
{
	m_type = Type::VERTGAUGE;
	XMStoreFloat4x4(&m_uiMatrix, XMMatrixIdentity());
	m_size.x /= g_GameFramework.GetAspectRatio();
}

void VertGaugeUI::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent)
{
	if (!m_enable) return;

	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_gauge), 16);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_maxGauge), 17);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &(m_border), 18);

	UI::Render(commandList, parent);
}

InputTextUI::InputTextUI(XMFLOAT2 position, XMFLOAT2 size, XMFLOAT2 textSize, INT limit) :
	TextUI(position, size, textSize), m_limit{ limit }, m_mouseOn{ false }, m_caretTime { 0.f }, m_caret{ false }
{
	m_type = Type::TEXTBUTTON_NOACTIVE;
}

void InputTextUI::Update(FLOAT timeElapsed)
{
	if (!m_enable) return;

	// 텍스트 바가 활성화 된 상태일경우 캐럿에 관한 업데이트 진행
	if (m_type == Type::TEXTBUTTON_ACTIVE) {
		m_caretTime += timeElapsed;
		if (m_caretTime >= m_caretLifetime) {
			m_caretTime -= m_caretLifetime;
			m_caret = !m_caret;
		}
	}
	if (m_caret) {
		m_text->SetText(m_texting.str() + TEXT("|"));

	}
	else {
		m_text->SetText(m_texting.str());
	}

	for (auto& child : m_children) child->Update(timeElapsed);
}

void InputTextUI::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<UI>& parent)
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

	XMFLOAT4X4 uiMatrix;
	XMStoreFloat4x4(&uiMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_uiMatrix)));
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 16, &uiMatrix, 0);
	commandList->SetGraphicsRoot32BitConstants((INT)ShaderRegister::GameObject, 1, &m_type, 19);

	if (m_texture) {
		m_texture->UpdateShaderVariable(commandList);

		commandList->IASetVertexBuffers(0, 1, nullptr);
		commandList->IASetIndexBuffer(nullptr);
		// Geometry Shader를 거친다.
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		commandList->DrawInstanced(1, 1, 0, 0);
	}

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->Render(commandList, shared_from_this());
	}
}

void InputTextUI::OnProcessingMouseMessage(UINT message, LPARAM lParam)
{
	// 사용할 수 없다면 자식 UI까지 갈 것도 없이 return한다.
	if (!m_enable) return;

	// 충돌했다면 설정된 버튼의 함수를 stack에 넣는다.
	if (message == WM_LBUTTONDOWN) {
		if (m_uiMatrix._11 - m_uiMatrix._21 + 1.f <= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
			m_uiMatrix._11 + m_uiMatrix._21 + 1.f >= (FLOAT)g_mousePosition.x / (FLOAT)g_GameFramework.GetWindowWidth() * 2.f &&
			1.f - m_uiMatrix._12 - m_uiMatrix._22 <= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f &&
			1.f - m_uiMatrix._12 + m_uiMatrix._22 >= (FLOAT)g_mousePosition.y / (FLOAT)g_GameFramework.GetWindowHeight() * 2.f) {
			
			m_type = Type::TEXTBUTTON_ACTIVE;
		}
		else {
			m_type = Type::TEXTBUTTON_NOACTIVE;
			m_caret = false;
			m_caretTime = 0.f;
		}
	}

	for (auto& child : m_children) {
		// 자식 UI에도 전파한다.
		child->OnProcessingMouseMessage(message, lParam);
	}
}

void InputTextUI::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CHAR:
		if (m_type == Type::TEXTBUTTON_ACTIVE) {

			if (wParam < 128) {
				if (m_texting.str().length() < m_limit) {
					SetInputLogic(wParam);
				}
				else if (m_texting.str().length() >= m_limit && wParam == VK_BACK) {
					DeleteLastChar();
				}
			}
		}
		break;
	}

	for (auto& child : m_children) {
		child->OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
	}
}

void InputTextUI::SetTextLimit(INT limit)
{
	m_limit = limit;
}

wstring InputTextUI::GetString()
{
	return m_texting.str();
}

void InputTextUI::SetInputLogic(WPARAM wParam)
{
	if (wParam == VK_BACK) {
		if (m_texting.str().length() > 0) {
			DeleteLastChar();
		}
	}
	else if (wParam == VK_RETURN) {
		// 입력 처리
		m_type = Type::TEXTBUTTON_NOACTIVE;
		m_caret = false;
		m_caretTime = 0.f;
	}
	else if (wParam == VK_ESCAPE) {
		// 탈출
		m_type = Type::TEXTBUTTON_NOACTIVE;
		m_caret = false;
		m_caretTime = 0.f;
	}
	else {
		m_texting << static_cast<char>(wParam);
	}
	m_text->SetText(m_texting.str());
}

void InputTextUI::DeleteLastChar()
{
	wstring oldText = m_texting.str();
	wstring newText = TEXT("");
	for (int i = 0; i < oldText.length() - 1; ++i) {
		newText += oldText[i];
	}
	m_texting.str(TEXT(""));
	m_texting << newText;

	m_text->SetText(m_texting.str());
}
