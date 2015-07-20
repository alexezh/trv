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
#include "blockarray.h"
#include "tracelineparser.h"
#include "bitset.h"
#include "file.h"

///////////////////////////////////////////////////////////////////////////////
//
class CDebugOutputTraceSource : public CTraceSource
{
public:
	CDebugOutputTraceSource()
	{
		LineInfoDesc::Reset(m_Desc);
	}

	bool Init();

	// Returns the current line count. 
	// The count can change as we add more data at the end or in the beginning
	DWORD GetLineCount() override
	{
		return m_nTotal;
	}

	LineInfoDesc& GetDesc() override
	{
		return m_Desc;
	}

	LineInfo& GetLine(DWORD nIndex) override;
	void UpdateLineActive(DWORD line, int change) override;
	void UpdateLinesActive(const CBitSet & set, int change) override;
	void GetActiveLinesIndices(std::vector<DWORD> & lines) override;
	bool SetTraceFormat(const char * psz) override;

	// updates view with changes (if any)
	HRESULT Refresh() override;

	// register update notification handlers
	// called from Refresh()
	void SetHandler(CTraceViewNotificationHandler * pHandler)
	{
		m_pHandler = pHandler;
	}

private:
	struct Line
	{
		LineInfo Info;
		char Data[1];
	};

	static void WINAPI MonitorThreadInit(void * pCtx);
	void MonitorThread();
	std::unique_ptr<Line> AllocLine(const char* pszLine);

	std::mutex m_Lock;
	typedef std::lock_guard<std::mutex> LockGuard;

	HANDLE m_AckEvent = nullptr;
	HANDLE m_ReadyEvent = nullptr;
	HANDLE m_SharedFile = nullptr;
	void* m_SharedMem = nullptr;

	CTraceViewNotificationHandler * m_pHandler = nullptr;

	LineInfoDesc m_Desc;

	// collection of lines across all blocks
	std::vector<std::unique_ptr<Line>> m_Lines;

	DWORD m_nTotal = 0;
	DWORD m_nActive = 0;

	std::unique_ptr<TraceLineParser> m_Parser;
};
