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

#include "bitset.h"
#include "objectwrap.h"
#include "js/query.h"
#include "viewlinecache.h"

using namespace v8;

namespace Js {

// TODO: rename to TraceViewProxy
class View;
class FilterItem;

class View : public BaseObject<View>
{
public:
	static void Init(Isolate* iso);
	static Local<FunctionTemplate> GetTemplate(Isolate* iso) 
	{ 
		return Local<FunctionTemplate>::New(iso, _Template);
	}

	void RefreshView();
private:
	View(const Local<Object>& handle);

	static void jsNew(const FunctionCallbackInfo<Value> &args);
	static void jsSetSource(const FunctionCallbackInfo<Value>& args);

	static void jsCurrentLineGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
	static void jsGetSelected(const FunctionCallbackInfo<Value>& args);

	static void jsSetViewLayout(const FunctionCallbackInfo<Value>& args);
	static void jsSetColumns(const FunctionCallbackInfo<Value>& args);
	static void jsSetFocusLine(const FunctionCallbackInfo<Value>& args);
	static void jsResetCache(const FunctionCallbackInfo<Value>& args);
	static void jsOnRender(const FunctionCallbackInfo<Value>& args);

	std::unique_ptr<ViewLine> HandleLineRequest(v8::Isolate* iso, DWORD idx);

private:
	static Persistent<FunctionTemplate> _Template;
	v8::UniquePersistent<Function> m_OnRender;
	std::shared_ptr<ViewLineCache> m_LineCache;
};

} // Js

