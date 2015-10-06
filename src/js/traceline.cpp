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
#include "traceline.h"
#include "apphost.h"
#include "bitset.h"
#include "stringreader.h"
#include "make_unique.h"
#include "tracelineparser.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> TraceLine::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void TraceLine::Init(Isolate* iso)
{
	// TraceLineParser::Test();

	auto tmpl(FunctionTemplate::New(iso, jsNew));
	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "index"), jsIndexGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "time"), jsTimeGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "thread"), jsThreadGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "user1"), jsUser1Getter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "user2"), jsUser2Getter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "user3"), jsUser3Getter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "user4"), jsUser4Getter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "msg"), jsMsgGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "content"), jsContentGetter);
	tmpl_proto->Set(String::NewFromUtf8(iso, "print"), FunctionTemplate::New(iso, jsPrint));

	tmpl->SetClassName(String::NewFromUtf8(iso, "TraceLine"));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

void TraceLine::InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	target->Set(String::NewFromUtf8(iso, "TraceLine"), TraceLine::GetTemplate(iso)->GetFunction());
}

///////////////////////////////////////////////////////////////////////////////
TraceLine::TraceLine(const v8::Handle<v8::Object>& handle, int lineNum)
	: _Line(GetCurrentHost()->GetLine(lineNum))
{
	Wrap(handle);
}

///////////////////////////////////////////////////////////////////////////////
void TraceLine::jsNew(const FunctionCallbackInfo<Value> &args)
{
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		ThrowTypeError("Invalid number of FunctionCallbackInfo<Value>. Format traceline(int)");
	}

	TraceLine *line = new TraceLine(args.This(), args[0]->Int32Value());

	args.GetReturnValue().Set(args.This());
}
void TraceLine::jsPrint(const FunctionCallbackInfo<Value> &args)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(args.This());
	std::stringstream ss;
	ss << "Msg:";
	ss.write(pThis->_Line.Msg.psz, pThis->_Line.Msg.cch);
	ss << "\r\n";
	GetCurrentHost()->OutputLine(ss.str().c_str());
}

void TraceLine::jsIndexGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Line.Index));
}

void TraceLine::jsTimeGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.Time.psz, String::kNormalString, pThis->_Line.Time.cch));
}

void TraceLine::jsThreadGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Line.Tid));
}

void TraceLine::jsUser1Getter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	if (pThis->_Line.User[0].psz != nullptr)
	{
		info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.User[0].psz, String::kNormalString, pThis->_Line.User[0].cch));
	}
}

void TraceLine::jsUser2Getter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	if (pThis->_Line.User[1].psz != nullptr)
	{
		info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.User[1].psz, String::kNormalString, pThis->_Line.User[1].cch));
	}
}

void TraceLine::jsUser3Getter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	if (pThis->_Line.User[2].psz != nullptr)
	{
		info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.User[2].psz, String::kNormalString, pThis->_Line.User[2].cch));
	}
}

void TraceLine::jsUser4Getter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	if (pThis->_Line.User[3].psz != nullptr)
	{
		info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.User[3].psz, String::kNormalString, pThis->_Line.User[3].cch));
	}
}

void TraceLine::jsMsgGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.Msg.psz, String::kNormalString, pThis->_Line.Msg.cch));
}

void TraceLine::jsContentGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.Content.psz, String::kNormalString, pThis->_Line.Content.cch));
}


} // Js
