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
#include "tid.h"
#include "objectwrap.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

static char s_InvalidArgs[] = "Invalid number of FunctionCallbackInfo<Value>. Format matchtid(int)";
UniquePersistent<FunctionTemplate> MatchTid::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void MatchTid::Init(v8::Isolate* iso)
{
	LOG("");
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	tmpl->SetClassName(String::NewFromUtf8(iso, "MatchTid"));
	_Template.Reset(iso, tmpl);
}

void MatchTid::InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	target->Set(String::NewFromUtf8(iso, "MatchTid"), MatchTid::GetTemplate(iso)->GetFunction());
}

MatchTid* MatchTid::TryGetMatchTid(const Local<Object> & obj)
{
	auto res = obj->FindInstanceInPrototypeChain(GetTemplate(Isolate::GetCurrent()));
	if (res.IsEmpty())
	{
		return nullptr;
	}

	auto pThis = Unwrap(obj);
	return pThis;
}

///////////////////////////////////////////////////////////////////////////////
MatchTid::MatchTid(const v8::Handle<v8::Object>& handle, int tid)
	: _Tid(tid)
{
	Wrap(handle);
}

///////////////////////////////////////////////////////////////////////////////
void MatchTid::jsMake(const FunctionCallbackInfo<Value> &args)
{
	if(args.Length() != 3)
	{
		ThrowTypeError(s_InvalidArgs);
	}

	args.GetReturnValue().Set(GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(1, &args[0]));
}

///////////////////////////////////////////////////////////////////////////////
void MatchTid::jsNew(const FunctionCallbackInfo<Value> &args)
{
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		ThrowTypeError(s_InvalidArgs);
	}

	MatchTid *tid = new MatchTid(args.This(), args[0]->Int32Value());

	args.GetReturnValue().Set(args.This());
}

} // 
