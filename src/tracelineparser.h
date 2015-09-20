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

// parses input line into fields
class TraceLineParser
{
public:
	enum class FieldId
	{
		None,
		Time,
		ThreadId,
		User1,
		User2,
		User3,
		User4,
		Msg
	};

	TraceLineParser()
	{
	}

	bool ParseLine(const char * pszLine, size_t cchLine, LineInfo& res)
	{
		cchLine = (cchLine == 0) ? strlen(pszLine) : cchLine;
		CStringReaderA rdr(pszLine, cchLine);
		return Parse(rdr, res);
	}

	const std::vector<FieldId>& GetFields()
	{
		return _Fields;
	}

	// throw invalid_argument exception
	bool SetFormat(const char * pszFormat, size_t cch, const std::vector<char>& separators);

private:
	FieldId StringToFieldId(const std::string & str);
	bool SetFormat(CStringReaderA& rdr, const std::vector<char>& separators);

	enum NumberFormat
	{
		NumberDec,
		NumberHex,
	};

	enum class ParseFormatState
	{
		Separator,
		Name,
	};

	std::vector<FieldId> _Fields;
	std::vector<char> _Separators;

	bool Parse(CStringReaderA & rdr, LineInfo& res); 
	void SetLineInfoField(LineInfo& res, FieldId id, const char * pszStart, const char * pszEnd);
};

