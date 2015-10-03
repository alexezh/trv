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
class CTextTraceSource;

class CTextTraceFile : public CTraceSource
{
	friend class CTextTraceSource;

public:

	// lines might get split by block boundaries
	// to simplify handling we are going to copy parts of strings
	struct LoadBlock
	{
		LoadBlock()
		{
		}

		// actual position in the file offset for this block
		uint64_t nFileStart = 0;
		uint64_t nFileStop = 0;

		BYTE * pbBuf = nullptr;

		// total size of buf
		DWORD cbBuf = 0;

		// total number of data in the buffer
		DWORD cbData = 0;

		// offset to start writing
		DWORD cbWriteStart = 0;

		// offset from beginning of data to the first line
		DWORD cbFirstFullLineStart = 0;

		// TODO: need better name
		// offset to end of data
		// for ascii is the same as line end
		DWORD cbDataEnd = 0;

		// offset from beginning of data to the end of last line
		DWORD cbLastFullLineEnd = 0;

		// true if buffer was trimmed
		bool isTrimmed = false;
	};

public:
	CTextTraceFile();
	~CTextTraceFile();

	// set a file name and direction of load
	HRESULT Open(LPCWSTR pszFile, CTraceFileLoadCallback * pCallback, bool bReverse = false);
	HRESULT Close();

	// load can be called multiple times passing different stop value
	// dependent on reverse flag the position either indicates the end of beginning
	void Load(uint64_t nStop);

	// Returns the current line count. 
	// The count can change as we add more data at the end or in the beginning
	DWORD GetLineCount() override
	{
		return m_Lines.GetCount();
	}

	const LineInfoDesc& GetDesc() override
	{
		return m_Desc;
	}

	const LineInfo& GetLine(DWORD nIndex) override;
	bool SetTraceFormat(const char * pszFormat, const char* pszSep) override;

	// register update notification handlers
	// called from Refresh()
	void SetHandler(CTraceViewNotificationHandler * pHandler)
	{
		m_pHandler = pHandler;
	}


private:
	static void WINAPI LoadThreadInit(void * pCtx);

	void LoadThread();
	HRESULT AllocBlock(DWORD cbSize, LoadBlock ** ppBlock);
	void TrimBlock(LoadBlock* pBlock);

	// for ascii file pnStop == nStop
	HRESULT ParseBlock(LoadBlock * pBlock, DWORD nStart, DWORD nStop, DWORD * pnDataEnd, DWORD * pnLineEnd);

private:
	std::mutex m_Lock;
	typedef std::lock_guard<std::mutex> LockGuard;
	CTraceFileLoadCallback * m_pCallback = nullptr;

	bool m_bUnicode = false;

	// true if thread is running
	bool m_bLoading = false;

	LARGE_INTEGER m_FileSize;

	// store start / stop position for reading
	uint64_t m_nStart = 0;
	uint64_t m_nStop = 0;
	bool m_bReverse = false;
	uint64_t m_cbTotalAlloc = 0;

	DWORD m_BlockSize = 1024 * 1024 * 1;
	DWORD m_PageSize = 4096;

	CSparseBlockArray<LineInfo, 1024*32> m_Lines;
	std::vector<LoadBlock*> m_Blocks;

	CTraceViewNotificationHandler * m_pHandler = nullptr;
	LineInfoDesc m_Desc;

	std::unique_ptr<TraceLineParser> m_Parser;
	CBitSet m_LineParsed;

	HANDLE m_hFile = INVALID_HANDLE_VALUE;
};

