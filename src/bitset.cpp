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
}


void CBitSet::Copy(CBitSet&& other)
{
	m_Buf = std::move(other.m_Buf);

	m_nBuf = other.m_nBuf;
	m_nSetBit = other.m_nSetBit;
	m_nTotalBit = other.m_nTotalBit;
	m_nFirstBit = other.m_nFirstBit;
	m_nLastBit = other.m_nLastBit;
}


void CBitSet::Resize(DWORD nElems)
{
    m_nTotalBit = nElems;
    m_nSetBit = 0;
    m_nBuf = (nElems >> 5) + 1;
    m_Buf.resize(m_nBuf);
    // ZeroMemory(m_Buf, m_nBuf*sizeof(DWORD));
}

void CBitSet::Fill(BOOL fSet)
{
    if(fSet)
    {
        m_nSetBit = m_nTotalBit;
        FillMemory(m_Buf.data(), m_nBuf*sizeof(DWORD), 0xff);
    }
    else
    {
        m_nSetBit = 0;
        FillMemory(m_Buf.data(), m_nBuf*sizeof(DWORD), 0);
    }
}

inline DWORD GetBitCount(DWORD v)
{
	// from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
	v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
	DWORD c = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
	return c;
}

void CBitSet::Or(const CBitSet& src)
{
	// Or only affects bits which are common in src and dest sets
	// so we can take small optimization
	DWORD srcFirst = src.m_nFirstBit >> 5;
	DWORD srcLast = (src.m_nLastBit >> 5) + 1;

	DWORD nBuf = std::min<DWORD>(m_nBuf, srcLast);
	for(DWORD i = srcFirst; i < nBuf; i++)
	{
		// compute number of unique bits in src
		DWORD v = src.m_Buf[i] ^ (m_Buf[i] & src.m_Buf[i]);
		DWORD c = GetBitCount(v);
		m_nSetBit += c;
		m_Buf[i] |= src.m_Buf[i];
	}

	m_nFirstBit = std::min<DWORD>(m_nFirstBit, src.m_nFirstBit);
	m_nLastBit = std::max<DWORD>(m_nLastBit, src.m_nLastBit);
}

void CBitSet::And(const CBitSet& src)
{
	for (DWORD i = 0; i < m_nBuf; i++)
	{
		// compute number of unique bits in dest (will be erased)
		DWORD v = m_Buf[i] ^ (m_Buf[i] & src.m_Buf[i]);
		DWORD c = GetBitCount(v);
		m_nSetBit -= c;
		m_Buf[i] &= src.m_Buf[i];
	}

	m_nFirstBit = std::max<DWORD>(m_nFirstBit, src.m_nFirstBit);
	m_nLastBit = std::min<DWORD>(m_nLastBit, src.m_nLastBit);
}

DWORD CBitSet::FindNSetBit(DWORD idx)
{
	for (DWORD i = m_nFirstBit; i < m_nLastBit; i++)
	{
		if (GetBit(i))
		{
			if (idx == 0)
			{
				return i;
			}
			else
			{
				idx--;
			}
		}
	}

	return -1;
}

CBitSet CBitSet::Clone()
{
	CBitSet set;
	set.m_Buf.resize(m_nBuf);
	memcpy_s(set.m_Buf.data(), sizeof(DWORD)*m_nBuf, m_Buf.data(), sizeof(DWORD)*m_nBuf);
	set.m_nBuf = m_nBuf;
	set.m_nSetBit = m_nSetBit;
	set.m_nTotalBit = m_nTotalBit;
	set.m_nFirstBit = m_nFirstBit;
	set.m_nLastBit = m_nLastBit;
	return set;
}
