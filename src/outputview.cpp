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
#include "outputview.h"

///////////////////////////////////////////////////////////////////////////////
//
LRESULT COutputView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwStyle;
    HRESULT hr = S_OK;
    RECT rect;
    
    dwStyle = WS_CHILD | 
                WS_BORDER | 
                WS_VISIBLE |
				WS_VSCROLL |
				ES_AUTOVSCROLL |
				ES_MULTILINE |
				ES_READONLY;

    ZeroMemory(&rect, sizeof(rect));
    
    if(m_Output.Create(L"EDIT", m_hWnd, &rect, NULL, dwStyle, 0) == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

	// m_Output.SendMessage(EM_SETEVENTMASK, 0, ENM_PROTECTED);
	m_Output.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));

Cleanup:

    return 0;
}

LRESULT COutputView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rect;
    
    rect.top = 0;
    rect.left = 0;
    rect.right = LOWORD(lParam);
    rect.bottom = HIWORD(lParam);

    m_Output.SetWindowPos(NULL, 
                    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
                    SWP_NOZORDER | SWP_NOREDRAW);

    bHandled = TRUE;
    
    return 0;
}

void COutputView::OutputLineA(LPCSTR pszLine, DWORD cch)
{
	if(cch == 0)
	{
		cch = strlen(pszLine);
	}

	if(m_ConvertBuf.size() < cch + 1)
	{
		m_ConvertBuf.resize(cch + 1);
	}

	int cchWide = MultiByteToWideChar(CP_UTF8, 0, pszLine, cch, &m_ConvertBuf[0], m_ConvertBuf.size());
    m_ConvertBuf[cchWide] = '\0';

	OutputLineW(&m_ConvertBuf[0]);
}

void COutputView::OutputLineW(LPCWSTR pszLine)
{
    int nLength, nStart;
    LRESULT lRes;
    int x,y;
    RECT rcArea;
    
	nStart = nLength = m_Output.GetWindowTextLength(); 
    m_Output.SendMessage(EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);
    m_Output.SendMessage(EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)pszLine);

    nLength = m_Output.GetWindowTextLength(); 
    m_Output.SendMessage(EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);
    m_Output.SendMessage(EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)L"\r\n");

#if 0
	// protect the region
	CHARFORMAT fmt;
	fmt.cbSize = sizeof(CHARFORMAT);
	fmt.dwMask = CFM_PROTECTED;
	fmt.dwEffects = CFE_PROTECTED;

    m_Output.SendMessage(EM_SETSEL, (WPARAM)nStart, (LPARAM)nLength);
	m_Output.SendMessage(EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&fmt);
#endif

	// make sure that line is on screen
	lRes = m_Output.SendMessage(EM_POSFROMCHAR, (WPARAM)nLength, 0);

    x = (int)LOWORD(lRes);
    y = (int)HIWORD(lRes);

    m_Output.GetClientRect(&rcArea);
    if(y > rcArea.bottom)
    {
        // yScrollPos = GetScrollPos(SB_VERT);
        m_Output.SetScrollPos(SB_VERT, y - rcArea.top, TRUE);
    }
}

LRESULT COutputView::OnProtected(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	return 1;
}

