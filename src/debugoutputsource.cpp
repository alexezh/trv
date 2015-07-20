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
#include "debugoutputsource.h"
#include "log.h"

bool CDebugOutputTraceSource::Init()
{
	SECURITY_DESCRIPTOR sd;
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
		return false;

	if (!SetSecurityDescriptorDacl(&sd, true, nullptr, false))
		return false;

	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle = false;
	sa.lpSecurityDescriptor = &sd;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	m_AckEvent = CreateEvent(&sa, false, false, L"DBWIN_BUFFER_READY");
	if (m_AckEvent == nullptr) 
	{
		return false;
	}

	m_ReadyEvent = CreateEvent(&sa, false, false, L"DBWIN_DATA_READY");
	if (m_ReadyEvent == nullptr) 
	{
		return false;
	}

	// Get a handle to the readable shared memory at slot 'DBWIN_BUFFER'.
	m_SharedFile = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, 4096, L"DBWIN_BUFFER");
	if (m_SharedFile == nullptr) 
	{
		return false;
	}

	m_SharedMem = MapViewOfFile(m_SharedFile, SECTION_MAP_READ, 0, 0, 512);
	if (m_SharedMem == nullptr) 
	{
		return false;
	}

	QueueUserWorkItem((LPTHREAD_START_ROUTINE)MonitorThreadInit, this, 0);

	return true;
}

void WINAPI CDebugOutputTraceSource::MonitorThreadInit(void * pCtx)
{
	CDebugOutputTraceSource * pThis = (CDebugOutputTraceSource*)pCtx;
	pThis->MonitorThread();
}

void CDebugOutputTraceSource::MonitorThread()
{
	while (true)
	{
		SetEvent(m_AckEvent);
		DWORD dw = WaitForSingleObject(m_ReadyEvent, 1000);

		if (dw == WAIT_TIMEOUT)
		{
			LockGuard guard(m_Lock);
			auto line = AllocLine("New line");
			line->Info.Index = m_Lines.size();
			m_Lines.push_back(std::move(line));
		}
		else
		{
			assert(false);
			// LPSTR String = (LPSTR)SharedMem + sizeof(DWORD);
			DWORD pid = *(DWORD*)m_SharedMem;
		}
	}
}

std::unique_ptr<CDebugOutputTraceSource::Line> CDebugOutputTraceSource::AllocLine(const char* pszLine)
{
	size_t cchLine = strlen(pszLine);
	std::unique_ptr<CDebugOutputTraceSource::Line> line(reinterpret_cast<Line*>(new uint8_t[sizeof(Line) + cchLine - 1]));
	line->Info.Active = 0;
	line->Info.Content.psz = line->Data;
	line->Info.Content.cch = cchLine;
	line->Info.Index = 0;
	memcpy(line->Data, pszLine, cchLine);
	return line;
}

LineInfo& CDebugOutputTraceSource::GetLine(DWORD nIndex)
{
	LockGuard guard(m_Lock);
	if(nIndex >= m_Lines.size())
	{
		static LineInfo line;
		return line;
	}
	return m_Lines[nIndex]->Info;
}

void CDebugOutputTraceSource::UpdateLineActive(DWORD idx, int change)
{
	LockGuard guard(m_Lock);
	auto& line = m_Lines[idx];

	int curActive = line->Info.Active;
	int newActive = curActive + change;

	if (curActive == 0)
	{
		m_nActive++;
	}

	if (newActive == 0)
	{
		m_nActive--;
	}

	line->Info.Active = newActive;
}

void CDebugOutputTraceSource::UpdateLinesActive(const CBitSet & set, int change)
{
	LockGuard guard(m_Lock);
	for(auto& line : m_Lines)
	{
		if(set.GetBit(line->Info.Index))
		{
			int curActive = line->Info.Active;
			int newActive = curActive + change;

			if(curActive == 0)
			{
				m_nActive++;
			}

			if(newActive == 0)
			{
				m_nActive--;
			}

			line->Info.Active = newActive;
		}
	}
}

void CDebugOutputTraceSource::GetActiveLinesIndices(std::vector<DWORD> & lines)
{
	LockGuard guard(m_Lock);
	if(m_nActive == 0)
	{
		return;
	}

	lines.resize(m_nActive);
	size_t nBlockStart = 0;
	size_t k = 0;

	for(auto& line : m_Lines)
	{
		if(line->Info.Active)
		{
			assert(lines.size() >= k);
			lines[k++] = line->Info.Index;
		}
	}
}

bool CDebugOutputTraceSource::SetTraceFormat(const char * psz)
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

	return true;
}

// updates view with changes (if any)
HRESULT CDebugOutputTraceSource::Refresh()
{
	LockGuard guard(m_Lock);

	m_nTotal = m_Lines.size();
	// for now do not fire any events

	return S_OK;
}
