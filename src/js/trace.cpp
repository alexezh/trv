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
#include "trace.h"
#include "apphost.h"
#include "bitset.h"
#include "stringreader.h"
#include "make_unique.h"
#include "tracelineparser.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> Trace::_Template;
UniquePersistent<FunctionTemplate> TraceLine::_Template;
UniquePersistent<FunctionTemplate> TraceRange::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void TraceLine::Init(Isolate* iso)
{
	// TraceLineParser::Test();

	auto tmpl(FunctionTemplate::New(iso, jsNew));
	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "time"), jsTimeGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "thread"), jsThreadGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "msg"), jsMsgGetter);
	tmpl_proto->Set(String::NewFromUtf8(iso, "print"), FunctionTemplate::New(iso, jsPrint));
	tmpl->SetClassName(String::NewFromUtf8(iso, "traceline"));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
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

void TraceLine::jsTimeGetter(Local<String> property, 
											const PropertyCallbackInfo<v8::Value>& info)
{
}
void TraceLine::jsThreadGetter(Local<String> property, 
											const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Line.Tid));
}
void TraceLine::jsMsgGetter(Local<String> property, 
											const PropertyCallbackInfo<v8::Value>& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.Msg.psz, String::kNormalString, pThis->_Line.Msg.cch));
}

///////////////////////////////////////////////////////////////////////////////
//
void TraceRange::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->SetClassName(String::NewFromUtf8(iso,"tracerange"));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

void TraceRange::jsNew(const v8::FunctionCallbackInfo<Value> &args)
{
	DWORD dwStart, dwEnd;
	if(!(ValueToLineIndex(args[0], dwStart) && ValueToLineIndex(args[1], dwEnd)))
	{
		ThrowTypeError("Invalid parameter. Int or Line expected");
	}

	TraceRange *tr = new TraceRange(args.This(), dwStart, dwEnd);

	args.GetReturnValue().Set(args.This());
}

bool TraceRange::ValueToLineIndex(Local<Value>& v, DWORD& idx)
{
	if(v->IsInt32())
	{
		idx = v->Int32Value();
	}
	else 
	{
		Local<Object> obj = v.As<Object>();
		auto lineJs = obj->FindInstanceInPrototypeChain(TraceLine::GetTemplate(Isolate::GetCurrent()));
		if(lineJs.IsEmpty())
		{
			return false;
		}

		auto line = UnwrapThis<TraceLine>(lineJs);
		idx = line->Line().Index;
	}

	return true;
}

TraceRange::TraceRange(const v8::Handle<v8::Object>& handle, DWORD start, DWORD end)
{
	Wrap(handle);
	if(start >= end)
	{
		throw V8RuntimeException("invalid parameter");
	}

	_Lines.Init(end - start + 1);
	_Start = start;
	for(DWORD i = 0; i <= end-start; i++)
	{
		_Lines.SetBit(i);
	}
}

void TraceRange::GetLines(CBitSet& set)
{
	for(DWORD i = 0; i < _Lines.GetTotalBitCount(); i++)
	{
		set.SetBit(_Start+i);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
void Trace::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "format"), jsFormatGetter, jsFormatSetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "linecount"), jsLineCountGetter);
	tmpl_proto->Set(String::NewFromUtf8(iso, "line"), FunctionTemplate::New(iso, jsGetLine));
	tmpl_proto->Set(String::NewFromUtf8(iso, "fromrange"), FunctionTemplate::New(iso, jsFromRange));
	tmpl->SetClassName(String::NewFromUtf8(iso,"trace"));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	_Template = UniquePersistent<FunctionTemplate>(iso, tmpl);

	TraceLine::Init(iso);
	TraceRange::Init(iso);
}

void Trace::jsNew(const FunctionCallbackInfo<Value> &args)
{
	Trace *trace = new Trace(args.This());

	args.GetReturnValue().Set(args.This());
}

void Trace::jsFormatGetter(Local<String> property, 
												const PropertyCallbackInfo<v8::Value>& info)
{
	Trace * trace = UnwrapThis<Trace>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), trace->_Format.c_str()));
}

void Trace::jsFormatSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	Trace * pThis = UnwrapThis<Trace>(info.This());
	auto s = value->ToString();
	String::Utf8Value str(s);

	pThis->_Format.assign(*str, str.length());
	GetCurrentHost()->SetTraceFormat(pThis->_Format.c_str());
}

void Trace::jsGetLine(const FunctionCallbackInfo<Value> &args)
{
	auto pThis = UnwrapThis<Trace>(args.This());
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		ThrowTypeError("invalid number of FunctionCallbackInfo<Value>. Format getline(int)");
	}

	auto jsLine = TraceLine::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(1, &args[0]);

	args.GetReturnValue().Set(jsLine);
}

void Trace::jsFromRange(const v8::FunctionCallbackInfo<Value> &args)
{
	if(args.Length() != 2)
	{
		ThrowTypeError("invalid number of FunctionCallbackInfo<Value>. Format fromrange(int, int)");
	}

	// return trace collection object as a set of lines between 
	Local<Value> v[2];
	v[0] = args[0];
	v[1] = args[1];
	args.GetReturnValue().Set(TraceRange::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(2, v));
}

void Trace::jsLineCountGetter(Local<String> property, 
											const PropertyCallbackInfo<v8::Value>& info)
{
	Trace * trace = UnwrapThis<Trace>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), GetCurrentHost()->GetLineCount()));
}

} // Js
