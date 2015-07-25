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
#include "tracecollection.h"
#include "querytracesource.h"
#include "apphost.h"
#include "bitset.h"
#include "stringreader.h"
#include "make_unique.h"
#include "tracelineparser.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> TraceCollection::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void TraceCollection::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->Inherit(Queryable::GetTemplate(iso));
	tmpl->SetClassName(String::NewFromUtf8(iso,"TraceCollection"));

	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->Set(String::NewFromUtf8(iso, "addline"), FunctionTemplate::New(iso, jsAddLine));
	tmpl_proto->Set(String::NewFromUtf8(iso, "removeline"), FunctionTemplate::New(iso, jsRemoveLine));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

void TraceCollection::InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	target->Set(String::NewFromUtf8(iso, "TraceCollection"), TraceCollection::GetTemplate(iso)->GetFunction());
}

void TraceCollection::jsNew(const v8::FunctionCallbackInfo<Value> &args)
{
	TraceCollection *tr = nullptr;
	if (args.Length() == 0)
	{
		tr = new TraceCollection(args.This(), GetCurrentHost()->GetLineCount());
	}
	else if (args.Length() == 2)
	{
		DWORD dwStart, dwEnd;
		if (!(ValueToLineIndex(args[0], dwStart) && ValueToLineIndex(args[1], dwEnd)))
		{
			ThrowTypeError("Invalid parameter. Int or Line expected");
		}
		tr = new TraceCollection(args.This(), dwStart, dwEnd);
	}
	else
	{
		ThrowTypeError("Invalid number of parameters");
	}


	args.GetReturnValue().Set(args.This());
}

void TraceCollection::jsAddLine(const v8::FunctionCallbackInfo<Value> &args)
{
	DWORD dwLine;
	if (!(ValueToLineIndex(args[0], dwLine)))
	{
		ThrowTypeError("Invalid parameter. Int or Line expected");
	}

	TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
	pThis->AddLine(dwLine);
}

void TraceCollection::jsRemoveLine(const v8::FunctionCallbackInfo<Value> &args)
{
	DWORD dwLine;
	if (!(ValueToLineIndex(args[0], dwLine)))
	{
		ThrowTypeError("Invalid parameter. Int or Line expected");
	}

	TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
	pThis->RemoveLine(dwLine);
}

void TraceCollection::AddLine(DWORD dwLine)
{
	auto oldLines(std::move(_Lines));
	_Lines = std::make_shared<CBitSet>(oldLines->Clone());
	_Lines->SetBit(dwLine);
	
	if (_Listener)
		_Listener(this, *oldLines, *_Lines);
}

void TraceCollection::RemoveLine(DWORD dwLine)
{
	auto oldLines(std::move(_Lines));
	_Lines = std::make_shared<CBitSet>(oldLines->Clone());
	_Lines->ResetBit(dwLine);
	if (_Listener)
		_Listener(this, *oldLines, *_Lines);
}

bool TraceCollection::ValueToLineIndex(Local<Value>& v, DWORD& idx)
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

TraceCollection::TraceCollection(const v8::Handle<v8::Object>& handle, DWORD lineCount)
	: Queryable(handle)
	, _Lines(std::make_shared<CBitSet>())
{
	_Lines->Init(lineCount);
	_Op = std::make_shared<QueryOpTraceCollection>(_Lines);
}

TraceCollection::TraceCollection(const v8::Handle<v8::Object>& handle, DWORD start, DWORD end)
	: Queryable(handle)
{
	if(start >= end)
	{
		throw V8RuntimeException("invalid parameter");
	}

	// for now ignore start / end
	_Lines->Init(end);
	for(DWORD i = start; i <= end; i++)
	{
		_Lines->SetBit(i);
	}
	_Op = std::make_shared<QueryOpTraceCollection>(_Lines);
}

TraceCollection* TraceCollection::TryGetCollection(const Local<Object> & obj)
{
	auto res = obj->FindInstanceInPrototypeChain(GetTemplate(Isolate::GetCurrent()));
	if (res.IsEmpty())
	{
		return nullptr;
	}

	auto pThis = Unwrap(obj);
	return pThis;
}

void TraceCollection::SetChangeListener(const ChangeListener& listner)
{
	// do not allow multiple listners
	assert(!(_Listener && listner));
	_Listener = listner;
}

size_t TraceCollection::ComputeCount()
{
	return _Lines->GetSetBitCount();
}

} // Js
