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
// block array with sparse support
template <class T, size_t itemsPerBlock>
class CSparseBlockArray
{
private:
	struct BLOCK
	{
		size_t nItems = 0;
		T Items[itemsPerBlock];

		void Set(size_t idx, const T& val)
		{
			Items[idx] = val;
			nItems++;
		}
		void Reset(size_t idx)
		{
			Items[idx] = T();
			nItems--;
		}
	};

public:
	CSparseBlockArray()
	{
	}

	void Clear()
	{
		m_pBlocks.erase();
		m_nBlocksUsed = m_nBlocksTotal = 0;
		m_nSize = 0;
		m_nItems = 0;
	}

	void Add(const T & Elem)
	{
		if (m_nSize == (m_Blocks.size() * itemsPerBlock))
		{
			// allocate new block
			std::unique_ptr<BLOCK> pNewBlock(new BLOCK);
			m_Blocks.push_back(std::move(pNewBlock));
		}

		auto& pBlock = GetBlock(m_nSize);

		pBlock->Set(m_nSize % itemsPerBlock, Elem);
		m_nItems++;
		m_nSize++;
	}
	 
	// resets item to default value
	void Reset(size_t index)
	{
		auto& pBlock = GetBlocks(index);
		pBlock->Reset(index % itemsPerBlock);
		m_nItems--;
	}

	T & GetAt(size_t nIndex)
	{
		if (nIndex >= m_nItems)
		{
			throw std::invalid_argument("invalid line index");
		}

		auto& pBlock = GetBlock(nIndex);

		return pBlock->Items[nIndex % itemsPerBlock];
	}

	DWORD GetCount()
	{
		return m_nItems;
	}
private:
	std::unique_ptr<BLOCK>& GetBlock(size_t idx)
	{
		return m_Blocks[idx / itemsPerBlock];
	}

	// total size of array
	size_t m_nSize { 0 };
	size_t m_nItems { 0 };
	std::vector<std::unique_ptr<BLOCK>> m_Blocks;
};

