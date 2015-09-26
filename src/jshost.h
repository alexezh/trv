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

#include "js/init.h"
#include "js/apphost.h"

class CTraceApp;
class CTraceSource;

class AutoCS
{
public:
	AutoCS(CRITICAL_SECTION & cs)
		: _cs(cs)
	{
		EnterCriticalSection(&_cs);
	}

	~AutoCS()
	{
		LeaveCriticalSection(&_cs);
	}
private:
	CRITICAL_SECTION & _cs;
};

///////////////////////////////////////////////////////////////////////////////
// script keeps things around as long as there is a context active
// we cannot just enter context every time. So we are going to run loop 
// on different thread
class JsHost : public Js::IAppHost
{
public:
	JsHost(CTraceApp * pApp)
		: _pView(nullptr)
		, _pHistory(nullptr)
	{
		_pApp = pApp;
	}

	// initialize console
	void Init(const std::shared_ptr<CTraceSource>& pColl);

	void ProcessInputLine(const char * pszLine);
	void ProcessAccelerator(uint8_t modifier, uint16_t key);

	size_t GetHistoryCount();
	bool GetHistoryEntry(size_t idx, std::string& entry);

	// compute line color
	BYTE GetLineColor(DWORD dwLine);

public:
	void OnViewCreated(Js::View*) override;
	void OnHistoryCreated(Js::History*) override;
	void OnDotExpressionsCreated(Js::DotExpressions*) override;
	void OnShortcutsCreated(Js::Shortcuts*) override;
	void OnTaggerCreated(Js::Tagger*) override;

	void LoadTrace(const char* pszName, int startPos, int endPos) override;

	const std::string& GetAppDataDir() override
	{
		return m_AppDataPath;
	}

	// trace storage
	std::shared_ptr<CTraceSource> GetFileTraceSource() override;

	const LineInfo& GetLine(size_t idx) override;
	size_t GetLineCount() override;
	size_t GetCurrentLine() override;
	void AddShortcut(uint8_t modifier, uint16_t key) override;

	void ConsoleSetConsole(const std::string& szText) override;
	void ConsoleSetFocus() override;

	bool SetTraceFormat(const char * pszFormat, const char* pszSep) override;
	void RefreshView() override;
	void SetViewSource(const std::shared_ptr<CBitSet>& scope) override;
	void SetFocusLine(DWORD nLine) override;

	// console access
	void OutputLine(const char * psz) override;
	void SetViewLayout(double cmdHeight, double outHeight) override;
	void SetColumns(const std::vector<std::string>& name) override;

	void ReportException(v8::Isolate* isolate, v8::TryCatch& try_catch) override;

private:
	std::string GetKnownPath(REFKNOWNFOLDERID id);
	void QueueInput(std::unique_ptr<std::function<void(v8::Isolate*)> > && item);
	void ExecuteString(v8::Isolate* isolate, const std::string & line);
	void ExecuteStringAsDotExpression(v8::Isolate* iso, const std::string & line);
	void ExecuteStringAsScript(v8::Isolate* iso, const std::string & line);

    static void WINAPI ScriptThreadInit(void * pCtx);
    void ScriptThread();

private:
	CTraceApp * _pApp;

	HANDLE _hSem;
	CRITICAL_SECTION _cs;
	std::queue<std::unique_ptr<std::function<void(v8::Isolate*)> > > _InputQueue;

	std::shared_ptr<CTraceSource> _pFileTraceSource;

	Js::View* _pView = nullptr;
	Js::History* _pHistory = nullptr;
	Js::Shortcuts* _pShortcuts = nullptr;
	Js::DotExpressions* _pDotExpressions = nullptr;
	Js::Tagger* _pTagger = nullptr;
	v8::UniquePersistent<v8::ObjectTemplate> _Global;
	v8::UniquePersistent<v8::Context> _Context;

	std::string m_AppDataPath;
};

