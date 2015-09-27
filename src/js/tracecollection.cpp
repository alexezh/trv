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
#include "trace.h"
#include "tracecollection.h"
#include "querytracesource.h"
#include "apphost.h"
#include "bitset.h"
#include "make_unique.h"
#include "traceline.h"
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
	tmpl_proto->Set(String::NewFromUtf8(iso, "addLine"), FunctionTemplate::New(iso, jsAddLine));
	tmpl_proto->Set(String::NewFromUtf8(iso, "removeLine"), FunctionTemplate::New(iso, jsRemoveLine));
	tmpl_proto->Set(String::NewFromUtf8(iso, "intersect"), FunctionTemplate::New(iso, jsIntersect));
	tmpl_proto->Set(String::NewFromUtf8(iso, "combine"), FunctionTemplate::New(iso, jsCombine));
	tmpl_proto->Set(String::NewFromUtf8(iso, "getLine"), FunctionTemplate::New(iso, jsGetLine));

	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "count"), jsCountGetter);

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

void TraceCollection::InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	target->Set(String::NewFromUtf8(iso, "TraceCollection"), TraceCollection::GetTemplate(iso)->GetFunction());
}

static void ThrowIncorrectNewSyntax()
{
	ThrowTypeError("use TraceCollection(source, [start, end])");
}

void TraceCollection::jsNew(const v8::FunctionCallbackInfo<Value> &args)
{
	TraceCollection *tr = nullptr;
	if (args.Length() == 0)
	{
		tr = new TraceCollection(args.This(), GetCurrentHost()->GetFileTraceSource(), GetCurrentHost()->GetFileTraceSource()->GetLineCount());
	}
	else if (args.Length() == 2)
	{
		DWORD dwStart, dwEnd;
		if (!(ValueToLineIndex(args[0], dwStart) && ValueToLineIndex(args[1], dwEnd)))
		{
			ThrowTypeError("Invalid parameter. Int or Line expected");
		}
		tr = new TraceCollection(args.This(), GetCurrentHost()->GetFileTraceSource(), dwStart, dwEnd);
	}
	else
	{
		ThrowTypeError("Invalid number of parameters");
	}


	args.GetReturnValue().Set(args.This());
}

void TraceCollection::jsAddLine(const v8::FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		DWORD dwLine;
		if (!(ValueToLineIndex(args[0], dwLine)))
		{
			ThrowTypeError("Invalid parameter. Int or Line expected");
		}

		TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
		pThis->AddLine(dwLine);
		return Local<Value>();
	});
}

void TraceCollection::jsRemoveLine(const v8::FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		DWORD dwLine;
		if (!(ValueToLineIndex(args[0], dwLine)))
		{
			ThrowTypeError("Invalid parameter. Int or Line expected");
		}

		TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
		pThis->RemoveLine(dwLine);
		return Local<Value>();
	});
}

void TraceCollection::jsCountGetter(Local<String> property, const PropertyCallbackInfo<Value>& info)
{
	TraceCollection * pThis = UnwrapThis<TraceCollection>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Lines->GetSetBitCount()));
}

void TraceCollection::jsGetLine(const v8::FunctionCallbackInfo<v8::Value> &args)
{	
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		if (args.Length() != 1 || !args[0]->IsInt32())
			ThrowTypeError("Invalid parameter. Use coll.getLine(index)");

		// we have index to selected bit; need to find actual line number
		TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
		auto idx = args[0]->Int32Value();
		auto idxLine = pThis->_Lines->FindNSetBit(idx);
		if (idxLine == -1)
			return Local<Value>();

		auto idxLineJs = v8::Integer::New(v8::Isolate::GetCurrent(), idxLine).As<Value>();
		auto line = TraceLine::GetTemplate(v8::Isolate::GetCurrent())->GetFunction()->NewInstance(1, &idxLineJs);
		return line;
	});
}

void TraceCollection::jsIntersect(const v8::FunctionCallbackInfo<v8::Value> &args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
		if (args.Length() != 1 || !args[0]->IsObject())
			ThrowTypeError("Invalid parameter. Use coll.intersect(collection)");

		auto* other = TraceCollection::TryGetCollection(args[0].As<Object>());
		if (other == nullptr)
		{
			ThrowTypeError("Invalid parameter. Use coll.intersect(collection)");
		}

		auto res = pThis->_Lines->Clone();
		res.And(*other->GetLines());

		auto collJs(TraceCollection::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance());
		auto coll = TraceCollection::Unwrap(collJs);
		coll->SetLines(std::move(res));

		return collJs;
	});
}

void TraceCollection::jsCombine(const v8::FunctionCallbackInfo<v8::Value> &args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		TraceCollection * pThis = UnwrapThis<TraceCollection>(args.This());
		if (args.Length() != 1 || !args[0]->IsObject())
		{
			ThrowTypeError("Invalid parameter. Use coll.combine(collection)");
		}

		auto* other = TraceCollection::TryGetCollection(args[0].As<Object>());
		if (other == nullptr)
		{
			ThrowTypeError("Invalid parameter. Use coll.combine(collection)");
		}

		auto res = pThis->_Lines->Clone();
		res.Or(*other->GetLines());

		auto collJs(TraceCollection::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance());
		auto coll = TraceCollection::Unwrap(collJs);
		coll->SetLines(std::move(res));

		return collJs;
	});
}

void TraceCollection::AddLines(const CBitSet& lines)
{
	_Lines->Or(lines);
}

void TraceCollection::AddLine(DWORD dwLine)
{
	auto oldLines(std::move(_Lines));
	_Lines = std::make_shared<CBitSet>(oldLines->Clone());
	_Lines->SetBit(dwLine);
	
	if (_Listener)
		_Listener(this, oldLines, _Lines);
}

void TraceCollection::RemoveLine(DWORD dwLine)
{
	auto oldLines(std::move(_Lines));
	_Lines = std::make_shared<CBitSet>(oldLines->Clone());
	_Lines->ResetBit(dwLine);
	if (_Listener)
		_Listener(this, oldLines, _Lines);
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

TraceCollection::TraceCollection(const v8::Handle<v8::Object>& handle, const std::shared_ptr<CTraceSource>& src, DWORD lineCount)
	: Queryable(handle)
	, _Source(src)
	, _Lines(std::make_shared<CBitSet>())
{
	_Lines->Resize(lineCount);
	_Op = std::make_shared<QueryOpTraceCollection>(_Source, _Lines);
}

TraceCollection::TraceCollection(const v8::Handle<v8::Object>& handle, const std::shared_ptr<CTraceSource>& src, DWORD start, DWORD end)
	: Queryable(handle)
	, _Source(src)
{
	if(start >= end)
	{
		throw V8RuntimeException("invalid parameter");
	}

	// for now ignore start / end
	_Lines->Resize(end);
	for(DWORD i = start; i <= end; i++)
	{
		_Lines->SetBit(i);
	}
	_Op = std::make_shared<QueryOpTraceCollection>(_Source, _Lines);
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

const std::shared_ptr<CTraceSource>& TraceCollection::Source()
{
	return _Source;
}

size_t TraceCollection::ComputeCount()
{
	return _Lines->GetSetBitCount();
}

} // Js
