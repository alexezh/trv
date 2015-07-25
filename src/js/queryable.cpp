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
#include "queryable.h"
#include "query.h"
#include "apphost.h"
#include "bitset.h"
#include "trace.h"
#include "error.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> Queryable::_Template;

void Queryable::Init(v8::Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso));
	tmpl->SetClassName(String::NewFromUtf8(iso, "Queryable"));
	auto protoTempl = tmpl->PrototypeTemplate();

	protoTempl->Set(String::NewFromUtf8(iso, "where"), FunctionTemplate::New(iso, &jsWhere));
	protoTempl->Set(String::NewFromUtf8(iso, "select"), FunctionTemplate::New(iso, &jsSelect));
	protoTempl->Set(String::NewFromUtf8(iso, "pair"), FunctionTemplate::New(iso, &jsPair));
	protoTempl->Set(String::NewFromUtf8(iso, "find"), FunctionTemplate::New(iso, &jsFind));
	protoTempl->Set(String::NewFromUtf8(iso, "count"), FunctionTemplate::New(iso, &jsCount));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

Queryable * Queryable::TryGetQueryable(const v8::Local<v8::Object> & obj)
{
	auto res = obj->FindInstanceInPrototypeChain(GetTemplate(Isolate::GetCurrent()));
	if (res.IsEmpty())
	{
		return nullptr;
	}

	auto pThis = UnwrapThis<Query>(obj);
	return pThis;
}

void Queryable::jsWhere(const FunctionCallbackInfo<Value> &args)
{
	args.GetReturnValue().Set(BuildWhereExpr(args, Queryable::WHERE));
}

void Queryable::jsSelect(const FunctionCallbackInfo<Value> &args)
{
	args.GetReturnValue().Set(BuildWhereExpr(args, Queryable::MAP));
}

void Queryable::jsPair(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]
	{
		Local<Value> initArgs[2];
		initArgs[0] = Integer::New(Isolate::GetCurrent(), Queryable::PAIR);
		initArgs[1] = args.This();

		return Query::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(2, initArgs);
	});
}

void Queryable::jsFind(const v8::FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]
	{
		auto queryJs = BuildWhereExpr(args, Queryable::WHERE);
		Query * pThis = UnwrapThis<Query>(queryJs.As<Object>());
		LOG("@%p", pThis);
		auto it = pThis->Op()->CreateIterator();
		if (it->IsEnd())
		{
			throw V8RuntimeException("Cannot find item");
		}

		return it->JsValue();
	});
}

void Queryable::jsCount(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]() -> Local < Value >
	{
		auto * pThis = Unwrap(args.This());
		return Integer::New(Isolate::GetCurrent(), pThis->ComputeCount());
	});
}

Handle<Value> Queryable::BuildWhereExpr(const FunctionCallbackInfo<Value> &args, Queryable::OP op)
{
	if (args.Length() != 1)
	{
		ThrowTypeError("invalid number of FunctionCallbackInfo<Value>. where(expr)");
	}

	// pass op as first parameter, this as left and current value as right
	Local<Value> initArgs[3];
	initArgs[0] = Integer::New(Isolate::GetCurrent(), op);
	initArgs[1] = args.This();
	initArgs[2] = args[0];
	auto jsQuery = Query::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(3, initArgs);

	return jsQuery;
}

}
