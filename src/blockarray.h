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
// array optimized a little bit for append from end
// manages array or arrays of elements
template <class T>
class CBlockArray
{
private:
    struct BLOCK
    {
        T v[1];
    };
    
public:
    CBlockArray(DWORD nItemsPerBlock)
        : m_nItems(0)
        , m_nItemsPerBlock(nItemsPerBlock)
    {;
    }

    void Clear()
    {
        DWORD nBlocks = m_nItems / m_nItemsPerBlock;
        DWORD n;
        
        for(n = 0; n < nBlocks; n++)
        {
            delete m_pBlocks[n];
        }

        delete m_pBlocks;
        m_nBlocksUsed = m_nBlocksTotal = 0;
        m_nItems = 0;
    }
    
    HRESULT Add(const T & Elem)
    {
        if(m_nItems == (m_Blocks.size() * m_nItemsPerBlock))
        {
            // allocate new block
            BLOCK * pNewBlock = new BLOCK[m_nItemsPerBlock];

            if(pNewBlock == NULL)
            {
                return E_OUTOFMEMORY;
            }

            m_Blocks.push_back(pNewBlock);
        }

        BLOCK * pBlock = m_Blocks[m_nItems / m_nItemsPerBlock];

        pBlock->v[m_nItems % m_nItemsPerBlock] = Elem;
        m_nItems++;
        
        return S_OK;
    }
    
    T & GetAt(DWORD nIndex)
    {
        if(nIndex >= m_nItems)
        {
			throw std::invalid_argument("invalid line index");
        }

        BLOCK * pBlock = m_Blocks[nIndex / m_nItemsPerBlock];
        
        return pBlock->v[nIndex % m_nItemsPerBlock];
    }
    
    DWORD GetCount() { return m_nItems; }
private:

    DWORD m_nItemsPerBlock;
    DWORD m_nItems;
    std::vector<BLOCK*> m_Blocks;
};

