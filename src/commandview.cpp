// Copyright (c) 2013 Alexandre Grigorovitch (alexezh@gmail.com).
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "stdafx.h"
#include "traceapp.h"
#include "jshost.h"
#include "commandview.h"
#include <atlenc.h>

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CInputCtrl::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = (wParam == VK_RETURN);

    return 0;
}

LRESULT CInputCtrl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(wParam == VK_RETURN)
    {
		m_HistoryIndex = -1;
        PostMessage(WM_EXECUTECOMMAND, 0, 0);    
        bHandled = TRUE;
    }
	else if(wParam == VK_UP)
	{
		PostMessage(WM_SCROLLHISTORY, -1, 0);
		bHandled = TRUE;
	}
	else if (wParam == VK_DOWN)
	{
		PostMessage(WM_SCROLLHISTORY, 1, 0);
		bHandled = TRUE;
	}
	else if (wParam == VK_ESCAPE)
	{
		PostMessage(WM_CLEARCOMMAND, 1, 0);
		bHandled = TRUE;
	}
	else
    {
		m_HistoryIndex = -1;
        bHandled = FALSE;
    }

    return 0;
}

LRESULT CInputCtrl::OnExecuteCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CAtlStringW szText;
    GetWindowText(szText);
	if(szText.GetLength() == 0)
	{
		return 0;
	}

	std::vector<char> szTextA;
	szTextA.resize(szText.GetLength() * 3 + 1);
	int cch = ATL::AtlUnicodeToUTF8((LPCWSTR)szText, szText.GetLength(), &szTextA[0], szTextA.size()); 
	assert(cch != 0);
	szTextA[cch] = '\0';

    m_pApp->PJsHost()->ProcessInputLine(&szTextA[0]);
    SetWindowText(L"");

    return 0;
}

LRESULT CInputCtrl::OnClearCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetWindowText(L"");
	return 0;
}

LRESULT CInputCtrl::OnScrollHistory(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	int dir = wParam;
	if(m_HistoryIndex == -1)
	{
		if(dir == -1)
		{
			m_HistoryIndex = m_pApp->PJsHost()->GetHistoryCount()-1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		m_HistoryIndex += dir;
		m_HistoryIndex = max(m_HistoryIndex, 0);
		m_HistoryIndex = min(m_HistoryIndex, m_pApp->PJsHost()->GetHistoryCount()-1);
	}
	
	// load history into editor
	std::string szText;
	if(!m_pApp->PJsHost()->GetHistoryEntry(m_HistoryIndex, szText))
	{
		m_HistoryIndex = -1;
		assert(false);
		return 0;
	}

	SetText(szText);

	return 0;
}

void CInputCtrl::SetText(const std::string& szText)
{
	std::vector<WCHAR> szTextW;

	szTextW.resize(szText.length() + 1);
	int cchWide = MultiByteToWideChar(CP_UTF8, 0, szText.c_str(), szText.length(), &szTextW[0], szTextW.size());
	szTextW[cchWide] = '\0';

	SetWindowText(&szTextW[0]);
	SendMessage(EM_SETSEL, szTextW.size(), szTextW.size());
}

/////////////////////////////////////////////////////////////////////////////
//
LRESULT CCommandView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwStyle;
    HRESULT hr = S_OK;
    RECT rect;
    
    dwStyle = WS_CHILD | 
                WS_BORDER | 
                WS_VISIBLE;

    ZeroMemory(&rect, sizeof(rect));
    
	CWindow input;
    if(input.Create(L"EDIT", m_hWnd, &rect, NULL, dwStyle, 0) == NULL)
    {
		assert(false);
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

	m_Input.SubclassWindow(input.m_hWnd);
	m_Input.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));

Cleanup:

    return 0;
}

LRESULT CCommandView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rect;
    
    rect.top = 0;
    rect.left = 0;
    rect.right = LOWORD(lParam);
    rect.bottom = HIWORD(lParam);

    m_Input.SetWindowPos(NULL, 
                    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
                    SWP_NOZORDER | SWP_NOREDRAW);

    bHandled = TRUE;
    
    return 0;
}

void CCommandView::SetText(const std::string& text)
{
	m_Input.SetText(text);
}
