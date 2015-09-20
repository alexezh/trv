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
#include "query.h"
#include "querymap.h"
#include "querywhere.h"
#include "querypair.h"
#include "querytracesource.h"
#include "apphost.h"
#include "bitset.h"
#include "trace.h"
#include "traceline.h"
#include "tracecollection.h"
#include "error.h"

using namespace v8;

namespace Js {

///////////////////////////////////////////////////////////////////////////////
//
void QueryIteratorHelper::SelectLinesFromIteratorValue(QueryIterator* it, CBitSet& set)
{
	HandleScope scope(Isolate::GetCurrent());
	if (it->IsNative())
	{
		set.SetBit(it->NativeValue().Index);
	}
	else
	{
		auto res = it->JsValue();

		// if result is line, return it
		// if result is line set, return it
		// otherwise fail
		if (res->IsInt32())
		{
			set.SetBit(res->ToInteger()->Int32Value());
			return;
		}
		else if (res->IsObject())
		{
			Handle<Object> objRes = res.As<Object>();
			auto lineJs = objRes->FindInstanceInPrototypeChain(TraceLine::GetTemplate(Isolate::GetCurrent()));
			if (!lineJs.IsEmpty())
			{
				TraceLine* line = TraceLine::Unwrap(lineJs);
				set.SetBit(line->Line().Index);
				return;
			}

			auto rangeJs = objRes->FindInstanceInPrototypeChain(TraceCollection::GetTemplate(Isolate::GetCurrent()));
			if (!rangeJs.IsEmpty())
			{
				TraceCollection * range = UnwrapThis<TraceCollection>(rangeJs);
				set.Or(*(range->GetLines()));
			}
		}

		throw V8RuntimeException("Unsupported type for iterator value");
	}
}

///////////////////////////////////////////////////////////////////////////////
//
UniquePersistent<FunctionTemplate> Query::_Template;

void Query::Init(v8::Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->SetClassName(String::NewFromUtf8(iso, "Query"));
	tmpl->Inherit(Queryable::GetTemplate(iso));

	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "asCollection"), FunctionTemplate::New(iso, &jsAsCollection));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

Query::Query(const v8::Handle<v8::Object>& handle, const FunctionCallbackInfo<Value> &args)
	: Queryable(handle)
{
	LOG("@%p", this);
	assert(args.Length() == 3);
	Queryable::OP op = (Queryable::OP)args[0]->Int32Value();

	Queryable * pLeft = Queryable::TryGetQueryable(args[1].As<Object>());
	std::shared_ptr<QueryOp> leftOp;
	if (pLeft == nullptr)
	{
		ThrowSyntaxError("Left object should be Queryable");
	}

	leftOp = pLeft->Op();
	_Source = pLeft->Source();

	if (op == Queryable::WHERE)
	{
		std::shared_ptr<QueryOp> rightOp;
		_Op = std::make_shared<QueryOpWhere>(leftOp, QueryOpWhere::ALLMATCH, QueryOpWhere::FromJs(args[2])); 
	}
	else if (op == Queryable::MAP)
	{
		if(!args[2]->IsFunction())
		{
			ThrowSyntaxError("Unsupported parameter");
		}
		_Op.reset(new QueryOpMap(leftOp, args[2].As<Function>())); 
	}
	else // PAIR
	{
		_Op.reset(new QueryOpPair(leftOp));
	}
}

void Query::jsNew(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]
	{
		Query *query;
		query = new Query(args.This(), args);
		return args.This();
	});
}

void Query::jsAsCollection(const v8::FunctionCallbackInfo<v8::Value> &args)
{
	auto * pThis = Unwrap(args.This());
	args.GetReturnValue().Set(pThis->GetCollection());
}

size_t Query::ComputeCount()
{
	size_t count = 0;
	LOG("@%p", this);

	for (auto it = Op()->CreateIterator(); !it->IsEnd(); it->Next())
	{
		count++;
	}

	return count;
}

Query* Query::TryGetQuery(const Local<Object> & obj)
{
	auto res = obj->FindInstanceInPrototypeChain(GetTemplate(Isolate::GetCurrent()));
	if(res.IsEmpty())
	{
		return nullptr;
	}

	auto pThis = UnwrapThis<Query>(obj);
	return pThis;
}

std::string Query::MakeDescription()
{
	return _Op->MakeDescription();
}

const std::shared_ptr<CTraceSource>& Query::Source()
{
	return _Source;
}

v8::Local<v8::Object> Query::GetCollection()
{
	std::string startMsg;
	startMsg = std::string("Start query: ") + MakeDescription() + "\r\n";

	GetCurrentHost()->OutputLine(startMsg.c_str());

	auto collJs(TraceCollection::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance());
	auto coll = TraceCollection::Unwrap(collJs);

	DWORD dwStart = GetTickCount();
	{
		// populate set from query
		// TODO: check if iterator is Js; inverse loop to Js
		for (auto it = Op()->CreateIterator(); !it->IsEnd(); it->Next())
		{
			QueryIteratorHelper::SelectLinesFromIteratorValue(it.get(), *coll->GetLines());
		}
	}
	DWORD dwEnd = GetTickCount();

	// echo to output
	std::stringstream ss;
	ss << "Query execution time " << (dwEnd - dwStart) << "ms\r\n";
	ss << " match=" << coll->GetLines()->GetSetBitCount() << "\r\n";
	GetCurrentHost()->OutputLine(ss.str().c_str());

	return collJs;
}

} // Js
