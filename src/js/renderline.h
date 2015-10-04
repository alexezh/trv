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

#include "strstr.h"
#include "stringref.h"
#include "lineinfo.h"
#include "objectwrap.h"
#include "viewlinecache.h"

class CBitSet;

namespace Js {

class RenderLine : public BaseObject<RenderLine> 
{
public:
	static void Init(v8::Isolate* iso);
	static void InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target);
	static v8::Local<v8::FunctionTemplate> GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	const LineInfo& Line() { return _Line; }

private:
	RenderLine(const v8::Handle<v8::Object>& handle, int lineNum);

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);

	static void jsGetUser(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsIndexGetter(v8::Local<v8::String> property,
		const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsTimeGetter(v8::Local<v8::String> property,
		const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsThreadGetter(v8::Local<v8::String> property,
												const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsMsgGetter(v8::Local<v8::String> property,
		const v8::PropertyCallbackInfo<v8::Value>& info);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	ViewLine _Line;
};

} // Js
