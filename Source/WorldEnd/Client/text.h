#pragma once
#include "stdafx.h"

class Text
{
public:
	Text();
	Text(XMFLOAT2 position, FLOAT width, FLOAT height);
	~Text() = default;

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT timeElapsed);

	void SetText(const wstring& text) { m_text = text; }
	void SetTextBrush(const ComPtr<ID2D1SolidColorBrush>& textBrush) { m_textBrush = textBrush; }
	void SetTextFormat(const ComPtr<IDWriteTextFormat>& textFormat) { m_textFormat = textFormat; }

	void SetPosition(XMFLOAT2 position);
	void UpdateTransform();

protected:
	wstring								m_text;
	BOOL								m_enable;

	FLOAT								m_width;
	FLOAT								m_height;
	XMFLOAT2							m_position;
	D2D1_RECT_F							m_layoutRect;

	ComPtr<ID2D1SolidColorBrush>	m_textBrush;
	ComPtr<IDWriteTextFormat>		m_textFormat;
};

class LoadingText : public Text
{
public:
	LoadingText(UINT maxFileNum);
	~LoadingText() = default;

	void Update(FLOAT timeElapsed) override;

	void SetFileName(const wstring& fileName) { m_fileName = fileName; }
	void LoadingFile() { ++m_LoadedFileNum; }

private:
	wstring				m_fileName;
	UINT				m_maxFileNum;
	UINT				m_LoadedFileNum;
};