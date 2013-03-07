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
bool TraceLineParser::SetFormat(const char * pszFormat, size_t cch)
{
	cch = (cch == 0) ? strlen(pszFormat) : cch;
	CStringReaderA rdr(pszFormat, cch);
	_Root.reset(new GroupOp);
	FormatParser fmt(rdr, _Root->Children);
	if(!fmt.Parse())
	{
		return false;
	}

	ValidateFields(_Root.get());

	return true;
}

void TraceLineParser::ValidateFields(GroupOp * op)
{
	// std::unique_ptr<Op> & op = (*it);
	for(auto it = op->Children.begin(); it != op->Children.end(); it++)
	{
		if((*it)->Type == OpGroup)
		{
			auto g = static_cast<GroupOp*>(it->get());
			
			_Fields.push_back(g->Gid);

			if(g->Gid == FieldId::Thread)
			{
				// check if we can match hex number
				CStringReaderA rdr("Ab10", 4);
				_TidFormat = NumberHex;

				for(auto it = g->Children.begin(); it != g->Children.end(); it++)
				{
					if((*it)->Type == OpMatch)
					{
						MatchOp * m = static_cast<MatchOp*>(it->get());
						if(!m->Match(rdr))
						{
							_TidFormat = NumberDec;
							break;
						}
					}
					else
					{
						assert(false);
						_TidFormat = NumberDec;
						break;
					}
				}
			}
		}
	}
}

bool TraceLineParser::Match(CStringReaderA & rdr, GroupOp * op, LineInfo & res)
{
	// std::unique_ptr<Op> & op = (*it);
	for(auto it = op->Children.begin(); it != op->Children.end(); it++)
	{
		if((*it)->Type == OpMatch)
		{
			MatchOp * m = static_cast<MatchOp*>(it->get());
			if(!m->Match(rdr))
			{
				return false;
			}
		}
		else
		{
			auto g = static_cast<GroupOp*>(it->get());
			const char * pszBegin = rdr.P();
			if(!Match(rdr, g, res))
			{
				return false;
			}
			const char * pszEnd = rdr.P();
			SetLineInfoField(res, g->Gid, pszBegin, pszEnd);
		}
	}
	
	return true;
}

bool TraceLineParser::FormatParser::Parse()
{
	while(!_Rdr.IsEOL())
	{
		char c = _Rdr.PeekChar();
		switch(c)
		{
		case '?':
			if(!ParseNamedGroup())
			{
				return false;
			}
			break;
		case '(':
			{
				// unnamed group is mapped to message
				std::unique_ptr<GroupOp> g = ParseGroup();
				if(!g)
				{
					return false;
				}

				g->Gid = FieldId::Msg;
				_Res.push_back(std::move(g));
			}
			break;
		case ')':
			throw std::invalid_argument("invalid syntax. unidentified (");
		default:
			auto m = ParseMatch();
			_Res.push_back(std::move(m));
			break;
		}
	}

	return true;
}

bool TraceLineParser::FormatParser::ParseNamedGroup()
{
	assert(_Rdr.PeekChar() == '?');
	_Rdr.ReadChar();
	std::string name;

	// first read name until (
	while(!_Rdr.IsEOL())
	{
		char c = _Rdr.PeekChar();
		if(c == '(')
		{
			break;
		}
		else if(c >= 'a' && c <= 'z')
		{
			_Rdr.ReadChar();
			name.push_back(c);
			continue;
		}
		else
		{
			throw std::invalid_argument("Invalid syntax: incorrect symbol in a ?name()");
		}
	}

	FieldId gid = StringToFieldId(name);
	if(gid == FieldId::None)
	{
		throw std::invalid_argument("Invalid syntax: incorrect group name");
	}

	std::unique_ptr<GroupOp> g = ParseGroup();
	if(!g)
	{
		return false;
	}

	g->Gid = gid;
	_Res.push_back(std::move(g));
	return true;
}

TraceLineParser::FieldId TraceLineParser::FormatParser::StringToFieldId(const std::string & str)
{
	if(str == "year")
	{
		return FieldId::Year;
	}
	else if(str == "month")
	{
		return FieldId::Month;
	}
	else if(str == "day")
	{
		return FieldId::Day;
	}
	else if(str == "hour")
	{
		return FieldId::Hour;
	}
	else if(str == "minute")
	{
		return FieldId::Minute;
	}
	else if(str == "second")
	{
		return FieldId::Second;
	}
	else if(str == "nanosecond")
	{
		return FieldId::Nanosecond;
	}
	else if(str == "tid")
	{
		return FieldId::Thread;
	}
	else
	{
		return FieldId::None;
	}
}

std::unique_ptr<TraceLineParser::GroupOp> TraceLineParser::FormatParser::ParseGroup()
{
	assert(_Rdr.PeekChar() == '(');
	_Rdr.ReadChar();

	std::unique_ptr<TraceLineParser::GroupOp> g(new TraceLineParser::GroupOp);
	while(!_Rdr.IsEOL())
	{
		char c = _Rdr.PeekChar();
		if(c == '(')
		{
			throw std::invalid_argument("Invalid syntax: nested groups not supported");
		}
		if(c == ')')
		{
			_Rdr.ReadChar();
			break;
		}
		auto m = ParseMatch();
		if(m == nullptr)
		{
			break;
		}
		g->Children.push_back(std::move(m));
	}
	return std::move(g);
}

bool TraceLineParser::FormatParser::ParseSet(std::unique_ptr<TraceLineParser::MatchOp> & m)
{
	assert(_Rdr.PeekChar() == '[');
	_Rdr.ReadChar();
	while(!_Rdr.IsEOL())
	{
		char c = _Rdr.PeekChar();
		if(c == ']')
		{
			_Rdr.ReadChar();
			break;
		}
		
		if(c == '\\')
		{
			_Rdr.ReadChar();
			c = _Rdr.PeekChar();
			if(!IsEscapedSimple(c))
			{
				return false;
			}
		}
		// read next char; check if asked for sequence
		_Rdr.ReadChar();
		if(_Rdr.PeekChar() != '-')
		{
			m->Set[c] = true;
			continue;
		}

		// read - and next char
		_Rdr.ReadChar();
		char c2 = _Rdr.PeekChar();
		if(c2 == '\\')
		{
			_Rdr.ReadChar();
			c2 = _Rdr.PeekChar();
			if(!IsEscapedSimple(c2))
			{
				return false;
			}
		}

		_Rdr.ReadChar();
		m->FillRange(c, c2, true);
	}

	return std::move(m);
}
bool TraceLineParser::FormatParser::IsEscapedSimple(char c)
{
	return (c == '.' || c == '/' || c == '\\' || c == '*' || c == '[' || c == ']');
}
bool TraceLineParser::FormatParser::ParseEscapedChar(std::unique_ptr<MatchOp> & m)
{
	assert(_Rdr.PeekChar() == '\\');
	_Rdr.ReadChar();
	char c = _Rdr.ReadChar();
	if(IsEscapedSimple(c))
	{
		m->Set[c] = true;
		return true;
	}

	switch(c)
	{
	case 'd':
		m->FillRange('0', '9', true);
		break;
	case 's':
		m->Set[' '] = true;
		m->Set['\t'] = true;
		break;
	case 'x':
		m->FillRange('0', '9', true);
		m->FillRange('a', 'f', true);
		m->FillRange('A', 'F', true);
		break;
	default:
		throw std::invalid_argument("Invalid syntax: incorrect escape symbol");
		return false;
	}
	return true;
}
// match is either char or set
std::unique_ptr<TraceLineParser::MatchOp> TraceLineParser::FormatParser::ParseMatch()
{
	std::unique_ptr<TraceLineParser::MatchOp> m(new MatchOp);
	char c = _Rdr.PeekChar();
	if(c == '[')
	{
		if(!ParseSet(m))
		{
			throw std::invalid_argument("Invalid syntax: incorrect set specification");
			return nullptr;
		}
	}
	else if(c == '.')
	{
		_Rdr.ReadChar();
		m->Fill(true);
	}
	else if(c == '\\')
	{
		if(!ParseEscapedChar(m))
		{
			throw std::invalid_argument("Invalid syntax: incorrect escape sequence");
		}
	}
	else
	{
		m->Set[c] = true;
		_Rdr.ReadChar();
	}

	// see if next char is * or +
	c = _Rdr.PeekChar();
	if(c == '+')
	{
		m->Rep = OneOrMore;
		_Rdr.ReadChar();
	}
	else if(c == '*')
	{
		m->Rep = ZeroOrMore;
		_Rdr.ReadChar();
	}

	return std::move(m);
}
void TraceLineParser::SetLineInfoField(LineInfo& res, FieldId id, const char * pszStart, const char * pszEnd)
{
	switch(id)
	{
	case FieldId::Year: break;
	case FieldId::Month: break;
	case FieldId::Day: break;
	case FieldId::Hour: break;
	case FieldId::Minute: break;
	case FieldId::Second: break;
	case FieldId::Thread: 
		{
			CStringReaderA rdr(pszStart, pszEnd-pszStart);
			rdr.ReadUInt32(&res.Tid, (_TidFormat == NumberDec) ? 10 : 16);
		}
		break;
	case FieldId::Msg: 
		res.Msg.psz = pszStart; 
		res.Msg.cch = pszEnd - pszStart;
		break;
	}
}

bool TraceLineParser::TestSetFormat(const char * pszFormat)
{
	TraceLineParser t;
	try
	{
		TestTrue(t.SetFormat(pszFormat, 0));
	}
	catch(std::invalid_argument &)
	{
		return false;
	}
	return true;
}
bool TraceLineParser::Test()
{
	{
		TraceLineParser t;
		LineInfo res;
		t.SetFormat("?year(\\d+)-?month(\\d+)-(.*)", 0);
		TestTrue(t.ParseLine("100-20-Hello", 0, res));
		TestTrue(t.ParseLine("s-20-Hello", 0, res) == false);
		TestTrue(t.ParseLine("100+20-Hello", 0, res) == false);
	}
	
	TestFalse(TestSetFormat("?yaar([0-9]+)-?month(\\d+)-(.*)"));
	TestFalse(TestSetFormat("?year([\\0-9]+)-?month(\\d+)-(.*)"));
	TestFalse(TestSetFormat("?year([0-9+)-?month(\\d+)-(.*)"));
	TestFalse(TestSetFormat("?year([0-9]+())-?month(\\d+)-(.*)"));
	TestFalse(TestSetFormat("?year([0-9]+)-?month(\\p+)-(.*)"));

	{
		TraceLineParser t;
		LineInfo res;
		TestTrue(t.SetFormat("?year([0-9]+)-?month(\\d+)-(.*)", 0));
		TestTrue(t.ParseLine("1-20-Hello", 0, res) == true);
		TestTrue(t.ParseLine("b-20-Hello", 0, res) == false);
	}

	{
		TraceLineParser t;
		LineInfo res;
		TestTrue(t.SetFormat("\\[\\.\\*", 0));
		TestTrue(t.ParseLine("[.*", 0, res) == true);
	}

	return true;
}

