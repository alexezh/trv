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
	return std::make_shared<QueryOpWhere>(_Left, QueryOpWhere::ALLMATCH, expr);
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
		std::vector<std::shared_ptr<Expr>> exprs;

		// if expression is object, treat it as json
		auto obj = val.As<v8::Object>();
		auto maybeTid = GetObjectField(obj, "tid");
		if (!maybeTid.IsEmpty())
		{
			auto tid = maybeTid.ToLocalChecked();				
			if (tid->IsInt32())
			{
				exprs.push_back(std::make_shared<MatchTid>(tid->ToInteger()->Int32Value()));
			}
		}

		for (size_t i = 0; i < LineInfoDesc::MaxUser; i++)
		{
			std::string userName = "user";
			char idxA[32];
			_itoa(i+1, idxA, 10);
			userName += idxA;
			auto maybeUser = GetObjectField(obj, userName.c_str());

			if (!maybeUser.IsEmpty())
			{
				auto user = maybeUser.ToLocalChecked();
				if (user->IsUndefined())
					continue;

				if (user->IsString() || user->IsStringObject())
				{
					v8::String::Utf8Value str(user);
					exprs.push_back(std::make_shared<MatchUser>(*str, i));
				}
				else if (user->IsArray())
				{
					std::vector<std::string> values;
					auto userA = user.As<v8::Array>();
					for (int i = 0; i < userA->Length(); i++)
					{
						if (!userA->Get(i)->IsString())
							ThrowSyntaxError("input must be array of strings");

						v8::String::Utf8Value str(userA->Get(i)->ToString());
						values.push_back(std::string(*str, str.length()));
					}

					exprs.push_back(std::make_shared<MatchUser>(std::move(values), i));
				}
				else
				{
					ThrowSyntaxError("input must be string or array of strings");
				}
			}
		}

		if(exprs.size() > 1)
			ThrowSyntaxError("multiple expression not yet supported");

		if(exprs.size() == 0)
			ThrowSyntaxError("unrecognized or empty expression");

		return exprs[0];
	}
	
	throw V8RuntimeException("Unsupported parameter");
}

}
