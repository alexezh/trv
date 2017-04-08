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
#pragma once

#include "queryop.h"

namespace Js {

class QueryOpWhere : public QueryOp
{
private:
	// WHERE manages a tree of expressions
	class Expr
	{
	public:
		// true if expression supports native evaluation
		virtual bool IsNative()
		{
			return true;
		}

		// evaluate expression on string
		virtual bool NativeEval(const LineInfo & line) = 0;

		// evaluate expression on object
		virtual bool JsEval(v8::Handle<v8::Value> & line)
		{
			assert(false);
			return false;
		}

		virtual std::string MakeDescription() = 0;
	};

	class MatchMsg : public Expr
	{
	public:

		MatchMsg(const char * pszExpr)
		{
			_Expr = pszExpr;
			_Bc.BuildBc(pszExpr);
		}

		bool IsNative() override
		{
			return true;
		}

		// evaluate expression on string
		bool NativeEval(const LineInfo & line) override
		{
			return _Bc.Search(line.Msg.psz, line.Msg.cch) != nullptr;
		}

		std::string MakeDescription() override
		{
			return std::string("\"") + _Expr + "\"";
		}
	private:
		std::string _Expr;
		CStrStr _Bc;
	};

	class MatchUser : public Expr
	{
	public:

		MatchUser(const char * pszUser, size_t userIdx)
		{
			_User.push_back(pszUser);
			_UserIdx = userIdx;
		}

		MatchUser(std::vector<std::string>&& val, size_t userIdx)
		{
			_User = std::move(val);
			_UserIdx = userIdx;
		}

		bool IsNative() override
		{
			return true;
		}

		// evaluate expression on string
		bool NativeEval(const LineInfo & line) override
		{
			if (line.User[_UserIdx].psz == nullptr)
				return false;

			for (auto& v : _User)
			{
				if (_strnicmp(v.c_str(), line.User[_UserIdx].psz, line.User[_UserIdx].cch) == 0)
					return true;
			}

			return false;
		}

		std::string MakeDescription() override
		{
			std::string desc("\"");
			for (auto& v : _User)
			{
				if (desc.length() > 1)
					desc += " || ";

				desc += v;
			}

			desc += "\"";
			return desc;
		}

	private:
		std::vector<std::string> _User;
		size_t _UserIdx;
	};

	class MatchTid : public Expr
	{
	public:
		MatchTid(int tid)
			: _Tid(tid)
		{
		}

		bool IsNative() override
		{
			return true;
		}

		// evaluate expression on string
		bool NativeEval(const LineInfo & line) override
		{
			return line.Tid == _Tid;
		}

		std::string MakeDescription() override
		{
			char tidA[32];
			return _itoa(_Tid, tidA, 10);
		}
	private:
		int _Tid;
	};

	class MatchJs : public Expr
	{
	public:
		MatchJs(const v8::Handle<v8::Function> & func)
		{
			_Func.Reset(v8::Isolate::GetCurrent(), func);
		}

		bool IsNative() override
		{
			return false;
		}

		// evaluate expression on string
		bool NativeEval(const LineInfo & line) override
		{
			assert(false);
			return false;
		}

		bool JsEval(v8::Local<v8::Value> & line) override
		{
			v8::HandleScope handleScope(v8::Isolate::GetCurrent());
			auto func(v8::Local<v8::Function>::New(v8::Isolate::GetCurrent(), _Func));

			v8::TryCatch try_catch;
			try_catch.SetVerbose(true);
			auto res = func->Call(v8::Isolate::GetCurrent()->GetCurrentContext()->Global(), 1, &line);
			if (try_catch.HasCaught())
			{
				GetCurrentHost()->ReportException(v8::Isolate::GetCurrent(), try_catch);
				throw V8RuntimeException("failed to run JS function");
			}

			if (res.IsEmpty())
				return false;

			bool v = res->BooleanValue();
			return v;
		}

		std::string MakeDescription() override
		{
			return "js";
		}
	private:
		v8::Persistent<v8::Function> _Func;
	};

	class ExprLogic2 : public Expr
	{
	public:
		ExprLogic2(const std::shared_ptr<Expr> & left, const std::shared_ptr<Expr> & right)
			: _Left(left)
			, _Right(right)
		{
		}
	protected:
		std::shared_ptr<Expr> _Right;
		std::shared_ptr<Expr> _Left;
	};

	class ExprOr : public ExprLogic2
	{
	public:
		ExprOr(const std::shared_ptr<Expr> & left, const std::shared_ptr<Expr> & right)
			: ExprLogic2(left, right)
		{
		}

		bool IsNative() override
		{
			return _Left->IsNative() && _Right->IsNative();
		}

		// evaluate expression on string
		bool NativeEval(const LineInfo & line) override
		{
			return _Left->NativeEval(line) || _Right->NativeEval(line);
		}

		std::string MakeDescription() override
		{
			return _Left->MakeDescription() + " or " + _Right->MakeDescription();
		}
	};

	class ExprAnd : public ExprLogic2
	{
	public:
		ExprAnd(const std::shared_ptr<Expr> & left, const std::shared_ptr<Expr> & right)
			: ExprLogic2(left, right)
		{
		}

		bool IsNative() override
		{
			return true;
		}

		// evaluate expression on string
		bool NativeEval(const LineInfo & line) override
		{
			return _Left->NativeEval(line) && _Right->NativeEval(line);
		}

		std::string MakeDescription() override
		{
			return _Left->MakeDescription() + " and " + _Right->MakeDescription();
		}
	};

public:
	enum EXPRTYPE
	{
		MATCH,
		OR,
		AND,
	};

	enum ITERTYPE
	{
		// returns all entries which match
		ALLMATCH,
		TAKEWHILE,
		SKIPWHILE,
	};

	class Iterator : public QueryIterator
	{
	public:
		Iterator(std::unique_ptr<QueryIterator>&& src, std::shared_ptr<Expr>& expr)
			: _Src(std::move(src))
			, _Expr(expr)
			, _Native(false)
		{
			for(;!_Src->IsEnd();_Src->Next())
			{
				if(CheckCurrent())
				{
					break;
				}
			}
		}

		bool Next() override
		{
			for(;;)
			{
				if(!_Src->Next())
				{
					return false;
				}

				if(CheckCurrent())
				{
					return true;
				}
			}
		
			return false;
		}
		bool IsEnd() override
		{
			return _Src->IsEnd();
		}
		bool IsNative() override
		{
			return _Src->IsNative() && _Expr->IsNative();
		}

		const LineInfo& NativeValue() override
		{
			return _Src->NativeValue();
		}
		v8::Handle<v8::Value> JsValue() override
		{
			return _Src->JsValue();
		}
	private:
		bool CheckCurrent()
		{
			// use native eval if we can
			if(_Expr->IsNative() && _Src->IsNative())
			{
				if(_Expr->NativeEval(_Src->NativeValue()))
				{
					return true;
				}
			}
			else
			{
				if(_Expr->JsEval(_Src->JsValue()))
				{
					return true;
				}
			}
			return false;
		}
		std::unique_ptr<QueryIterator> _Src;
		std::shared_ptr<Expr> _Expr;
		bool _Native;
	};

	QueryOpWhere(const std::shared_ptr<QueryOp>& src, ITERTYPE iterType, const std::shared_ptr<Expr>& expr)
		: _Left(src)
		, _Expr(expr)
		, _IterType(iterType)
	{
	}

	TYPE Type()
	{
		return QueryOp::WHERE;
	}

	std::string MakeDescription()
	{
		return std::string("where ") + _Expr->MakeDescription();
	}

	std::unique_ptr<QueryIterator> CreateIterator()
	{
		auto it = _Left->CreateIterator();
		return std::unique_ptr<QueryIterator>(new Iterator(std::move(it), _Expr));
	}

	std::shared_ptr<QueryOp> Combine(EXPRTYPE type, const std::shared_ptr<Expr> & rightExpr);

	static std::shared_ptr<Expr> FromJs(v8::Handle<v8::Value> & val);

protected:
	std::shared_ptr<QueryOp> _Left;
	std::shared_ptr<Expr> _Expr;
	ITERTYPE _IterType;
};

}
