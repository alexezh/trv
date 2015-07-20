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

class CTextTraceFile : public CTraceFile
{
	friend class CTextTraceSource;

public:

	// lines might get split by block boundaries
	// to simplify handling we are going to copy parts of strings
	struct LoadBlock
	{
		LoadBlock()
			: Lines(1024 * 32)
		{
		}

		// actual position in the file offset for this block
		QWORD nFileStart = 0;
		QWORD nFileStop = 0;

		BYTE * pbBuf = nullptr;
		
		// total size of buf
		DWORD cbBuf = 0;

		// total number of data in the buffer
		DWORD cbData = 0;

		// offset to start writing
		DWORD cbWriteStart = 0;

		// offset from beginning of data to the first line
		DWORD cbFirstFullLineStart = 0;

		// offset to end of data
		// for ascii is the same as line end
		DWORD cbDataEnd = 0;

		// offset from beginning of data to the end of last line
		DWORD cbLastFullLineEnd = 0;

		// lines which were parsed but not reported
		CBlockArray<LineInfo> Lines;

		// bit flag for each line indicating if line was parsed
		CBitSet LineParsed;
	};

public:
    CTextTraceFile();
    ~CTextTraceFile();
    
	// set a file name and direction of load
    HRESULT Open(LPCWSTR pszFile, CTraceFileLoadCallback * pCallback, bool bReverse = false);
    HRESULT Close();

	// load can be called multiple times passing different stop value
	// dependent on reverse flag the position either indicates the end of beginning
	void Load(QWORD nStop);

	void CreateCollection(CTraceSource ** ppCollection);

private:    
    static void WINAPI LoadThreadInit(void * pCtx);

    void LoadThread();
    HRESULT AllocBlock(DWORD cbSize, LoadBlock ** ppBlock);

	// for ascii file pnStop == nStop
    HRESULT ParseBlock(LoadBlock * pBlock, DWORD nStart, DWORD nStop, DWORD * pnDataEnd, DWORD * pnLineEnd);

private:
    CTraceFileLoadCallback * m_pCallback = nullptr;

	bool m_bUnicode = false;

	// true if thread is running
	bool m_bLoading = false;

	LARGE_INTEGER m_FileSize;

	// store start / stop position for reading
    QWORD m_nStart = 0;
    QWORD m_nStop = 0;
	bool m_bReverse = false;

	DWORD m_BlockSize = 1024 * 1024 * 1;
	DWORD m_PageSize = 4096;

	std::vector<LoadBlock*> m_Blocks;
	DWORD m_TotalLines = 0;

	HANDLE m_hFile = INVALID_HANDLE_VALUE;
};

///////////////////////////////////////////////////////////////////////////////
//
class CTextTraceSource : public CTraceSource
{
public:
	CTextTraceSource(CTextTraceFile * pFile)
		: m_pFile(pFile)
	{
		LineInfoDesc::Reset(m_Desc);
	}

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
	bool CacheIndex(DWORD nIndex);

private:
	std::mutex m_Lock;
	typedef std::lock_guard<std::mutex> LockGuard;

	CTraceViewNotificationHandler * m_pHandler = nullptr;

	LineInfoDesc m_Desc;

	// collection of lines across all blocks
	std::vector<CTextTraceFile::LoadBlock*> m_Blocks;

	DWORD m_nTotal = 0;
	DWORD m_nActive = 0;

	// cached last block info
	CTextTraceFile::LoadBlock * m_pLastBlock = nullptr;
	size_t m_nLastBlockIndex = -1;
	// line number at which the last block starts
	DWORD m_nLastBlockStart = 0;

	std::unique_ptr<TraceLineParser> m_Parser;
	
	CTextTraceFile * m_pFile = nullptr;
};
