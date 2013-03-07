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
    ~CBitSet();
    
    void Init(DWORD nElems);
	void CopyFrom(const CBitSet& src);
    void Fill(BOOL fSet);
    
    // returns set size in DWORDs
    DWORD GetBufSize() { return m_nBuf; } 
    DWORD * GetBuf() { return m_pBuf; }

    DWORD GetSetBitCount() { return m_nSetBit; }
    DWORD GetTotalBitCount() { return m_nTotalBit; }

	void Or(CBitSet& src);
    
    void SetBit(DWORD nBit)
    {
        DWORD n = nBit >> 5;
        m_pBuf[n] |= (1 << (nBit & 0x1f));
        m_nSetBit++;
    }

    bool GetBit(DWORD nBit)
    {
		assert(nBit <= m_nTotalBit);
        DWORD n = nBit >> 5;
        return ((m_pBuf[n] >> (nBit & 0x1f)) & 1);
    }
    
private:    
    DWORD * m_pBuf;
    DWORD m_nBuf; // number of DWORDs in the mask
    DWORD m_nSetBit;
    DWORD m_nTotalBit;
};
