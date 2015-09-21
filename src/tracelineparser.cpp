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
#include "tracelineparser.h"
#include "js/error.h"

///////////////////////////////////////////////////////////////////////////////
//
bool TraceLineParser::SetFormat(const char * pszFormat, size_t cch, const std::vector<char>& separators)
{
	cch = (cch == 0) ? strlen(pszFormat) : cch;
	CStringReaderA rdr(pszFormat, cch);
	_Separators.assign(separators.begin(), separators.end());
	_Fields.resize(0);

	if (!SetFormat(rdr, separators))
	{
		_Fields.resize(0);
		return false;
	}

	return true;
}

bool TraceLineParser::SetFormat(CStringReaderA& rdr, const std::vector<char>& separators)
{
	std::string fieldName;

	while (!rdr.IsEOL())
	{
		char c = rdr.ReadChar();
		if (c == '|')
		{
			_Fields.push_back(StringToFieldId(fieldName));
			fieldName.erase();
		}
		else
		{
			fieldName += c;
		}
	}

	if (fieldName.length() > 0)
		_Fields.push_back(StringToFieldId(fieldName));

	return true;
}

bool TraceLineParser::Parse(CStringReaderA & rdr, LineInfo & res)
{
	if (_Fields.size() == 0 || _Separators.size() == 0)
	{
		res.Msg.psz = rdr.P();
		res.Msg.cch = rdr.Length();
	}
	else
	{
		const char * pszStart = rdr.P();
		size_t idxField = 0;
		size_t nSep = _Separators.size();
		// we already verified that we have at least one separator
		assert(nSep > 0); 
		char cSep = _Separators[0];
		while (!rdr.IsEOL())
		{
			char c = rdr.ReadChar();
			bool sep = false;
			if (nSep == 1)
			{
				sep = (c == cSep);
			}
			else
			{
				assert(false);
			}

			if(sep)
			{
				// skip multiple separators without content
				if (pszStart + 1 == rdr.P())
				{
					pszStart = rdr.P();
					continue;
				}

				if (idxField < _Fields.size())
				{
					SetLineInfoField(res, _Fields[idxField++], pszStart, rdr.P() - 1);
				}
				else
				{
					// pass the rest as message
					SetLineInfoField(res, FieldId::Msg, pszStart, rdr.P() + rdr.Length());
					break;
				}

				pszStart = rdr.P();
			}
			else
			{
				;
			}
		}
	}
	
	return true;
}

TraceLineParser::FieldId TraceLineParser::StringToFieldId(const std::string & str)
{
	if(str == "time")
	{
		return FieldId::Time;
	}
	else if (str == "tid")
	{
		return FieldId::ThreadId;
	}
	else if (str == "user1")
	{
		return FieldId::User1;
	}
	else if (str == "user2")
	{
		return FieldId::User2;
	}
	else if (str == "user3")
	{
		return FieldId::User3;
	}
	else if (str == "user4")
	{
		return FieldId::User4;
	}
	else
	{
		return FieldId::None;
	}
}

void TraceLineParser::SetLineInfoField(LineInfo& res, FieldId id, const char * pszStart, const char * pszEnd)
{
	switch(id)
	{
	case FieldId::Time: 
		res.Time.psz = pszStart;
		res.Time.cch = pszEnd - pszStart;
		break;
	case FieldId::ThreadId:
		{
			CStringReaderA rdr(pszStart, pszEnd-pszStart);
			int ibase = 10;
			if (rdr.Length() > 2 && strncmp(rdr.P(), "0x", 2) == 0)
			{
				rdr.ReadChar();
				rdr.ReadChar();
				ibase = 16;
			}
			rdr.ReadUInt32(&res.Tid, ibase);
		}
		break;
	case FieldId::User1:
		res.User[0].psz = pszStart;
		res.User[0].cch = pszEnd - pszStart;
		break;
	case FieldId::User2:
		res.User[1].psz = pszStart;
		res.User[1].cch = pszEnd - pszStart;
		break;
	case FieldId::User3:
		res.User[2].psz = pszStart;
		res.User[2].cch = pszEnd - pszStart;
		break;
	case FieldId::User4:
		res.User[3].psz = pszStart;
		res.User[3].cch = pszEnd - pszStart;
		break;
	case FieldId::Msg:
		res.Msg.psz = pszStart;
		res.Msg.cch = pszEnd - pszStart;
		break;
	case FieldId::None:
		break;
	default:
		assert(false);
		break;
	}
}

