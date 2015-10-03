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
template <class T, size_t itemsPerBlock>
class CBlockArray
{
private:
	struct BLOCK
	{
		size_t nItems = 0;
		T v[itemsPerBlock];
	};

public:
	CBlockArray()
	{
	}

	void Clear()
	{
		DWORD nBlocks = m_nItems / itemsPerBlock ;
		DWORD n;

		m_pBlocks.erase();
		m_nBlocksUsed = m_nBlocksTotal = 0;
		m_nItems = 0;
	}

	void Add(const T & Elem)
	{
		if (m_nItems == (m_Blocks.size() * m_nItemsPerBlock))
		{
			// allocate new block
			std::unique_ptr<BLOCK> pNewBlock(new BLOCK);
			m_Blocks.push_back(std::move(pNewBlock));
		}

		auto& pBlock = m_Blocks[m_nItems / itemsPerBlock];

		pBlock->v[m_nItems % m_nItemsPerBlock] = Elem;
		pBlock->nItems++;
		m_nItems++;
	}
	 
	void Erase(size_t index)
	{

	}

	T & GetAt(size_t nIndex)
	{
		if (nIndex >= m_nItems)
		{
			throw std::invalid_argument("invalid line index");
		}

		auto& pBlock = m_Blocks[nIndex / itemsPerBlock];

		return pBlock->v[nIndex % itemsPerBlock];
	}

	DWORD GetCount()
	{
		return m_nItems;
	}
private:
	size_t m_nItems { 0 };
	std::vector<std::unique_ptr<BLOCK>> m_Blocks;
};

