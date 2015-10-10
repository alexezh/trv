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

#include "lineinfo.h"
#include "file.h"

struct CStringRef;
class CBitSet;
class ViewLineCache;
class ViewLine;

namespace Js {

class View;
class History;
class DotExpressions;
class Shortcuts;
class Tagger;
class Dollar;

class IAppHost
{
public:

	virtual void OnDollarCreated(Dollar*) = 0;
	virtual void OnViewCreated(View*) = 0;
	virtual void OnHistoryCreated(History*) = 0;
	virtual void OnDotExpressionsCreated(DotExpressions*) = 0;
	virtual void OnShortcutsCreated(Shortcuts*) = 0;
	virtual void OnTaggerCreated(Tagger*) = 0;

	virtual void LoadTrace(const char* pszName, int startPos, int endPos) = 0;
	virtual void OnTraceLoaded() = 0;

	virtual const std::string& GetAppDataDir() = 0;

	virtual void ConsoleSetConsole(const std::string& szText) = 0;
	virtual void ConsoleSetFocus() = 0;

	// trace storage
	virtual std::shared_ptr<CTraceSource> GetFileTraceSource() = 0;

	virtual const LineInfo& GetLine(size_t idx) = 0;
	virtual size_t GetLineCount() = 0;

	virtual size_t GetCurrentLine() = 0;
	virtual void AddShortcut(uint8_t modifier, uint16_t key) = 0;

	// set trace format
	virtual bool SetTraceFormat(const char * pszFormat, const char* pszSep) = 0;
	virtual void RefreshView() = 0;
	virtual void SetViewSource(const std::shared_ptr<CBitSet>& scope) = 0;
	virtual void SetFocusLine(DWORD nLine) = 0;

	virtual void RequestViewLine() = 0;
	virtual void RegisterRequestLineHandler(const std::function<std::unique_ptr<ViewLine>(v8::Isolate*, DWORD idx)>&) = 0;
	virtual void ResetViewCache() = 0;

	// console access
	virtual void OutputLine(const char * psz) = 0;

	// set height of panes in %
	virtual void SetViewLayout(double cmdHeight, double outHeight) = 0;
	virtual void SetColumns(const std::vector<std::string>& name) = 0;

	virtual void ReportException(v8::Isolate* isolate, v8::TryCatch& try_catch) = 0;
};

inline IAppHost * GetCurrentHost()
{
	auto ctx = v8::Isolate::GetCurrent()->GetCurrentContext();
	auto data = ctx->GetEmbedderData(1);
	v8::Local<v8::External> dataExt = data.As<v8::External>();
	IAppHost * pHost = (IAppHost*)dataExt->Value();
	return pHost;
}

} // Js
