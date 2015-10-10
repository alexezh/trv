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
#include "Trace.h"
#include "Traceline.h"
#include "TraceCollection.h"
#include "queryTraceSource.h"
#include "apphost.h"
#include "bitset.h"
#include "stringreader.h"
#include "make_unique.h"
#include "TraceLineparser.h"
#include "error.h"
#include "log.h"
#include "file.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> TraceSourceProxy::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void TraceSourceProxy::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->Inherit(Queryable::GetTemplate(iso));

	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->Set(String::NewFromUtf8(iso, "setFormat"), FunctionTemplate::New(iso, jsSetFormat));
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "lineCount"), jsLineCountGetter);
	tmpl_proto->Set(String::NewFromUtf8(iso, "line"), FunctionTemplate::New(iso, jsGetLine));
	tmpl_proto->Set(String::NewFromUtf8(iso, "fromRange"), FunctionTemplate::New(iso, jsFromRange));

	tmpl->SetClassName(String::NewFromUtf8(iso, "TraceSourceProxy"));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	_Template = UniquePersistent<FunctionTemplate>(iso, tmpl);

	TraceLine::Init(iso);
	TraceCollection::Init(iso);
}

TraceSourceProxy::TraceSourceProxy(const v8::Handle<v8::Object>& handle, const std::shared_ptr<CTraceSource>& source)
	: Queryable(handle)
	, _Source(source)
{
	_Op = std::make_shared<QueryOpTraceSource>(source);
}

void TraceSourceProxy::jsNew(const FunctionCallbackInfo<Value> &args)
{
	if (args.Length() != 1)
	{
		ThrowTypeError("invalid number of parameters");
	}
	auto source = reinterpret_cast<std::shared_ptr<CTraceSource>*>(args[0].As<External>()->Value());

	TraceSourceProxy *pThis = new TraceSourceProxy(args.This(), *source);

	args.GetReturnValue().Set(args.This());
}

void TraceSourceProxy::jsSetFormat(const v8::FunctionCallbackInfo<v8::Value> &args)
{
	LOG("@");
	TraceSourceProxy * pThis = UnwrapThis<TraceSourceProxy>(args.This());
	if (!(args.Length() == 2 && args[0]->IsString() && args[1]->IsString()))
	{
		ThrowTypeError("invalid number of parameters");
	}
	String::Utf8Value strFormat(args[0]->ToString());
	String::Utf8Value strSep(args[1]->ToString());

	GetCurrentHost()->SetTraceFormat(std::string(*strFormat, strFormat.length()).c_str(), 
			std::string(*strSep, strSep.length()).c_str());
}

void TraceSourceProxy::jsGetLine(const FunctionCallbackInfo<Value> &args)
{
	auto pThis = UnwrapThis<TraceSourceProxy>(args.This());
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		ThrowTypeError("invalid number of FunctionCallbackInfo<Value>. Format getline(int)");
	}

	auto jsLine = TraceLine::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(1, &args[0]);

	args.GetReturnValue().Set(jsLine);
}

void TraceSourceProxy::jsFromRange(const v8::FunctionCallbackInfo<Value> &args)
{
	if(args.Length() != 2)
	{
		ThrowTypeError("invalid number of FunctionCallbackInfo<Value>. Format fromrange(int, int)");
	}

	// return TraceSourceProxy collection object as a set of lines between 
	Local<Value> v[2];
	v[0] = args[0];
	v[1] = args[1];
	args.GetReturnValue().Set(TraceCollection::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(2, v));
}

void TraceSourceProxy::jsLineCountGetter(Local<String> property,
											const PropertyCallbackInfo<v8::Value>& info)
{
	TraceSourceProxy * pThis = UnwrapThis<TraceSourceProxy>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Source->GetLineCount()));
}

size_t TraceSourceProxy::ComputeCount()
{
	if (_Source == nullptr)
		return 0;

	return _Source->GetLineCount();
}

} // Js
