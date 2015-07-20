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
#include "tid.h"

namespace Js {

std::shared_ptr<QueryOp> QueryOpWhere::Combine(EXPRTYPE type, const std::shared_ptr<Expr> & rightExpr)
{
	std::shared_ptr<Expr> expr;
	switch (type)
	{
	case OR: expr.reset(new ExprOr(_Expr, rightExpr)); break;
	case AND: expr.reset(new ExprAnd(_Expr, rightExpr)); break;
	default: assert(false);
	}

	// create where query with the same source and new expr
	return std::make_shared<QueryOpWhere>(_Source, QueryOpWhere::ALLMATCH, expr);
}

std::shared_ptr<QueryOpWhere::Expr> QueryOpWhere::FromJs(v8::Handle<v8::Value> & val)
{
	if (val->IsString())
	{
		return std::make_shared<MatchMsg>(*v8::String::Utf8Value(val));
	}
	else if (val->IsFunction())
	{
		return std::make_shared<MatchJs>(val.As<v8::Function>());
	}
	else if (val->IsObject())
	{
		auto match = Js::MatchTid::TryGetMatchTid(val.As<v8::Object>());
		if (match != nullptr)
		{
			return std::make_shared<MatchTid>(match->Tid());
		}
	}
	
	throw V8RuntimeException("Unsupported parameter");
}

}
