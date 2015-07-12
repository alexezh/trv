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
#include "resource.h"

#include "defs.h"
#include "dock.h"
#include "file.h"

///////////////////////////////////////////////////////////////////////////////
// external classes
class CTraceApp;
class CTraceView;
class CPersistComponentContainer;

class CTraceFile;
class CTextTraceFile;
class CTraceCollection;
class CCommandView;
class COutputView;
class JsHost;

///////////////////////////////////////////////////////////////////////////////
// 
class CTraceApp 
    : public CWindowImpl<CTraceApp>
    , public CTraceFileLoadCallback
{
public:
    CTraceApp();
    ~CTraceApp();

    HRESULT Init(LPWSTR lpCmdLine);

public:
    void OnLoadBegin();
    void OnLoadEnd(HRESULT hr);
    void OnLoadBlock();
    
    void LoadFile(const std::string& file, int startPos, int endPos);
    void LoadFile(QWORD nStart, QWORD nEnd);

	size_t GetCurrentLine();

protected:    
    void RebuildDock();
        
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnLoadBlock(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLoadEnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnQueueWork(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnToggleHide(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNavList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNavOutput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNavConsole(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFindNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    
    BEGIN_MSG_MAP(CTraceApp)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)

        MESSAGE_HANDLER(WM_LOAD_BLOCK, OnLoadBlock)
        MESSAGE_HANDLER(WM_LOAD_END, OnLoadEnd)
		MESSAGE_HANDLER(WM_QUEUE_WORK, OnQueueWork)

		COMMAND_HANDLER(ID_TOGGLEHIDE, 1, OnToggleHide)
        COMMAND_HANDLER(ID_VIEW_TRACEWINDOW, 1, OnNavList)
        COMMAND_HANDLER(ID_VIEW_MESSAGEWINDOW, 1, OnNavOutput)
        COMMAND_HANDLER(ID_VIEW_CONSOLEWINDOW, 1, OnNavConsole)
        COMMAND_HANDLER(ID_EDIT_COPY, 1, OnCopy)
        COMMAND_HANDLER(ID_EDIT_FIND, 1, OnFind)
        COMMAND_HANDLER(ID_EDIT_FINDNEXT, 1, OnFindNext)
        COMMAND_HANDLER(IDM_ABOUT, 0, OnAbout)

        COMMAND_HANDLER(ID_TOGGLEHIDE, 0, OnToggleHide)
        COMMAND_HANDLER(ID_VIEW_TRACEWINDOW, 0, OnNavList)
        COMMAND_HANDLER(ID_VIEW_MESSAGEWINDOW, 0, OnNavOutput)
        COMMAND_HANDLER(ID_VIEW_CONSOLEWINDOW, 0, OnNavConsole)
        COMMAND_HANDLER(ID_EDIT_COPY, 0, OnCopy)
        COMMAND_HANDLER(ID_EDIT_FIND, 0, OnFind)
        COMMAND_HANDLER(ID_EDIT_FINDNEXT, 0, OnFindNext)
    END_MSG_MAP()

public:

    CPersistComponentContainer * PPersist() { return m_pPersist; }
    CTextTraceFile * PFile() { return m_pFile; }
	CTraceCollection * PFileColl() { return m_pFileColl; }

    CTraceView * PTraceView() { return m_pTraceView; }
    JsHost * PJsHost() { return m_pJsHost; }
    COutputView * POutputView() { return m_pOutputView; }

	// post work to app thread
	void PostWork(const std::function<void()> & func);
	void SetDockLayout(double cmdHeight, double outHeight);

private:
    std::wstring m_File;
	Dock::HostWindow * m_pDock;
	Dock::Table * m_pDockRoot;
	size_t m_idxCmdColumn;
	size_t m_idxOutputColumn;

    // file to read
    CTextTraceFile * m_pFile;
	CTraceCollection * m_pFileColl;

	JsHost * m_pJsHost;

    // top level views;
    CTraceView * m_pTraceView;
    CCommandView * m_pCommandView;
    COutputView * m_pOutputView;
    
    // controls persistance
    CPersistComponentContainer * m_pPersist;
    
    // maximum number of Mb to load
    DWORD m_cbMaxLoadWindow;
    
    // last Find expression
    CAtlStringW m_szLastFind;
};


