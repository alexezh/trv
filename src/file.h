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

#include "lineinfo.h"
class CBitSet;

///////////////////////////////////////////////////////////////////////////////
//
class CTraceFileLoadCallback
{
public:
    virtual void OnLoadBegin() = 0;
    virtual void OnLoadEnd(HRESULT hr) = 0;

	// reports that more data is available
    virtual void OnLoadBlock() = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
class CTraceViewNotificationHandler
{
public:
	virtual void OnLinesAdded(DWORD nStart, DWORD cAdded) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// 
class CTraceSource
{
public:
	// Returns the current line count. 
	// The count can change as we add more data at the end or in the beginning
    virtual DWORD GetLineCount() = 0;

	virtual LineInfoDesc& GetDesc() = 0;
    virtual LineInfo& GetLine(DWORD nIndex) = 0;
	virtual void UpdateLineActive(DWORD line, int change) = 0;
	virtual void UpdateLinesActive(const CBitSet & set, int change) = 0;
	virtual void GetActiveLinesIndices(std::vector<DWORD> & lines) = 0;
	virtual bool SetTraceFormat(const char * psz) = 0;

	// updates view with changes (if any)
	virtual HRESULT Refresh() = 0;

	virtual void SetHandler(CTraceViewNotificationHandler * pHandler) = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
class CTraceFile
{
public:
	virtual ~CTraceFile() {}

	virtual void CreateCollection(CTraceSource ** ppCollection) = 0;
};

