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
#include "clipboard.h"
///////////////////////////////////////////////////////////////////////////////
// external classes
class CTraceApp;
class CTraceView;
class CPersistComponentContainer;

class CTraceFile;
class CTextTraceFile;
class CTraceSource;
class CCommandView;
class COutputView;
class JsHost;
enum class ColumnId;

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
	void SetClipboardHandler(IClipboardHandler* pHandler)
	{
		m_pClipboardHandler = pHandler;
	}
protected:
	void RebuildDock();

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnLoadBegin(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLoadBlock(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnLoadEnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnQueueWork(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnToggleHide(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnNavList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnNavOutput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnNavConsole(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnFindNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


	BEGIN_MSG_MAP(CTraceApp)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)

		MESSAGE_HANDLER(WM_LOAD_BLOCK, OnLoadBlock)
		MESSAGE_HANDLER(WM_LOAD_END, OnLoadEnd)
		MESSAGE_HANDLER(WM_LOAD_BEGIN, OnLoadBegin)
		MESSAGE_HANDLER(WM_QUEUE_WORK, OnQueueWork)

		COMMAND_HANDLER(ID_TOGGLEHIDE, 1, OnToggleHide)
		COMMAND_HANDLER(ID_VIEW_TRACEWINDOW, 1, OnNavList)
		COMMAND_HANDLER(ID_VIEW_MESSAGEWINDOW, 1, OnNavOutput)
		COMMAND_HANDLER(ID_VIEW_CONSOLEWINDOW, 1, OnNavConsole)
		COMMAND_HANDLER(ID_VIEW_REFRESH, 1, OnRefresh)
		COMMAND_HANDLER(ID_EDIT_COPY, 1, OnCopy)
		COMMAND_HANDLER(IDM_ABOUT, 0, OnAbout)

		COMMAND_HANDLER(ID_TOGGLEHIDE, 0, OnToggleHide)
		COMMAND_HANDLER(ID_VIEW_TRACEWINDOW, 0, OnNavList)
		COMMAND_HANDLER(ID_VIEW_MESSAGEWINDOW, 0, OnNavOutput)
		COMMAND_HANDLER(ID_VIEW_CONSOLEWINDOW, 0, OnNavConsole)
		COMMAND_HANDLER(ID_VIEW_REFRESH, 0, OnRefresh)
		COMMAND_HANDLER(ID_EDIT_COPY, 0, OnCopy)
	END_MSG_MAP()

public:

	bool HandleJsAccelerators(MSG& msg);

	CPersistComponentContainer * PPersist()
	{
		return m_pPersist;
	}
	CTextTraceFile * PFile()
	{
		return m_pFile.get();
	}
	CTraceSource * PFileColl()
	{
		return m_pFileColl.get();
	}

	CTraceView * PTraceView()
	{
		return m_pTraceView;
	}
	JsHost * PJsHost()
	{
		return m_pJsHost;
	}
	COutputView * POutputView()
	{
		return m_pOutputView;
	}
	CCommandView * PCommandView()
	{
		return m_pCommandView;
	}

	// post work to app thread
	void PostWork(const std::function<void()> & func);
	void SetDockLayout(double cmdHeight, double outHeight);
	void SetTraceColumns(const std::vector<ColumnId>& columns);

	std::shared_ptr<CTraceSource> GetTraceSource();
	void SetTraceSource(const std::shared_ptr<CTraceSource>& src);

	void AddShortcut(uint8_t modifier, uint16_t key);

private:
	enum class SourceType
	{
		File,
		DebugOutput
	};
	SourceType m_SourceType;
	std::wstring m_File;

	Dock::HostWindow * m_pDock { nullptr };
	Dock::Table * m_pDockRoot { nullptr };
	size_t m_idxCmdColumn;
	size_t m_idxOutputColumn;

	// file to read
	std::shared_ptr<CTextTraceFile> m_pFile;
	std::shared_ptr<CTraceSource> m_pFileColl;

	JsHost * m_pJsHost { nullptr };

	// top level views;
	CTraceView * m_pTraceView { nullptr };
	CCommandView * m_pCommandView { nullptr };
	COutputView * m_pOutputView { nullptr };
	IClipboardHandler * m_pClipboardHandler { nullptr };

	// controls persistance
	CPersistComponentContainer * m_pPersist { nullptr };

	// maximum number of Mb to load
	DWORD m_cbMaxLoadWindow;

	// last Find expression
	std::wstring m_szLastFind;

	// map of BYTE + WORD
	std::set<DWORD> m_Keys;
};


