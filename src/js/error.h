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

#include "log.h"

namespace Js {

class V8RuntimeException : public std::exception
{
public:
	V8RuntimeException(const char* pszError)
		: std::exception(pszError)
	{
	}
};

inline void TryCatchCpp(const v8::FunctionCallbackInfo<v8::Value>& args, const std::function<v8::Local<v8::Value>()>& func)
{
	try
	{
		auto r(func());
		if (!r.IsEmpty())
			args.GetReturnValue().Set(r);
	}
	catch (V8RuntimeException e)
	{
		LOG(" v8 exception");
	}
}

inline void ThrowError(const char* pszText)
{
	v8::Isolate::GetCurrent()->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), pszText)));
	throw V8RuntimeException(pszText);
}

inline void ThrowTypeError(const char* pszText)
{
	v8::Isolate::GetCurrent()->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), pszText)));
	throw V8RuntimeException(pszText);
}

inline void ThrowSyntaxError(const char* pszText)
{
	v8::Isolate::GetCurrent()->ThrowException(v8::Exception::SyntaxError(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), pszText)));
	throw V8RuntimeException(pszText);
}

}
