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
#include "funcwrap.h"
#include "apphost.h"
#include "bitset.h"
#include "stringreader.h"
#include "make_unique.h"
#include "tracelineparser.h"
#include "error.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> Trace::_Template;
Persistent<FunctionTemplate> TraceLine::_Template;
Persistent<FunctionTemplate> TraceRange::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void TraceLine::Init()
{
	// TraceLineParser::Test();

	_Template = Persistent<FunctionTemplate>(FunctionTemplate::New(jsNew));
	_Template->InstanceTemplate()->SetAccessor(String::New("time"), jsTimeGetter);
	_Template->InstanceTemplate()->SetAccessor(String::New("thread"), jsThreadGetter);
	_Template->InstanceTemplate()->SetAccessor(String::New("msg"), jsMsgGetter);
	_Template->InstanceTemplate()->Set(String::New("print"), FunctionTemplate::New(jsPrint));
	_Template->SetClassName(String::New("traceline"));

	_Template->InstanceTemplate()->SetInternalFieldCount(1);
}

///////////////////////////////////////////////////////////////////////////////
TraceLine::TraceLine(const v8::Handle<v8::Object>& handle, int lineNum)
	: _Line(GetCurrentHost()->GetLine(lineNum))
{
	Wrap(handle);
}

///////////////////////////////////////////////////////////////////////////////
Handle<Value> TraceLine::jsNew(const Arguments &args)
{
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		return ThrowException(Exception::TypeError(String::New(
			"Invalid number of arguments. Format traceline(int)")));
	}

	TraceLine *line = new TraceLine(args.This(), args[0]->Int32Value());

	return args.This();
}
Handle<Value> TraceLine::jsPrint(const Arguments &args)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(args.This());
	std::stringstream ss;
	ss << "Msg:";
	ss.write(pThis->_Line.Msg.psz, pThis->_Line.Msg.cch);
	ss << "\r\n";
	GetCurrentHost()->OutputLine(ss.str().c_str());

	return Undefined();
}

Handle<Value> TraceLine::jsTimeGetter(Local<String> property, 
											const AccessorInfo& info)
{
	return Undefined();
}
Handle<Value> TraceLine::jsThreadGetter(Local<String> property, 
											const AccessorInfo& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	return Integer::New(pThis->_Line.Tid);
}
Handle<Value> TraceLine::jsMsgGetter(Local<String> property, 
											const AccessorInfo& info)
{
	TraceLine * pThis = UnwrapThis<TraceLine>(info.This());
	return String::New(pThis->_Line.Msg.psz, pThis->_Line.Msg.cch);
}

///////////////////////////////////////////////////////////////////////////////
//
void TraceRange::Init()
{
	_Template = Persistent<FunctionTemplate>(FunctionTemplate::New(jsNew));
	_Template->SetClassName(String::New("tracerange"));

	_Template->InstanceTemplate()->SetInternalFieldCount(1);
}

Handle<Value> TraceRange::jsNew(const v8::Arguments &args)
{
	DWORD dwStart, dwEnd;
	if(!(ValueToLineIndex(args[0], dwStart) && ValueToLineIndex(args[1], dwEnd)))
	{
		return ThrowException(Exception::TypeError(String::New(
			"Invalid parameter. Int or Line expected")));
	}

	TraceRange *tr = new TraceRange(args.This(), dwStart, dwEnd);

	return args.This();
}

bool TraceRange::ValueToLineIndex(Local<Value>& v, DWORD& idx)
{
	if(v->IsInt32())
	{
		idx = v->Int32Value();
	}
	else 
	{
		Handle<Object> obj = v.As<Object>();
		auto lineJs = obj->FindInstanceInPrototypeChain(TraceLine::GetTemplate());
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
void Trace::Init()
{
	_Template = Persistent<FunctionTemplate>(FunctionTemplate::New(jsNew));
	_Template->InstanceTemplate()->SetAccessor(String::New("format"), jsFormatGetter, jsFormatSetter);
	_Template->InstanceTemplate()->SetAccessor(String::New("linecount"), jsLineCountGetter);
	_Template->InstanceTemplate()->Set(String::New("line"), FunctionTemplate::New(jsGetLine));
	_Template->InstanceTemplate()->Set(String::New("fromrange"), FunctionTemplate::New(jsFromRange));
	_Template->SetClassName(String::New("trace"));

	_Template->InstanceTemplate()->SetInternalFieldCount(1);

	TraceLine::Init();
	TraceRange::Init();
}

Handle<Value> Trace::jsNew(const Arguments &args)
{
	Trace *trace = new Trace();
	args.This()->SetInternalField(0, External::New(trace));

	return args.This();
}

Handle<Value> Trace::jsFormatGetter(Local<String> property, 
												const AccessorInfo& info)
{
	Trace * trace = UnwrapThis<Trace>(info.This());
	return String::New(trace->_Format.c_str());
}

void Trace::jsFormatSetter(Local<String> property, Local<Value> value,
								const AccessorInfo& info)
{
	Trace * pThis = UnwrapThis<Trace>(info.This());
	auto s = value->ToString();
	String::Utf8Value str(s);

	pThis->_Format = *str;
	GetCurrentHost()->SetTraceFormat(pThis->_Format.c_str());
}

Handle<Value> Trace::jsGetLine(const Arguments &args)
{
	auto pThis = UnwrapThis<Trace>(args.This());
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		return ThrowException(Exception::TypeError(String::New(
			"invalid number of arguments. Format getline(int)")));
	}

	auto jsLine = TraceLine::GetTemplate()->GetFunction()->NewInstance(1, &args[0]);

	return jsLine;
}

Handle<Value> Trace::jsFromRange(const v8::Arguments &args)
{
	if(args.Length() != 2)
	{
		return ThrowException(Exception::TypeError(String::New(
			"invalid number of arguments. Format fromrange(int, int)")));
	}

	// return trace collection object as a set of lines between 
	Local<Value> v[2];
	v[0] = args[0];
	v[1] = args[1];
	return TraceRange::GetTemplate()->GetFunction()->NewInstance(2, v);
}

Handle<Value> Trace::jsLineCountGetter(Local<String> property, 
											const AccessorInfo& info)
{
	Trace * trace = UnwrapThis<Trace>(info.This());
	return Integer::New(GetCurrentHost()->GetLineCount());
}

} // Js
