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
#include "error.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> Query::_Template;

void Query::Init(v8::Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "and"), FunctionTemplate::New(iso, jsAnd));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "or"), FunctionTemplate::New(iso, &jsOr));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "where"), FunctionTemplate::New(iso, &jsWhere));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "map"), FunctionTemplate::New(iso, &jsMap));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "pair"), FunctionTemplate::New(iso, &jsPair));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "find"), FunctionTemplate::New(iso, &jsFind));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "count"), FunctionTemplate::New(iso, &jsCount));
	tmpl->SetClassName(String::NewFromUtf8(iso, "query"));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

Query::Query(const v8::Handle<v8::Object>& handle, const FunctionCallbackInfo<Value> &args)
{
	Wrap(handle);
	if(args.Length() == 1)
	{
		// parameter is a reference to collection
		if(!args[0]->IsObject())
		{
			throw V8RuntimeException("Parameter must be collection");
		}

		auto coll = args[0].As<Object>();

		// check if this is trace collection
		auto traceColl = coll->FindInstanceInPrototypeChain(Trace::GetTemplate(Isolate::GetCurrent()));
		if(traceColl.IsEmpty())
		{
			throw V8RuntimeException("Only trace collection is supported");
		}

		// store source
		_Source.Reset(Isolate::GetCurrent(), traceColl);
		_Op.reset(new QueryOpTraceSource());
	}
	else
	{
		OP op = (OP)args[0]->Int32Value();

		Query * pLeft = UnwrapThis<Query>(args[1].As<Object>());
		_Source.Reset(Isolate::GetCurrent(), pLeft->_Source);

		if(op == WHERE)
		{
			std::shared_ptr<QueryOp> rightOp;
			_Op = std::make_shared<QueryOpWhere>(pLeft->Op(), QueryOpWhere::ALLMATCH, QueryOpWhere::FromJs(args[2])); 
		}
		else if(op == OR || op == AND)
		{
			if(pLeft == nullptr || pLeft->Op()->Type() != QueryOp::WHERE)
			{
				throw V8RuntimeException("Invalid op sequence");
			}

			QueryOpWhere * pWhere = static_cast<QueryOpWhere*>(pLeft->Op().get());
			_Op = pWhere->Combine((op == OR) ? QueryOpWhere::OR : QueryOpWhere::AND, 
						QueryOpWhere::FromJs(args[2]));
		}
		else if(op == MAP)
		{
			if(!args[2]->IsFunction())
			{
				ThrowSyntaxError("Unsupported parameter");
			}
			_Op.reset(new QueryOpMap(pLeft->Op(), args[2].As<Function>())); 
		}
		else // PAIR
		{
			_Op.reset(new QueryOpPair(pLeft->Op()));
		}
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

void Query::jsWhere(const FunctionCallbackInfo<Value> &args)
{
	args.GetReturnValue().Set(BuildWhereExpr(args, WHERE));
}

void Query::jsMap(const FunctionCallbackInfo<Value> &args)
{
	args.GetReturnValue().Set(BuildWhereExpr(args, MAP));
}

void Query::jsPair(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args] 
	{
		Local<Value> initArgs[2];
		initArgs[0] = Integer::New(Isolate::GetCurrent(), PAIR);
		initArgs[1] = args.This();

		return Query::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance(2, initArgs);
	});
}

void Query::jsFind(const v8::FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]
	{
		auto queryJs = BuildWhereExpr(args, WHERE);
		Query * pThis = UnwrapThis<Query>(queryJs.As<Object>());
		LOG("@%p", pThis);
		auto it = pThis->Op()->CreateIterator();
		if(it->IsEnd())
		{
			throw V8RuntimeException("Cannot find item");
		}

		return it->JsValue();
	});
}

void Query::jsCount(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		Query * pThis = UnwrapThis<Query>(args.This());
		size_t count = 0;
		LOG("@%p", pThis);

		for(auto it = pThis->Op()->CreateIterator(); !it->IsEnd(); it->Next())
		{
			count++;
		}

		return Integer::New(Isolate::GetCurrent(), count);
	});
}

void Query::jsOr(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]() { return BuildWhereExpr(args, OR); });
}

void Query::jsAnd(const FunctionCallbackInfo<Value> &args)
{
	TryCatchCpp(args, [&args]() { return BuildWhereExpr(args, AND); });
}

Handle<Value> Query::BuildWhereExpr(const FunctionCallbackInfo<Value> &args, Query::OP op)
{
	auto pThis = UnwrapThis<Query>(args.This());
	LOG("@%p", pThis);
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

} // Js
