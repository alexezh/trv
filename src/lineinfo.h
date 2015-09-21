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

struct LineInfoDesc
{
	static void Reset(LineInfoDesc& v)
	{
		ZeroMemory(&v, sizeof(v));
	}

	uint16_t Tid : 1;
	uint16_t Time : 1;
	uint16_t Msg : 1;
	uint16_t User : 4;

	bool GetUser(size_t idx) const
	{
		return (User & (1 << idx)) != 0;
	}

	void SetUser(size_t idx)
	{
		User |= (1 << idx);
	}

	enum
	{
		MaxUser = 4
	};
};

struct LineInfo
{
	LineInfo()
		: Content()
		, Tid(0)
	{
	}
	LineInfo(CStringRef && content)
		: Content(content)
	{
	}

	CStringRef Content;

	// line index in the file
	DWORD Index;
	DWORD Tid = 0;

	// we can compress fields by referencing content in string
	// since string is short (we can limit to 65K), we can package to 32bit
	CStringRef Time;
	CStringRef User[LineInfoDesc::MaxUser];
	CStringRef Msg;
};
