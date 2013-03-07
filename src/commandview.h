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
#pragma once

#include "resource.h"       // main symbols

///////////////////////////////////////////////////////////////////////////////
// in order to avoid unnecessary \r\n char, post WM_EXECUTECOMMAND message when user types "return"
#define WM_EXECUTECOMMAND (WM_USER+1)
#define WM_SCROLLHISTORY (WM_USER+2)

class CTraceApp;

///////////////////////////////////////////////////////////////////////////////
//
class CInputCtrl : 
	public CWindowImpl<CInputCtrl>
{
public:
    CInputCtrl(CTraceApp * pApp)
		: m_HistoryIndex(-1)
	{
		m_pApp = pApp;
	}

    BEGIN_MSG_MAP(CInputCtrl)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_EXECUTECOMMAND, OnExecuteCommand)
        MESSAGE_HANDLER(WM_SCROLLHISTORY, OnScrollHistory)
    END_MSG_MAP()
    
    LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnExecuteCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScrollHistory(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    CTraceApp * m_pApp;
	int m_HistoryIndex;
};

///////////////////////////////////////////////////////////////////////////////
//
class CCommandView : 
	public CWindowImpl<CCommandView>
{
public:
    CCommandView(CTraceApp * pApp)
		: m_Input(pApp)
	{
		m_pApp = pApp;
	}

    BEGIN_MSG_MAP(CCommandView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()
    
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	CTraceApp * m_pApp;
	CInputCtrl m_Input;
};




