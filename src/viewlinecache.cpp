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

#include <strsafe.h>
#include "traceapp.h"
#include "viewlinecache.h"
#include "js/apphost.h"

ViewLineCache::ViewLineCache(IDispatchQueue* uiQueue, Js::IAppHost* host)
{
	m_UiQueue = uiQueue;
	m_Host = host;
}

void ViewLineCache::SetCacheRange(DWORD dwStart, DWORD dwEnd)
{
	if (m_Cache.GetItemCount() < 2000)
		return;

	m_Cache.ResetIfNot(dwStart, dwEnd);
}

bool ViewLineCache::ProcessNextLine(const std::function<std::unique_ptr<ViewLine>(DWORD)>& func)
{
	DWORD idx;
	{
		std::lock_guard<std::mutex> guard(m_Lock);
		if (m_RequestedLines.size() == 0)
			return false;

		idx = m_RequestedLines.back();
		auto& line = func(idx);
		m_Cache.Set(idx, std::move(line));

		// remove line from list and map
		m_RequestedLines.pop_back();
		m_RequestedMap.erase(idx);
	}

	m_UiQueue->Post([this, idx]()
	{
		if (!m_OnLineAvailable)
			return;

		m_OnLineAvailable(idx);
	});

	return true;
}

const ViewLine* ViewLineCache::GetLine(DWORD idx)
{
	std::lock_guard<std::mutex> guard(m_Lock);
	const ViewLine* line = nullptr;
	if(idx < m_Cache.GetSize())
		line = m_Cache.GetAt(idx).get();

	if (line == nullptr && m_RequestedMap.find(idx) == m_RequestedMap.end())
	{
		m_RequestedLines.push_back(idx);
		m_RequestedMap.insert(idx);

		m_Host->RequestViewLine();

	}

	return line;
}

void ViewLineCache::RegisterLineAvailableListener(const LiveAvailableHandler& handler)
{
	m_OnLineAvailable = handler;
}

void ViewLineCache::Resize(size_t n)
{
	m_Cache.Resize(n);
}
