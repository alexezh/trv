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

class QueryOpPair : public QueryOp
{
public:
	class Iterator : public QueryIterator
	{
	public:
		Iterator(std::unique_ptr<QueryIterator>&& src)
			: _Src(std::move(src))
		{
			MakePair();
		}

		bool Next()
		{
			if(!_Src->Next())
			{
				return false;
			}
			return MakePair();
		}
		bool IsEnd()
		{
			return _Src->IsEnd();
		}
		bool IsNative()
		{
			// we return pair so we never native
			return false;
		}

		LineInfo& NativeValue()
		{
			throw V8RuntimeException("unsupported operation");
		}
		v8::Handle<v8::Value> JsValue()
		{
			if(_Pair.IsEmpty())
			{
				throw V8RuntimeException("Pending exception");
			}
			return v8::Local<v8::Array>::New(v8::Isolate::GetCurrent(), _Pair);
		}
	private:
		bool MakePair()
		{
			if(_Src->IsEnd())
			{
				return false;
			}

			auto v1 = _Src->JsValue();
			if(!_Src->Next())
			{
				return false;
			}
			auto v2 = _Src->JsValue();

			auto pair(v8::Array::New(v8::Isolate::GetCurrent(), 2));
			_Pair.Reset(v8::Isolate::GetCurrent(), pair);
			pair->Set(0, v1);
			pair->Set(1, v2);
			return true;
		}
		std::unique_ptr<QueryIterator> _Src;
		v8::Persistent<v8::Array> _Pair;
	};

	QueryOpPair(const std::shared_ptr<QueryOp>& src)
		: _Source(src)
	{
	}

	TYPE Type()
	{
		return QueryOp::PAIR;
	}

	std::string MakeDescription()
	{
		return std::string("pair ") + _Source->MakeDescription();
	}

	std::unique_ptr<QueryIterator> CreateIterator()
	{
		auto it = _Source->CreateIterator();
		return std::unique_ptr<QueryIterator>(new Iterator(std::move(it)));
	}
private:
	std::shared_ptr<QueryOp> _Source;
};

}
