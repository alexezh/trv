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

///////////////////////////////////////////////////////////////////////////////
//
class CBitSet
{
public:
	CBitSet();
	CBitSet(CBitSet&& other)
	{
		Copy(std::move(other));
	}
	~CBitSet();

	CBitSet& operator=(CBitSet&& other)
	{
		if (&other == this)
			return *this;

		Copy(std::move(other));
		return *this;
	}

	void Init(DWORD nElems);
	void Fill(BOOL fSet);

	DWORD GetSetBitCount() const
	{
		return m_nSetBit;
	}
	DWORD GetTotalBitCount() const
	{
		return m_nTotalBit;
	}

	void Or(const CBitSet& src);
	void And(const CBitSet& src);

	void SetBit(DWORD nBit)
	{
		DWORD n = nBit >> 5;
		m_pBuf[n] |= (1 << (nBit & 0x1f));
		m_nSetBit++;
		m_nFirstBit = std::min<DWORD>(m_nFirstBit, nBit);
		m_nLastBit = std::max<DWORD>(m_nLastBit, nBit);
	}

	void ResetBit(DWORD nBit)
	{
		DWORD n = nBit >> 5;
		m_pBuf[n] &= ~(1 << (nBit & 0x1f));
		m_nSetBit--;
	}

	bool GetBit(DWORD nBit) const
	{
		assert(nBit <= m_nTotalBit);
		DWORD n = nBit >> 5;
		return ((m_pBuf[n] >> (nBit & 0x1f)) & 1);
	}

	DWORD FindNSetBit(DWORD idx);

	CBitSet Clone();

private:
	void Copy(CBitSet&& other);

	// TODO: allocate with VirtualAllow but do not commit pages
	DWORD * m_pBuf = nullptr;
	DWORD m_nBuf = 0; // number of DWORDs in the mask
	DWORD m_nSetBit = 0;
	DWORD m_nTotalBit = 0;

	// for optimization store first and last set bits
	// when bit is reset, the value is not changed
	DWORD m_nFirstBit = 0xffffffff;
	DWORD m_nLastBit = 0;
};

class CSparseBitSet
{
private:
	CBitSet m_Set;
};
