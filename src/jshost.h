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
class CTraceCollection;

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
		_pTraceColl = nullptr;
	}

	// initialize console
	void Init(CTraceCollection * pColl);

	void ProcessInputLine(const char * pszLine);

	size_t GetHistoryCount();
	bool GetHistoryEntry(size_t idx, std::string& entry);

	// compute line color
	BYTE GetLineColor(DWORD dwLine);

public:
	void OnViewCreated(Js::View*);
	void OnHistoryCreated(Js::History*);

	// trace storage
	LineInfo& GetLine(size_t idx) override;
	size_t GetLineCount() override;
	void UpdateLinesActive(CBitSet & set, int change) override;
	bool SetTraceFormat(const char * psz) override;
	void RefreshView() override;

	// console access
	void OutputLine(const char * psz) override;
	void SetViewLayout(double cmdHeight, double outHeight) override;

private:
	void QueueInput(std::unique_ptr<std::function<void()> > && item);
	void ExecuteString(const std::string & line);
	void ReportException(v8::TryCatch& try_catch);

    static void WINAPI ScriptThreadInit(void * pCtx);
    void ScriptThread();

private:
	CTraceApp * _pApp;

	HANDLE _hSem;
	CRITICAL_SECTION _cs;
	std::queue<std::unique_ptr<std::function<void()> > > _InputQueue;

	CTraceCollection* _pTraceColl;

	Js::View* _pView;
	Js::History* _pHistory;
	v8::Persistent<v8::ObjectTemplate> _Global;
	v8::Persistent<v8::Context> _Context;
};

