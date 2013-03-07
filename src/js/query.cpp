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
#include "funcwrap.h"
#include "apphost.h"
#include "bitset.h"
#include "trace.h"
#include "error.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> Query::_Template;

void Query::Init()
{
	_Template = Persistent<FunctionTemplate>(FunctionTemplate::New(jsNew));
	_Template->PrototypeTemplate()->Set("and", FunctionTemplate::New(&jsAnd));
	_Template->PrototypeTemplate()->Set("or", FunctionTemplate::New(&jsOr));
	_Template->PrototypeTemplate()->Set("where", FunctionTemplate::New(&jsWhere));
	_Template->PrototypeTemplate()->Set("map", FunctionTemplate::New(&jsMap));
	_Template->PrototypeTemplate()->Set("pair", FunctionTemplate::New(&jsPair));
	_Template->PrototypeTemplate()->Set("find", FunctionTemplate::New(&jsFind));
	_Template->PrototypeTemplate()->Set("count", FunctionTemplate::New(&jsCount));
	_Template->SetClassName(String::New("query"));

	_Template->InstanceTemplate()->SetInternalFieldCount(1);
}

Query::Query(const v8::Handle<v8::Object>& handle, const Arguments &args)
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
		auto traceColl = coll->FindInstanceInPrototypeChain(Trace::GetTemplate());
		if(traceColl.IsEmpty())
		{
			throw V8RuntimeException("Only trace collection is supported");
		}

		// store source
		_Source = Persistent<Object>::New(traceColl);
		_Op.reset(new QueryOpTraceSource());
	}
	else
	{
		OP op = (OP)args[0]->Int32Value();

		Query * pLeft = UnwrapThis<Query>(args[1].As<Object>());
		_Source = pLeft->_Source;

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
				throw V8RuntimeException("Unsupported parameter");
			}
			_Op.reset(new QueryOpMap(pLeft->Op(), args[2].As<Function>())); 
		}
		else // PAIR
		{
			_Op.reset(new QueryOpPair(pLeft->Op()));
		}
	}
}

Handle<Value> Query::jsNew(const Arguments &args)
{
	return TryCatchCpp([&args] 
	{
		Query *query;
		query = new Query(args.This(), args);
		return args.This();
	});
}

Handle<Value> Query::jsWhere(const Arguments &args)
{
	return BuildWhereExpr(args, WHERE);
}

Handle<Value> Query::jsMap(const Arguments &args)
{
	return BuildWhereExpr(args, MAP);
}

Handle<Value> Query::jsPair(const Arguments &args)
{
	return TryCatchCpp([&args] 
	{
		Local<Value> initArgs[2];
		initArgs[0] = Integer::New(PAIR);
		initArgs[1] = args.This();

		return Query::GetTemplate()->GetFunction()->NewInstance(2, initArgs);
	});
}

Handle<Value> Query::jsFind(const v8::Arguments &args)
{
	return TryCatchCpp([&args]
	{
		auto queryJs = BuildWhereExpr(args, WHERE);
		Query * pThis = UnwrapThis<Query>(queryJs.As<Object>());

		auto it = pThis->Op()->CreateIterator();
		if(it->IsEnd())
		{
			throw V8RuntimeException("Cannot find item");
		}

		return it->JsValue();
	});
}

Handle<Value> Query::jsCount(const Arguments &args)
{
	return TryCatchCpp([&args] 
	{
		Query * pThis = UnwrapThis<Query>(args.This());
		size_t count = 0;

		for(auto it = pThis->Op()->CreateIterator(); !it->IsEnd(); it->Next())
		{
			count++;
		}

		return Integer::New(count);
	});
}

Handle<Value> Query::jsOr(const Arguments &args)
{
	return BuildWhereExpr(args, OR);
}

Handle<Value> Query::jsAnd(const Arguments &args)
{
	return BuildWhereExpr(args, AND);
}

Handle<Value> Query::BuildWhereExpr(const Arguments &args, Query::OP op)
{
	return TryCatchCpp([&args, op]() -> Handle<Value>
	{
		auto pThis = UnwrapThis<Query>(args.This());
		if(args.Length() != 1)
		{
			return ThrowException(Exception::TypeError(String::New("invalid number of arguments. where(expr)")));
		}

		// pass op as first parameter, this as left and current value as right
		Local<Value> initArgs[3];
		initArgs[0] = Integer::New(op);
		initArgs[1] = args.This();
		initArgs[2] = args[0];
		auto jsQuery = Query::GetTemplate()->GetFunction()->NewInstance(3, initArgs);

		return jsQuery;
	});
}

Query * Query::TryGetQuery(const Persistent<Object> & obj)
{
	auto res = obj->FindInstanceInPrototypeChain(_Template);
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
