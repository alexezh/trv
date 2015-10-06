#pragma once

#include <array>
#include "blockarray.h"
#include "dispatchqueue.h"

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
	using LineRequestedHandler = std::function<void(DWORD idx)>;

	ViewLineCache(IDispatchQueue* uiQueue, Js::IAppHost* host);

	std::pair<bool, const ViewLine&> GetLine(DWORD idx);

	bool HaveLineRequests()
	{
		std::lock_guard<std::mutex> guard(m_Lock);
		return m_RequestedLines.size() != 0;
	}

	static const DWORD NoLine = static_cast<DWORD>(-1);
	DWORD GetNextRequestedLine()
	{
		std::lock_guard<std::mutex> guard(m_Lock);
		if (m_RequestedLines.size() == 0)
			return NoLine;

		DWORD idx = m_RequestedLines.back();
		m_RequestedLines.pop_back();
		return idx;
	}
	void SetLine(DWORD idx, ViewLine&& line);
	void SetLine(DWORD idx, std::unique_ptr<ViewLine>&& line);

	void StartGeneration();

	void RegisterLineAvailableListener(const LiveAvailableHandler& handler);
	void Resize(size_t n);
private:
	std::mutex m_Lock;

	std::vector<DWORD> m_RequestedLines;
	Js::IAppHost* m_Host;
	IDispatchQueue* m_UiQueue;

	CSparseBlockArray<std::unique_ptr<ViewLine>, 1024 * 32> m_Cache;
	LiveAvailableHandler m_OnLineAvailable;
};
