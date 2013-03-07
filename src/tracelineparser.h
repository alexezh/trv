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

#include "stringref.h"
#include "stringreader.h"
#include "lineinfo.h"
#include "testassert.h"

// format string uses perl expression syntax for extracting fields
// regexp implementation is simplified for our needs. 
// The only supported functionality is 
// Special sequences are
//    () - group
//    [] - set
//    ?name() - named group
//    \w - char
//    \d - match digit
//    \x - match hex digit
//    \s - match white space
//    . - match any char
//    +
//    *
// Known group names
//    year, month, date, hour, min, sec
//    thread
//    msg
// Nested groups not supported
class TraceLineParser
{
public:
	enum class FieldId
	{
		None,
		Year,
		Month,
		Day,
		Hour,
		Minute,
		Second,
		Nanosecond,
		Thread,
		Msg
	};

	TraceLineParser()
		: _TidFormat(NumberDec)
	{
	}

	bool ParseLine(const char * pszLine, size_t cchLine, LineInfo& res)
	{
		cchLine = (cchLine == 0) ? strlen(pszLine) : cchLine;
		CStringReaderA rdr(pszLine, cchLine);
		return Match(rdr, _Root.get(), res);
	}

	const std::vector<FieldId>& GetFields()
	{
		return _Fields;
	}

	// throw invalid_argument exception
	bool SetFormat(const char * pszFormat, size_t cch);

	// test support
	static bool Test();
	static bool TestSetFormat(const char * pszFormat);
private:
	enum NumberFormat
	{
		NumberDec,
		NumberHex,
	};

	enum OpType
	{
		OpMatch,
		OpGroup,
		// we can have another element Seq
	};
	
	enum RepeatFlag
	{
		Single,
		ZeroOrMore,
		OneOrMore,
	};

	struct Op 
	{
		OpType Type;
	};
	
	struct GroupOp : public Op
	{
		GroupOp()
		{
			Type = OpGroup;
			Gid = FieldId::None;
		}
		FieldId Gid;
		std::vector<std::unique_ptr<Op> > Children;
	};
	
	struct MatchOp : public Op
	{
		MatchOp()
		{
			Type = OpMatch;
			Rep = Single;
			Fill(false);
		}

		void Fill(bool b)
		{
			for(size_t i = 0; i < sizeof(Set); i++)
			{
				Set[i] = b;
			}
		}

		void FillRange(unsigned char start, unsigned char end, bool b)
		{
			if(end < start)
			{
				throw std::invalid_argument("incorrect [] sequence");
				return;
			}
			for(size_t i = start; i <= end; i++)
			{
				Set[i] = b;
			}
		}

		bool Match(CStringReaderA& rdr)
		{
			int cMatch = 0;
			while(!rdr.IsEOL())
			{
				char c = rdr.PeekChar();
				if(Set[c])
				{
					rdr.ReadChar();
					cMatch++;

					// always greedy
					if(Rep == Single)
					{
						return true;
					}

					continue;
				}
				else
				{
					// if we had a match or we matching 0 or more
					if(Rep == ZeroOrMore || cMatch > 0)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
			}
			return true;
		}

		RepeatFlag Rep;
		// chars which match this expr
		bool Set[256];
	};
	typedef std::vector<std::unique_ptr<Op> > OpVector;
	std::unique_ptr<GroupOp> _Root;
	std::vector<FieldId> _Fields; 
	NumberFormat _TidFormat;

	class FormatParser
	{
	public:
		FormatParser(CStringReaderA & rdr, OpVector & res)
			: _Res(res)
		{
			_Rdr.Init(rdr);
		}

		bool Parse();
	private:
		bool ParseNamedGroup();
		bool ParseSet(std::unique_ptr<TraceLineParser::MatchOp> & m);
		bool ParseEscapedChar(std::unique_ptr<MatchOp> & m);
		FieldId StringToFieldId(const std::string & str);
		std::unique_ptr<GroupOp> ParseGroup();
		std::unique_ptr<MatchOp> ParseMatch();
		static bool IsEscapedSimple(char c);

		CStringReaderA _Rdr;
		OpVector& _Res;
	};

	bool Match(CStringReaderA & rdr, GroupOp * op, LineInfo& res); 
	void ValidateFields(GroupOp * op); 
	void SetLineInfoField(LineInfo& res, FieldId id, const char * pszStart, const char * pszEnd);
};

