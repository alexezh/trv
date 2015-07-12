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
#include "init.h"
#include "dollar.h"
#include "query.h"
#include "trace.h"
#include "view.h"
#include "history.h"
#include "tid.h"
#include "dotexpressions.h"

namespace Js {

void InitRuntimeTemplate(v8::Isolate* iso, v8::Handle<v8::ObjectTemplate> & target)
{
	Dollar::Init(iso);
	Trace::Init(iso);
	Query::Init(iso);
	View::Init(iso);
	History::Init(iso);
	MatchTid::Init(iso, target);
	DotExpressions::Init(iso);
}

bool InitRuntime(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	Dollar::InitInstance(iso, target);
	return Dollar::ImportFile("trv.std.js");
}

} // Js

