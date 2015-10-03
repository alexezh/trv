#pragma once

namespace v8 {
class Utf8Value;
}

class ViewLine
{
public:
	void SetLineIndex(DWORD line);
	DWORD GetLineIndex() const
	{
		return m_LineIndex;
	}
	void SetTime(v8::Utf8Value& val);
	CStringRef GetTime() const
	{
		return m_Time;
	}
	void SetMsg(v8::Utf8Value& val);
	CStringRef GetMsg() const
	{
		return m_Msg;
	}
	void SetThreadId(v8::Utf8Value& val);
	CStringRef GetThreadId() const
	{
		return m_ThreadId;
	}
	void SetUser1(v8::Utf8Value& val);
	CStringRef GetUser1() const
	{
		return m_User1;
	}
	void SetUser2(v8::Utf8Value& val);
	CStringRef GetUser2() const
	{
		return m_User2;
	}
	void SetUser3(v8::Utf8Value& val);
	CStringRef GetUser3() const
	{
		return m_User3;
	}
	void SetUser4(v8::Utf8Value& val);
	CStringRef GetUser4() const
	{
		return m_User4;
	}

private:
	DWORD m_LineIndex;
	CStringRef m_Time;
	CStringRef m_ThreadId;
	CStringRef m_Msg;
	CStringRef m_User1;
	CStringRef m_User2;
	CStringRef m_User3;
	CStringRef m_User4;
	std::vector<char> m_Data;
};

class ViewLineCache
{
public:

private:

};
