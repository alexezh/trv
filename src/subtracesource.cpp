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
#include "subtracesource.h"
#include "log.h"

SubTraceSource::SubTraceSource(const std::shared_ptr<CTraceSource>& src, const CBitSet& set)
	: m_Source(src)
{
	m_Lines.resize(set.GetSetBitCount());
	DWORD idxLine = 0;
	for (DWORD i = 0; i < set.GetTotalBitCount(); i++)
	{
		if (set.GetBit(i))
		{
			m_Lines[idxLine++] = i;
		}
	}
}

// Returns the current line count. 
// The count can change as we add more data at the end or in the beginning
DWORD SubTraceSource::GetLineCount()
{
	return m_Lines.size();
}

const LineInfoDesc& SubTraceSource::GetDesc()
{
	return m_Source->GetDesc();
}

const LineInfo& SubTraceSource::GetLine(DWORD nIndex)
{
	if (nIndex >= m_Lines.size())
	{
		static LineInfo line;
		return line;
	}

	return m_Source->GetLine(m_Lines[nIndex]);
}

bool SubTraceSource::SetTraceFormat(const char * psz)
{
	// interesting idea. We first filter objects and then replace format
	assert(false);
	return false;
}

// updates view with changes (if any)
HRESULT SubTraceSource::Refresh()
{
	return S_OK;
}

void SubTraceSource::SetHandler(CTraceViewNotificationHandler * pHandler)
{

}