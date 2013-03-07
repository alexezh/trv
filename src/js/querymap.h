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

class QueryOpMap : public QueryOp
{
public:
	class Iterator : public QueryIterator
	{
	public:
		Iterator(std::unique_ptr<QueryIterator> && src, v8::Persistent<v8::Function> func)
			: _Src(std::move(src))
			, _Func(func)
		{
		}

		bool Next()
		{
			return _Src->Next();
		}
		bool IsEnd()
		{
			return _Src->IsEnd();
		}
		bool IsNative()
		{
			// we always run js function so we never native
			return false;
		}

		LineInfo& NativeValue()
		{
			throw V8RuntimeException("invalid operation");
		}
		v8::Handle<v8::Value> JsValue()
		{
			// v8::HandleScope scope;
			auto v = _Src->JsValue();
			auto v1 = _Func->Call(v8::Context::GetCurrent()->Global(), 1, &v);
			if(v1.IsEmpty())
			{
				throw V8RuntimeException("Exception from V8 failed");
			}
			return v1;
		}
	private:
		v8::Persistent<v8::Function> _Func;
		std::unique_ptr<QueryIterator> _Src;
	};

	QueryOpMap(const std::shared_ptr<QueryOp> & src, const v8::Handle<v8::Function> & func)
		: _Source(src)
	{
		_Func = v8::Persistent<v8::Function>::New(func);
	}

	TYPE Type()
	{
		return QueryOp::MAP;
	}

	std::string MakeDescription()
	{
		return std::string("map");
	}

	std::unique_ptr<QueryIterator> CreateIterator()
	{
		auto parentIt = _Source->CreateIterator();
		std::unique_ptr<QueryIterator> it(new Iterator(std::move(parentIt), _Func));
		return std::move(it);
	}

private:
	std::shared_ptr<QueryOp> _Source;
	v8::Persistent<v8::Function> _Func;
};

}
