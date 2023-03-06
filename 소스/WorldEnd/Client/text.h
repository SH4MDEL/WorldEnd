#pragma once
#include "stdafx.h"

class Text
{
public:
	Text();
	~Text() = default;

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT timeElapsed);

	void SetTextBrush(const shared_ptr<ID2D1SolidColorBrush>& textBrush) { m_textBrush = textBrush; }
	void SetTextFormat(const shared_ptr<IDWriteTextFormat>& textFormat) { m_textFormat = textFormat; }

protected:
	D2D1_RECT_F							m_layoutRect;
	wstring								m_text;
	BOOL								m_enable;

	FLOAT								m_width;
	FLOAT								m_height;

	shared_ptr<ID2D1SolidColorBrush>	m_textBrush;
	shared_ptr<IDWriteTextFormat>		m_textFormat;
};

class LoadingText : public Text
{
public:
	LoadingText(UINT maxFileNum);
	~LoadingText() = default;

	void Render(const ComPtr<ID2D1DeviceContext2>& device) override;
	void Update(FLOAT timeElapsed) override;

	void SetFileName(wstring fileName) { m_fileName = fileName; }
	void LoadingFile() { ++m_LoadedFileNum; }

private:
	wstring				m_fileName;
	UINT				m_maxFileNum;
	UINT				m_LoadedFileNum;
};