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

#include "resource.h"
#include "traceapp.h"
#include "traceview.h"
#include "commandview.h"
#include "outputview.h"
#include "jshost.h"
#include "about.h"
#include "file.h"
#include "textfile.h"
#include "make_unique.h"
#include "stringutils.h"
#include "log.h"

class CMyModule : public CAtlExeModuleT<CMyModule>
{
};

CMyModule _Module;

///////////////////////////////////////////////////////////////////////////////
//
HINSTANCE g_hInst = NULL;
CTraceApp * g_pApp;


///////////////////////////////////////////////////////////////////////////////
//
CTraceApp::CTraceApp()
	: m_pTraceView(NULL)
	, m_pCommandView(NULL)
	, m_pOutputView(NULL)
	, m_pJsHost(NULL)
{
}

CTraceApp::~CTraceApp()
{
	delete m_pTraceView;
	delete m_pCommandView;
	delete m_pOutputView;
	delete m_pPersist;
}

HRESULT CTraceApp::Init(LPWSTR lpCmdLine)
{
	HRESULT hr = S_OK;
	HMENU hMenu;
	HICON hIcon;
	std::wstring szTitle;

	// first initialize persistance container
	m_pPersist = new CPersistComponentContainer;
	if (m_pPersist == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}
	m_pPersist->Init();

	// save file name for later
	m_SourceType = SourceType::File;
	if (wcslen(lpCmdLine) > 0)
	{
		if (lpCmdLine[0] == '-')
		{
			if (wcscmp(lpCmdLine, L"-debug") == 0)
			{
				m_SourceType = SourceType::DebugOutput;
			}
		}
	}

	if (m_SourceType == SourceType::File)
	{
		m_File = lpCmdLine;
	}

	// create window
	hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_TRV));
	if (hMenu == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_TRV));
	if (hIcon == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	// load up to 512Mb at once
	m_cbMaxLoadWindow = 1024 * 1024 * 512;

	// set window name
	szTitle = L"TraceView -";
	szTitle += m_File;

	if (Create(NULL, NULL, szTitle.c_str(), WS_OVERLAPPEDWINDOW, 0, hMenu, NULL) == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	SetIcon(hIcon);
	SetIcon(hIcon, FALSE);

Cleanup:

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HRESULT hr = S_OK;
	RECT rect;

	GetClientRect(&rect);

	if (m_SourceType == SourceType::File)
	{
		// open trace file
		m_pFile = std::make_shared<CTextTraceFile>();
		if (m_pFile == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		// create collection
		m_pFileColl = m_pFile;
	}
	else
	{
		// auto coll = new CDebugOutputTraceSource();
		// coll->Init();
		// m_pFileColl = coll;
		assert(false);
	}

	// create dock window
	m_pDock = new Dock::HostWindow;
	if (m_pDock->Create(m_hWnd, &rect) == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	ZeroMemory(&rect, sizeof(rect));

	// create trace view
	m_pTraceView = new CTraceView;
	m_pTraceView->Init(this);

	// create js host
	// m_pFile->CreateCollection(&m_pJsColl);
	m_pJsHost = new JsHost(this);
	m_pJsHost->Init(m_pFileColl);

	// display file collection initially
	m_pTraceView->SetTraceSource(m_pFileColl);

	if (m_pTraceView->Create(m_pDock->m_hWnd, &rect) == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	// create command view
	m_pCommandView = new CCommandView(this);
	if (m_pCommandView->Create(m_pDock->m_hWnd, &rect) == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	// create output view
	m_pOutputView = new COutputView(this);
	if (m_pOutputView->Create(m_pDock->m_hWnd, &rect) == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	// add necessary panes to the dock
	RebuildDock();

	// set focus to listview
	m_pTraceView->SetFocus();

	m_pOutputView->OutputLineA("Welcome to trv.js\r\nFor help type help() or go to http://github.com/alexezh/trv/wiki\r\n");

	// start file load
	LoadFile(0, -1);

	// wait with host initialization until we have file

Cleanup:

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceApp::OnLoadBegin()
{
	PostMessage(WM_LOAD_BEGIN, 0, 0);
}

void CTraceApp::OnLoadEnd(HRESULT hr)
{
	PostMessage(WM_LOAD_END, 0, 0);
}

void CTraceApp::OnLoadBlock()
{
	PostMessage(WM_LOAD_BLOCK, 0, 0);
}

void CTraceApp::LoadFile(const std::string& file, int startPos, int endPos)
{
	LOG("@%p file=%s", file.c_str());
	StringToWString(file, m_File);
	LoadFile(startPos * 1024 * 1024, (endPos == -1) ? -1 : endPos * 1024 * 1024);
}

void CTraceApp::LoadFile(QWORD nStart, QWORD nStop)
{
	HRESULT hr = S_OK;

	if (m_File.length() == 0)
	{
		return;
	}

	// open file
	hr = m_pFile->Open(m_File.c_str(), this);
	if (FAILED(hr))
	{
		std::wstring s;
		s = std::wstring(L"cannot open file ") + m_File + L"\r\n";
		m_pOutputView->OutputLineW(s.c_str());
		return;
	}

	m_pFile->Load(m_cbMaxLoadWindow);
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnLoadBegin(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pOutputView->OutputLineA("Loading file");
	return 0;
}

LRESULT CTraceApp::OnLoadBlock(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pOutputView->OutputTextA(".");
	return 0;
}

LRESULT CTraceApp::OnLoadEnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pOutputView->OutputLineA("\r\nLoad complete");

	// refresh our collection
	m_pTraceView->LoadView();
	m_pJsHost->OnTraceLoaded();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pPersist->SaveAll();

	PostQuitMessage(0);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pDock->SetWindowPos(NULL,
		0, 0, LOWORD(lParam), HIWORD(lParam),
		SWP_NOZORDER | SWP_NOREDRAW);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pTraceView->SetFocus();
	bHandled = TRUE;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceApp::Post(const std::function<void()> & func)
{
	auto item = make_unique<std::function<void()> >(func);
	PostMessage(WM_QUEUE_WORK, 0, (LPARAM) item.release());
}

LRESULT CTraceApp::OnQueueWork(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	std::unique_ptr<std::function<void()> > item;
	item.reset((std::function<void()>*)lParam);
	(*item)();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (m_pClipboardHandler)
		m_pClipboardHandler->OnCopy();
	else
		m_pTraceView->OnCopy();

	bHandled = TRUE;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_pTraceView->Repaint();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnNavList(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_pTraceView->SetFocus();

	bHandled = TRUE;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnNavOutput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_pOutputView->SetFocus();

	bHandled = TRUE;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceApp::OnNavConsole(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_pCommandView->SetFocus();

	bHandled = TRUE;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceApp::SetDockLayout(double cmdHeight, double outHeight)
{
	// do not allow shrinking cmd below to
	cmdHeight = (cmdHeight < 10) ? 10 : cmdHeight;
	outHeight = min(0.8, outHeight);

	m_pDockRoot->ResizeColumn(m_idxCmdColumn, cmdHeight);
	m_pDockRoot->ResizeColumn(m_idxOutputColumn, outHeight);

	m_pDock->UpdateDock();
}

void CTraceApp::SetTraceColumns(const std::vector<ColumnId>& columns)
{
	m_pTraceView->SetColumns(columns);
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceApp::RebuildDock()
{
	m_pDockRoot = new Dock::Table(Dock::Table::Vertical);
	m_pDock->SetRootElement(m_pDockRoot);

	m_pDockRoot->AddColumn(Dock::Table::SizeMax, 0, new Dock::Window(m_pTraceView));
	m_pDockRoot->AddColumn(Dock::Table::SizePixel, 3, nullptr);
	m_idxOutputColumn = m_pDockRoot->AddColumn(Dock::Table::SizePercentage, 0.4, new Dock::Window(m_pOutputView));
	m_pDockRoot->AddColumn(Dock::Table::SizePixel, 3, nullptr);
	m_idxCmdColumn = m_pDockRoot->AddColumn(Dock::Table::SizePixel, 20, new Dock::Window(m_pCommandView));

	m_pDock->UpdateDock();
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceApp::AddShortcut(uint8_t modifier, uint16_t key)
{
	m_Keys.insert(static_cast<int>(modifier) << 16 | static_cast<WORD>(key));
}

///////////////////////////////////////////////////////////////////////////////
//
bool CTraceApp::HandleJsAccelerators(MSG& msg)
{
	byte modifier = 0;

	if ((GetAsyncKeyState(VK_CONTROL) & 0x80000000) != 0)
		modifier |= FCONTROL;

	if ((GetAsyncKeyState(VK_MENU) & 0x80000000) != 0)
		modifier |= FALT;

	auto it = m_Keys.find(static_cast<int>(modifier) << 16 | static_cast<WORD>(msg.wParam));
	if (it == m_Keys.end())
		return false;

	m_pJsHost->ProcessAccelerator(modifier, msg.wParam);

	return false;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT InitTraceApp(LPWSTR lpCmdLine, int nCmdShow)
{
	HRESULT hr = S_OK;

	g_pApp = new CTraceApp;
	if (g_pApp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	IFC(g_pApp->Init(lpCmdLine));

	/* Make the window visible; update its client area; and return "success" */
	g_pApp->ShowWindow(nCmdShow);
	g_pApp->UpdateWindow();

Cleanup:

	return hr;
}

int PASCAL wWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nCmdShow)
{
	HRESULT hr = S_OK;
	MSG  msg;
	HACCEL hAccelTable;

	g_hInst = hInstance;

	//required to use the common controls
	(void) LoadLibrary(TEXT("msftedit.dll"));
	InitCommonControls();

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR) IDC_TRV);

	/* Perform initializations that apply to a specific instance */
	IFC(InitTraceApp(lpCmdLine, nCmdShow));

	/* Acquire and dispatch messages until a WM_QUIT uMessage is received. */
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (TranslateAccelerator(g_pApp->m_hWnd, hAccelTable, &msg))
		{
			continue;
		}

		if (msg.message == WM_KEYDOWN && g_pApp && g_pApp->HandleJsAccelerators(msg))
		{
			continue;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;

Cleanup:

	return FALSE;
}




