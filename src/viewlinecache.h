#pragma once

#include <array>
#include "blockarray.h"
#include "dispatchqueue.h"
#include "bitset.h"

namespace v8 {
class Utf8Value;
}

namespace Js {
class IAppHost;
}

class ViewLine
{
public:
	ViewLine()
	{

	}

	ViewLine(ViewLine&& other)
		: m_LineIndex(other.m_LineIndex)
		, m_Time(std::move(other.m_Time))
		, m_ThreadId(other.m_ThreadId)
		, m_Msg(std::move(other.m_Msg))
		, m_User(std::move(other.m_User))
	{
	}

	ViewLine(const ViewLine& other)
		: m_LineIndex(other.m_LineIndex)
		, m_Time(other.m_Time)
		, m_ThreadId(other.m_ThreadId)
		, m_Msg(other.m_Msg)
		, m_User(other.m_User)
	{
	}

	ViewLine& operator=(ViewLine&& other)
	{
		if (this == &other)
			return *this;

		m_LineIndex = other.m_LineIndex;
		m_Time = std::move(other.m_Time);
		m_ThreadId = other.m_ThreadId;
		m_Msg = std::move(other.m_Msg);
		m_User = std::move(other.m_User);
			
		return *this;
	}

	void SetLineIndex(DWORD line)
	{
		m_LineIndex = line;
	}
	DWORD GetLineIndex() const
	{
		return m_LineIndex;
	}
	void SetTime(std::string&& val)
	{
		m_Time = std::move(val);
	}
	const std::string& GetTime() const
	{
		return m_Time;
	}
	void SetMsg(std::string&& val)
	{
		m_Msg = std::move(val);
	}
	const std::string& GetMsg() const
	{
		return m_Msg;
	}
	void SetThreadId(DWORD val)
	{
		m_ThreadId = val;
	}
	DWORD GetThreadId() const
	{
		return m_ThreadId;
	}
	void SetUser(size_t idx, std::string&& val)
	{
		m_User[idx] = std::move(val);
	}
	const std::string& GetUser(size_t idx) const
	{
		return m_User[idx];
	}

private:
	DWORD m_LineIndex;
	DWORD m_ThreadId;
	std::string m_Time;
	std::string m_Msg;
	std::array<std::string, 4> m_User;
};

class ViewLineCache
{
public:
	using LiveAvailableHandler = std::function<void(DWORD idx)>;

	ViewLineCache(IDispatchQueue* uiQueue, Js::IAppHost* host);

	const ViewLine* GetLine(DWORD idx);

	bool HaveLineRequests()
	{
		std::lock_guard<std::mutex> guard(m_Lock);
		return m_RequestedLines.size() != 0;
	}

	bool ProcessNextLine(const std::function<std::unique_ptr<ViewLine>(DWORD)>& func);

	void StartGeneration();

	void RegisterLineAvailableListener(const LiveAvailableHandler& handler);
	void Resize(size_t n);
private:
	std::mutex m_Lock;

	// list of lines requested and map indicating if line was requested
	std::vector<DWORD> m_RequestedLines;
	std::set<DWORD> m_RequestedMap;

	Js::IAppHost* m_Host;
	IDispatchQueue* m_UiQueue;

	CSparseBlockArray<std::unique_ptr<ViewLine>, 1024 * 32> m_Cache;
	LiveAvailableHandler m_OnLineAvailable;
};
