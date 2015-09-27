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

#include "traceapp.h"
#include "textfile.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
//
CTextTraceFile::CTextTraceFile()
	: m_Lines(1024*32)
{
	LineInfoDesc::Reset(m_Desc);
}

CTextTraceFile::~CTextTraceFile()
{
}

void WINAPI CTextTraceFile::LoadThreadInit(void * pCtx)
{
	CTextTraceFile * pFile = (CTextTraceFile*) pCtx;
	pFile->LoadThread();
}

HRESULT CTextTraceFile::Open(LPCWSTR pszFile, CTraceFileLoadCallback * pCallback, bool bReverse)
{
	HRESULT hr = S_OK;

	LOG("@%p open $S", this, pszFile);
	m_pCallback = pCallback;
	m_bReverse = bReverse;

	m_hFile = CreateFile(pszFile,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	if (!GetFileSizeEx(m_hFile, &m_FileSize))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}


Cleanup:

	return hr;
}

HRESULT CTextTraceFile::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
	}
	return S_OK;
}

void CTextTraceFile::Load(QWORD nStop)
{
	if (m_bLoading)
	{
		return;
	}

	if (m_bReverse)
	{
		ATLASSERT(false);
	}
	else
	{
		if (nStop > m_FileSize.QuadPart)
		{
			nStop = (QWORD) m_FileSize.QuadPart;
		}

		if (m_nStop > nStop)
		{
			return;
		}
	}

	m_nStop = nStop;
	m_bLoading = true;

	QueueUserWorkItem((LPTHREAD_START_ROUTINE) LoadThreadInit, this, 0);
}

///////////////////////////////////////////////////////////////////////////////
//
void CTextTraceFile::LoadThread()
{
	HRESULT hr = S_OK;
	DWORD cbRead;
	DWORD cbToRead;
	LARGE_INTEGER liPos;

	m_pCallback->OnLoadBegin();

	for (;; )
	{
		LoadBlock * pNew = nullptr;

		if (m_bReverse)
		{
			ATLASSERT(false);
		}
		else
		{
			DWORD cbRollover = 0;

			if (m_Blocks.size() > 0)
			{
				LoadBlock * pEnd = m_Blocks.back();

				if (pEnd->nFileStop > m_nStop)
				{
					return;
				}

				// copy end of line from previous buffer
				// we have to copy page aligned block and record the start of next data
				assert(pEnd->cbBuf >= pEnd->cbLastFullLineEnd);
				cbRollover = pEnd->cbBuf - pEnd->cbLastFullLineEnd;
				DWORD cbRolloverRounded = (cbRollover + m_PageSize - 1) & (~(m_PageSize - 1));
				IFC(AllocBlock(cbRolloverRounded + m_BlockSize, &pNew));

				pNew->cbFirstFullLineStart = cbRolloverRounded - cbRollover;
				memcpy(pNew->pbBuf + pNew->cbFirstFullLineStart, pEnd->pbBuf + pEnd->cbLastFullLineEnd, cbRollover);

				// at this point we can decommit unnecessary pages for unicode
				// this will waste address space but keep memory usage low

				// nFileStart is in file offset
				// cbLastFullLineEnd is in buffer
				pNew->nFileStart = pEnd->nFileStop;
				pNew->cbWriteStart = cbRolloverRounded;

				// if we are in unicode mode, trim previous block
				if (m_bUnicode)
					TrimBlock(pEnd);
			}
			else
			{
				IFC(AllocBlock(m_BlockSize, &pNew));

				// we are going forward so start pos is null
				pNew->cbWriteStart = 0;
			}
			pNew->nFileStop = pNew->nFileStart + m_BlockSize;

			// we actually have data in the buffer, so set cbData
			pNew->cbData = cbRollover;
		}

		liPos.QuadPart = (__int64) pNew->nFileStart;
		SetFilePointerEx(m_hFile, liPos, NULL, FILE_BEGIN);

		cbToRead = m_BlockSize;

		if (m_bReverse)
		{
			ATLASSERT(false);
		}
		else
		{
			// append data after rollover string
			if (!ReadFile(m_hFile, pNew->pbBuf + pNew->cbWriteStart, cbToRead, &cbRead, NULL))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				goto Cleanup;
			}
		}

		pNew->cbData = cbRead + pNew->cbData;

		// parse data
		IFC(ParseBlock(pNew,
			pNew->cbFirstFullLineStart,
			pNew->cbData,
			&pNew->cbDataEnd,
			&pNew->cbLastFullLineEnd));

		{
			LockGuard guard(m_Lock);
			// append block
			m_Blocks.push_back(pNew);
		}

		m_pCallback->OnLoadBlock();

		if (cbRead != m_BlockSize)
		{
			break;
		}
	}

Cleanup:
	m_pCallback->OnLoadEnd(hr);
}

HRESULT CTextTraceFile::AllocBlock(DWORD cbSize, LoadBlock ** ppBlock)
{
	LoadBlock * b = new LoadBlock;

	b->cbBuf = cbSize;
	b->pbBuf = (BYTE*) VirtualAlloc(NULL, cbSize, MEM_COMMIT, PAGE_READWRITE);

	if (b->pbBuf == NULL)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	m_cbTotalAlloc += cbSize;

	*ppBlock = b;

	return S_OK;
}

void CTextTraceFile::TrimBlock(LoadBlock* pBlock)
{
	if (pBlock->isTrimmed)
		return;

	// unmap unnecessary space
	DWORD cbUsedAligned = ((pBlock->cbDataEnd) / m_PageSize + 1) * m_PageSize;
	if (cbUsedAligned < pBlock->cbBuf)
	{
		VirtualFree(pBlock->pbBuf + cbUsedAligned, pBlock->cbBuf - cbUsedAligned, MEM_DECOMMIT);
		m_cbTotalAlloc -= (pBlock->cbBuf - cbUsedAligned);
	}

	pBlock->isTrimmed = true;
}

HRESULT CTextTraceFile::ParseBlock(LoadBlock * pBlock, DWORD nStart, DWORD nStop, DWORD * pnStop, DWORD * pnLineEnd)
{
	HRESULT hr = S_OK;
	char * pszCur;
	char * pszEnd;
	char * pszLine = NULL;
	WORD crcn = 0;

	BOOL fSkipSpace = FALSE;
	DWORD nVal = 0;

	LockGuard guard(m_Lock);

	// test unicode file
	pszCur = (char*) (pBlock->pbBuf + nStart);
	pszEnd = (char*) (pBlock->pbBuf + nStop);

	if (pBlock->nFileStart == 0 && pBlock->cbData > 2)
	{
		if (pBlock->pbBuf[0] == 0xff && pBlock->pbBuf[1] == 0xfe)
		{
			LOG("@%p unicode mode");
			m_bUnicode = true;
			pszCur += 2;
		}
	}


	if (m_bUnicode)
	{
		wchar_t c;
		wchar_t* pszCurW = reinterpret_cast<wchar_t*>(pszCur);
		wchar_t* pszEndW = reinterpret_cast<wchar_t*>(pszEnd);
		wchar_t* pszLineW = reinterpret_cast<wchar_t*>(pszCur);

		for (; pszCurW < pszEndW; pszCurW++)
		{
			c = *pszCurW;

			crcn <<= 8;
			crcn |= (char) c;

			if (crcn == 0x0d0a)
			{
				// for now just drop first bytes
				char* pszLine = pszCur;
				for (wchar_t* p = pszLineW; p <= pszCurW; p++, pszCur++)
				{
					*pszCur = (char) *p;
				}

				// pszCur points after pszCurW so we do not need +1
				m_Lines.Add(LineInfo(CStringRef(pszLine, pszCur - pszLine), m_Lines.GetCount()));
				pszLineW = pszCurW + 1;
			}
		}

		(*pnStop) = ((BYTE*) pszCur - pBlock->pbBuf);
		(*pnLineEnd) = (pszLineW) ? ((BYTE*) pszLineW - pBlock->pbBuf) : ((BYTE*) pszEndW - pBlock->pbBuf);
	}
	else
	{
		pszLine = pszCur;
		char c;
		for (; pszCur < pszEnd; pszCur++)
		{
			c = *pszCur;

			crcn <<= 8;
			crcn |= c;

			if (crcn == 0x0d0a)
			{
				m_Lines.Add(LineInfo(CStringRef(pszLine, pszCur - pszLine + 1), m_Lines.GetCount()));
				pszLine = pszCur + 1;
			}
		}

		(*pnStop) = nStop;
		(*pnLineEnd) = (pszLine) ? ((BYTE*) pszLine - pBlock->pbBuf) : ((BYTE*) pszEnd - pBlock->pbBuf);
	}

	// we are parsing under lock; it is safe to adjust the size
	m_LineParsed.Resize(m_Lines.GetCount());

	//Cleanup:

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
const LineInfo& CTextTraceFile::GetLine(DWORD nIndex)
{
	LockGuard guard(m_Lock);
	if (nIndex >= m_Lines.GetCount())
	{
		static LineInfo line;
		return line;
	}

	LineInfo& line = m_Lines.GetAt(nIndex);
	if (!m_LineParsed.GetBit(nIndex))
	{
		m_LineParsed.SetBit(nIndex);
		if (m_Parser == nullptr || !m_Parser->ParseLine(line.Content.psz, line.Content.cch, line))
		{
			// just set msg as content
			line.Msg = line.Content;
		}
	}

	return line;
}

bool CTextTraceFile::SetTraceFormat(const char * pszFormat, const char* pszSep)
{
	LockGuard guard(m_Lock);
	LineInfoDesc::Reset(m_Desc);
	m_Parser.reset(new TraceLineParser());
	try
	{
		std::vector<char> sep;
		if (pszSep == nullptr)
		{
			sep.push_back('\t');
		}
		else
		{
			sep.assign(pszSep, pszSep + strlen(pszSep));
		}
		m_Parser->SetFormat(pszFormat, 0, sep);
	}
	catch (std::invalid_argument&)
	{
		return false;
	}

	// check what we captured
	for (auto it = m_Parser->GetFields().begin(); it != m_Parser->GetFields().end(); ++it)
	{
		if (*it == TraceLineParser::FieldId::ThreadId)
		{
			m_Desc.Tid = true;
		}
		else if (*it == TraceLineParser::FieldId::Time)
		{
			m_Desc.Time = true;
		}
		else if (*it == TraceLineParser::FieldId::User1)
		{
			m_Desc.SetUser(0);
		}
		else if (*it == TraceLineParser::FieldId::User2)
		{
			m_Desc.SetUser(1);
		}
		else if (*it == TraceLineParser::FieldId::User3)
		{
			m_Desc.SetUser(2);
		}
		else if (*it == TraceLineParser::FieldId::User4)
		{
			m_Desc.SetUser(3);
		}
	}

	// reset all parsed bits
	m_LineParsed.Fill(false);

	return true;
}

