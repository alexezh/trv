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
#include "bitset.h"
#include "objectwrap.h"
#include "queryable.h"

class CBitSet;
class CTraceSource;

namespace Js {

class TraceSourceProxy : public Queryable
{
public:
	TraceSourceProxy(const v8::Handle<v8::Object>& handle, const std::shared_ptr<CTraceSource>& source);

	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	static inline TraceSourceProxy* Unwrap(v8::Handle<v8::Object> handle)
	{
		return static_cast<TraceSourceProxy*>(Queryable::Unwrap(handle));
	}

	static TraceSourceProxy* TryGetTraceSource(const v8::Local<v8::Object> & obj)
	{
		auto res = obj->FindInstanceInPrototypeChain(GetTemplate(v8::Isolate::GetCurrent()));
		if (res.IsEmpty())
		{
			return nullptr;
		}

		auto pThis = Unwrap(obj);
		return pThis;
	}

	const std::shared_ptr<QueryOp>& Op() override { return _Op; }

	const std::shared_ptr<CTraceSource>& Source() override { return _Source; }

protected:

	size_t ComputeCount() override;

private:
	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	// returns a line by index
	static void jsGetLine(const v8::FunctionCallbackInfo<v8::Value> &args);
	// returns set of lines based on range
	static void jsFromRange(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsLineCountGetter(v8::Local<v8::String> property, 
												const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsSetFormat(const v8::FunctionCallbackInfo<v8::Value> &args);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	// filter is either string expression or an object
	std::shared_ptr<QueryOp> _Op;
	std::shared_ptr<CTraceSource> _Source;
};

} // Js
