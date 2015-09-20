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
#include "traceline.h"
#include "file.h"

namespace Js {

class QueryOpTraceSource : public QueryOp
{
public:

	class Iterator : public QueryIterator
	{
	public:
		Iterator(const std::shared_ptr<CTraceSource>& source)
			: m_Source(source)
		{
			m_Host = GetCurrentHost();
			m_nLines = m_Source->GetLineCount();
			m_idxLine = 0;
		}
		bool Next() override
		{
			if(m_idxLine >= m_nLines)
			{
				return false;
			}
			m_idxLine++;
			if(m_idxLine >= m_nLines)
			{
				return false;
			}
			return true;
		}
		bool IsEnd() override
		{
			return (m_idxLine >= m_nLines);
		}
		bool IsNative() override
		{
			return true;
		}
		const LineInfo& NativeValue() override
		{
			return m_Source->GetLine(m_idxLine);
		}

		// return line wrapped in object
		v8::Handle<v8::Value> JsValue() override
		{
			v8::Local<v8::Value> args = v8::Integer::New(v8::Isolate::GetCurrent(), m_idxLine);
			return TraceLine::GetTemplate(v8::Isolate::GetCurrent())->GetFunction()->NewInstance(1, &args);
		}
	
	private:
		std::shared_ptr<CTraceSource> m_Source;
		IAppHost* m_Host;
		size_t m_idxLine;
		size_t m_nLines;
	};

	QueryOpTraceSource(const std::shared_ptr<CTraceSource>& source)
		: m_Source(source)
	{
	}

	TYPE Type() { return SOURCE; }

	std::string MakeDescription()
	{
		return std::string("trace");
	}

	// evaluate source and produces iterator
	std::unique_ptr<QueryIterator> CreateIterator()
	{
		return std::unique_ptr<QueryIterator>(new Iterator(m_Source));
	}

private:
	std::shared_ptr<CTraceSource> m_Source;
};

class QueryOpTraceCollection : public QueryOp
{
public:

	class Iterator : public QueryIterator
	{
	public:
		Iterator(const std::shared_ptr<CTraceSource>& src, const std::shared_ptr<CBitSet>& lines)
			: Lines(lines)
			, m_Source(src)
		{
			m_Host = GetCurrentHost();
		}
		bool Next() override
		{
			if (m_idxLine >= Lines->GetTotalBitCount())
				return false;

			for(++m_idxLine;m_idxLine < Lines->GetTotalBitCount() && !Lines->GetBit(m_idxLine); ++m_idxLine);

			if (m_idxLine >= Lines->GetTotalBitCount())
				return false;

			return true;
		}
		bool IsEnd() override
		{
			return (m_idxLine >= Lines->GetTotalBitCount());
		}
		bool IsNative() override
		{
			return true;
		}
		const LineInfo& NativeValue() override
		{
			return m_Source->GetLine(m_idxLine);
		}

		// return line wrapped in object
		v8::Handle<v8::Value> JsValue() override
		{
			v8::Local<v8::Value> args = v8::Integer::New(v8::Isolate::GetCurrent(), m_idxLine);
			return TraceLine::GetTemplate(v8::Isolate::GetCurrent())->GetFunction()->NewInstance(1, &args);
		}

	private:
		std::shared_ptr<CTraceSource> m_Source;
		IAppHost* m_Host;
		std::shared_ptr<CBitSet> Lines;
		size_t m_idxLine = 0;
	};

	QueryOpTraceCollection(const std::shared_ptr<CTraceSource>& src, const std::shared_ptr<CBitSet>& lines)
		: m_Lines(lines)
		, m_Source(src)
	{}

	TYPE Type() { return SOURCE; }

	std::string MakeDescription()
	{
		return std::string("trace collection");
	}

	// evaluate source and produces iterator
	std::unique_ptr<QueryIterator> CreateIterator()
	{
		return std::unique_ptr<QueryIterator>(new Iterator(m_Source, m_Lines));
	}

private:
	std::shared_ptr<CTraceSource> m_Source;
	std::shared_ptr<CBitSet> m_Lines;
};

} // Js
