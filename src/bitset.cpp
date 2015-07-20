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
#include "bitset.h"

///////////////////////////////////////////////////////////////////////////////
//
CBitSet::CBitSet()
{
}

CBitSet::~CBitSet()
{
    if(m_pBuf)
    {
        delete m_pBuf;
    }
}


void CBitSet::Copy(CBitSet&& other)
{
	m_pBuf = other.m_pBuf;
	other.m_pBuf = nullptr;

	m_nBuf = other.m_nBuf;
	m_nSetBit = other.m_nSetBit;
	m_nTotalBit = other.m_nTotalBit;
	m_nFirstBit = other.m_nFirstBit;
	m_nLastBit = other.m_nLastBit;
}


void CBitSet::Init(DWORD nElems)
{
    if(m_pBuf)
    {
        delete m_pBuf;
        m_pBuf = NULL;
    }
    
    m_nTotalBit = nElems;
    m_nSetBit = 0;
    m_nBuf = (nElems >> 5) + 1;
    m_pBuf = new DWORD[m_nBuf];
    ZeroMemory(m_pBuf, m_nBuf*sizeof(DWORD));
}

void CBitSet::Fill(BOOL fSet)
{
    if(fSet)
    {
        m_nSetBit = m_nTotalBit;
        FillMemory(m_pBuf, m_nBuf*sizeof(DWORD), 0xff);
    }
    else
    {
        m_nSetBit = 0;
        FillMemory(m_pBuf, m_nBuf*sizeof(DWORD), 0);
    }
}

void CBitSet::Or(CBitSet& src)
{
	DWORD srcFirst = src.m_nFirstBit >> 5;
	DWORD srcLast = src.m_nLastBit >> 5;

	DWORD nBuf = std::min<DWORD>(m_nBuf, srcLast);
	for(DWORD i = src.m_nFirstBit; i < nBuf; i++)
	{
		m_pBuf[i] |= src.m_pBuf[i];
	}
}

CBitSet CBitSet::Clone()
{
	CBitSet set;
	set.m_pBuf = new DWORD[m_nBuf];
	memcpy_s(set.m_pBuf, sizeof(DWORD)*m_nBuf, m_pBuf, sizeof(DWORD)*m_nBuf);
	set.m_nBuf = m_nBuf;
	set.m_nSetBit = m_nSetBit;
	set.m_nTotalBit = m_nTotalBit;
	set.m_nFirstBit = m_nFirstBit;
	set.m_nLastBit = m_nLastBit;
	return set;
}
