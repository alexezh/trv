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
{
}

CTextTraceFile::~CTextTraceFile()
{
}

void WINAPI CTextTraceFile::LoadThreadInit(void * pCtx)
{
    CTextTraceFile * pFile = (CTextTraceFile*)pCtx;
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

    if(m_hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

	if(!GetFileSizeEx(m_hFile, &m_FileSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

   
Cleanup:

    return hr;
}

HRESULT CTextTraceFile::Close()
{
    if(m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
    }
    return S_OK;
}

void CTextTraceFile::Load(QWORD nStop)
{
	if(m_bLoading)
	{
		return;
	}

	if(m_bReverse)
	{
		ATLASSERT(false);
	}
	else
	{
		if(nStop > m_FileSize.QuadPart)
		{
			nStop = (QWORD)m_FileSize.QuadPart;
		}

		if(m_nStop > nStop)
		{
			return;
		}
	}
    
	m_nStop = nStop;
	m_bLoading = true;

    QueueUserWorkItem((LPTHREAD_START_ROUTINE)LoadThreadInit, this, 0);
}

void CTextTraceFile::CreateCollection(CTraceSource ** ppCollection)
{
	(*ppCollection) = new CTextTraceSource(this);
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
    
    for(;; )
    {
		LoadBlock * pNew = nullptr;

		if(m_bReverse)
		{
			ATLASSERT(false);
		}
		else
		{
			DWORD cbRollover = 0;

			if(m_Blocks.size() > 0)
			{
				LoadBlock * pEnd = m_Blocks.back();

				if(pEnd->nFileStop > m_nStop)
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
				memcpy(pNew->pbBuf + pNew->cbFirstFullLineStart, pEnd->pbBuf+pEnd->cbLastFullLineEnd, cbRollover);

				// at this point we can decommit unnecessary pages for unicode
				// this will waste address space but keep memory usage low

				// nFileStart is in file offset
				// cbLastFullLineEnd is in buffer
				pNew->nFileStart = pEnd->nFileStop;
				pNew->cbWriteStart = cbRolloverRounded;
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

		liPos.QuadPart = (__int64)pNew->nFileStart;
		SetFilePointerEx(m_hFile, liPos, NULL, FILE_BEGIN);
    
        cbToRead = m_BlockSize;
        
		if(m_bReverse)
		{
			ATLASSERT(false);
		}
		else
		{
			// append data after rollover string
			if(!ReadFile(m_hFile, pNew->pbBuf+pNew->cbWriteStart, cbToRead, &cbRead, NULL))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				goto Cleanup;
			}
		}
        
        pNew->cbData = cbRead + pNew->cbData;

        // parse data
		DWORD cbNewData;
		IFC(ParseBlock(pNew, 
						pNew->cbFirstFullLineStart, 
						pNew->cbData, 
						&cbNewData,
						&pNew->cbLastFullLineEnd));

		// append block
		pNew->LineParsed.Init(pNew->Lines.GetCount());
		m_Blocks.push_back(pNew);
		m_TotalLines += pNew->Lines.GetCount();

        m_pCallback->OnLoadBlock();

		if(cbRead != m_BlockSize)
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
    b->pbBuf = (BYTE*)VirtualAlloc(NULL, cbSize, MEM_COMMIT, PAGE_READWRITE);

    if(b->pbBuf == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

	*ppBlock = b;
    
    return S_OK;
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

	// test unicode file
	pszCur = (char*)(pBlock->pbBuf + nStart);
	pszEnd = (char*)(pBlock->pbBuf + nStop);

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
			crcn |= (char)c;

			if (crcn == 0x0d0a)
			{
				// for now just drop first bytes
				char* pszLine = pszCur;
				for (wchar_t* p = pszLineW; p <= pszCurW; p++, pszCur++)
				{
					*pszCur = (char)*p;
				}

				// pszCur points after pszCurW so we do not need +1
				pBlock->Lines.Add(LineInfo(CStringRef(pszLine, pszCur - pszLine)));
				pszLineW = pszCurW + 1;
			}
		}

		(*pnStop) = ((BYTE*)pszCur - pBlock->pbBuf);
		(*pnLineEnd) = (pszLineW) ? ((BYTE*)pszLineW - pBlock->pbBuf) : ((BYTE*)pszEndW - pBlock->pbBuf);
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
				pBlock->Lines.Add(LineInfo(CStringRef(pszLine, pszCur - pszLine + 1)));
				pszLine = pszCur + 1;
			}
		}

		(*pnStop) = nStop;
		(*pnLineEnd) = (pszLine) ? ((BYTE*)pszLine - pBlock->pbBuf) : ((BYTE*)pszEnd - pBlock->pbBuf);
	}

    
//Cleanup:

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CTextTraceSource::CacheIndex(DWORD nIndex)
{
	if(m_pLastBlock)
	{
		if(nIndex < m_nLastBlockStart || nIndex - m_nLastBlockStart >= m_pLastBlock->Lines.GetCount())
		{
			m_pLastBlock = nullptr;
		}
	}

	if(m_pLastBlock == nullptr)
	{
		DWORD nBlockStart = 0;
		for(size_t i = 0; i < m_Blocks.size(); i++)
		{
			auto p = m_Blocks[i];

			if(nIndex - nBlockStart < p->Lines.GetCount())
			{
				m_pLastBlock = p;
				m_nLastBlockStart = nBlockStart;
				m_nLastBlockIndex = i;
				break;
			}
			else
			{
				nBlockStart += p->Lines.GetCount();
			}
		}

		if(m_pLastBlock == nullptr)
		{
			ATLASSERT(false);
			return false;
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
LineInfo& CTextTraceSource::GetLine(DWORD nIndex)
{
	LockGuard guard(m_Lock);
	if(!CacheIndex(nIndex))
	{
		static LineInfo line;
		return line;
	}

	DWORD lineIdx = nIndex - m_nLastBlockStart;
	LineInfo& line = m_pLastBlock->Lines.GetAt(lineIdx);
	if(m_Parser && !m_pLastBlock->LineParsed.GetBit(lineIdx))
	{
		m_pLastBlock->LineParsed.SetBit(lineIdx);
		if(!m_Parser->ParseLine(line.Content.psz, line.Content.cch, line))
		{
			// just set msg as content
			line.Msg = line.Content;
		}
	}

	// since the only way we can get line is here
	// we can update index on line entry
	line.Index = nIndex;

	return line;
}

void CTextTraceSource::UpdateLineActive(DWORD line, int change)
{
	LockGuard guard(m_Lock);
	size_t nBlockStart = 0;

	for (size_t i = 0; i < m_Blocks.size(); i++)
	{
		auto p = m_Blocks[i];

		if (nBlockStart + p->Lines.GetCount() > line)
		{
			int curActive = p->Lines.GetAt(line - nBlockStart).Active;
			int newActive = curActive + change;

			if (curActive == 0)
			{
				m_nActive++;
			}

			if (newActive == 0)
			{
				m_nActive--;
			}

			p->Lines.GetAt(line - nBlockStart).Active = newActive;
			break;
		}

		nBlockStart += p->Lines.GetCount();
	}
}

void CTextTraceSource::UpdateLinesActive(const CBitSet & set, int change)
{
	LockGuard guard(m_Lock);
	size_t nBlockStart = 0;

	for(size_t i = 0; i < m_Blocks.size(); i++)
	{
		auto p = m_Blocks[i];

		for(size_t idx = 0; idx < p->Lines.GetCount(); idx++)
		{
			if(set.GetBit(nBlockStart+idx))
			{
				int curActive = p->Lines.GetAt(idx).Active;
				int newActive = curActive + change;

				if(curActive == 0)
				{
					m_nActive++;
				}

				if(newActive == 0)
				{
					m_nActive--;
				}

				p->Lines.GetAt(idx).Active = newActive;
			}
		}

		nBlockStart += p->Lines.GetCount();
	}
}

void CTextTraceSource::GetActiveLinesIndices(std::vector<DWORD> & lines)
{
	LockGuard guard(m_Lock);
	if(m_nActive == 0)
	{
		return;
	}

	lines.resize(m_nActive);
	size_t nBlockStart = 0;
	size_t k = 0;

	for(size_t i = 0; i < m_Blocks.size(); i++)
	{
		auto p = m_Blocks[i];

		for(size_t idx = 0; idx < p->Lines.GetCount(); idx++)
		{
			if(p->Lines.GetAt(idx).Active)
			{
				assert(lines.size() >= k);
				lines[k++] = idx + nBlockStart;
			}
		}

		nBlockStart += p->Lines.GetCount();
	}
}

bool CTextTraceSource::SetTraceFormat(const char * psz)
{
	LockGuard guard(m_Lock);
	LineInfoDesc::Reset(m_Desc);
	m_Parser.reset(new TraceLineParser());
	try
	{
		m_Parser->SetFormat(psz, 0);
	}
	catch(std::invalid_argument&)
	{
		return false;
	}

	// check what we captured
	for(auto it = m_Parser->GetFields().begin(); it != m_Parser->GetFields().end(); ++it)
	{
		if(*it == TraceLineParser::FieldId::Thread)
		{
			m_Desc.Tid = true;
		}
	}

	// reset all parsed bits
	for(auto it = m_Blocks.begin(); it != m_Blocks.end(); it++)
	{
		(*it)->LineParsed.Fill(false);
	}

	return true;
}

// updates view with changes (if any)
HRESULT CTextTraceSource::Refresh()
{
	LockGuard guard(m_Lock);
	// for now do not fire any events
	m_Blocks = m_pFile->m_Blocks;
	m_pLastBlock = nullptr;

	m_nTotal = 0;
	for(auto it = m_Blocks.begin(); it != m_Blocks.end(); it++)
	{
		m_nTotal += (*it)->Lines.GetCount();
	}

	return S_OK;
}
