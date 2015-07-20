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

namespace Js {

class CommandViewProxy : public BaseObject<CommandViewProxy>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso) 
	{ 
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

private:
	CommandViewProxy(const v8::Handle<v8::Object>& handle);

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsSetText(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsSetFocus(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsVisibleGetter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsVisibleSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info);

private:

	static v8::Persistent<v8::FunctionTemplate> _Template;
};

} // Js

