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

struct CStringRef;
class CBitSet;
namespace Js {

class View;
class History;

class IAppHost
{
public:

	virtual void OnViewCreated(View*) = 0;
	virtual void OnHistoryCreated(History*) = 0;

	// trace storage
	virtual LineInfo& GetLine(size_t idx) = 0;
	virtual size_t GetLineCount() = 0;
	virtual void UpdateLinesActive(CBitSet & set, int change) = 0;
	// set trace format
	virtual bool SetTraceFormat(const char * psz) = 0;

	virtual void RefreshView() = 0;
	// console access
	virtual void OutputLine(const char * psz) = 0;

	// set height of panes in %
	virtual void SetViewLayout(double cmdHeight, double outHeight) = 0;
};

inline IAppHost * GetCurrentHost()
{
	auto ctx = v8::Context::GetEntered();
	auto data = ctx->GetEmbedderData(1);
	v8::Local<v8::External> dataExt = data.As<v8::External>();
	IAppHost * pHost = (IAppHost*)dataExt->Value();
	return pHost;
}

} // Js
