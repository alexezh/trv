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
#include "apphost.h"
#include "trace.h"

namespace Js {

class QueryOpTraceSource : public QueryOp
{
public:

	class Iterator : public QueryIterator
	{
	public:
		Iterator()
		{
			_Host = GetCurrentHost();
			_nLines = _Host->GetLineCount();
			_idxLine = 0;
		}
		virtual bool Next()
		{
			if(_idxLine >= _nLines)
			{
				return false;
			}
			_idxLine++;
			if(_idxLine >= _nLines)
			{
				return false;
			}
			return true;
		}
		bool IsEnd()
		{
			return (_idxLine >= _nLines);
		}
		bool IsNative()
		{
			return true;
		}
		LineInfo& NativeValue()
		{
			return _Host->GetLine(_idxLine);
		}
		v8::Handle<v8::Value> JsValue()
		{
			v8::Local<v8::Value> args = v8::Integer::New(_idxLine);
			return TraceLine::GetTemplate()->GetFunction()->NewInstance(1, &args);
		}
	
	private:
		IAppHost* _Host;
		size_t _idxLine;
		size_t _nLines;
	};

	TYPE Type() { return SOURCE; }

	std::string MakeDescription()
	{
		return std::string("trace");
	}

	// evaluate source and produces iterator
	std::unique_ptr<QueryIterator> CreateIterator()
	{
		return std::unique_ptr<QueryIterator>(new Iterator());
	}
};

} // Js
