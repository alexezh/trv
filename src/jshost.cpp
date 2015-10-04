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
#include "traceapp.h"
#include "outputview.h"
#include "traceview.h"
#include "commandview.h"
#include "jshost.h"
#include "make_unique.h"
#include "color.h"
#include "js/viewproxy.h"
#include "js/history.h"
#include "js/dotexpressions.h"
#include "js/shortcuts.h"
#include "js/tagger.h"
#include "js/trace.h"
#include "stringutils.h"
#include <include/libplatform/libplatform.h>

using namespace v8;

///////////////////////////////////////////////////////////////////////////////
//
void JsHost::Init(const std::shared_ptr<CTraceSource>& pColl)
{
	// TODO: add code to queue copy to another thread
	_pFileTraceSource = pColl;
	// _pApp->PTraceView()->SetTraceSource(pColl);

	_hSem = CreateSemaphore(NULL, 0, 0x7fff, NULL);
	InitializeCriticalSection(&_cs);

	// run everything on separate thread
	QueueUserWorkItem((LPTHREAD_START_ROUTINE)ScriptThreadInit, this, 0);
}

void JsHost::OnViewCreated(Js::View* view)
{
	LOG("@%p view=%p", this, view)
	_pView = view;
}

void JsHost::OnHistoryCreated(Js::History* history)
{
	_pHistory = history;
	_pHistory->Load();
}

void JsHost::OnDotExpressionsCreated(Js::DotExpressions* de)
{
	LOG("@%p de=%p", this, de)
	_pDotExpressions = de;
}

void JsHost::OnShortcutsCreated(Js::Shortcuts* obj)
{
	LOG("@%p obj=%p", this, obj)
	_pShortcuts = obj;
}

void JsHost::OnTaggerCreated(Js::Tagger* obj)
{
	_pTagger = obj;
}

///////////////////////////////////////////////////////////////////////////////
//
void WINAPI JsHost::ScriptThreadInit(void * pCtx)
{
	JsHost * pHost = (JsHost*)pCtx;
	pHost->ScriptThread();
}

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator 
{
public:
	virtual void* Allocate(size_t length) 
	{
		void* data = AllocateUninitialized(length);
		return data == NULL ? data : memset(data, 0, length);
	}
	virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
	virtual void Free(void* data, size_t) { free(data); }
};

void JsHost::ScriptThread()
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	v8::V8::InitializeICU();
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();

	ArrayBufferAllocator array_buffer_allocator;
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = &array_buffer_allocator;
	v8::Isolate* isolate = v8::Isolate::New(create_params);

	Isolate::Scope isoScope(isolate);

	// init path info
	m_AppDataPath = GetKnownPath(FOLDERID_LocalAppData);

	// now run
	HandleScope handleScope(isolate);

	// Create a template for the global object.
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
	_Global.Reset(isolate, global);

	// Create a new execution environment containing the built-in
	// functions
	v8::Local<v8::Context> context = Context::New(isolate, NULL, global);
	_Context.Reset(isolate, context);

	Context::Scope contextScope(context);

	context->SetEmbedderData(1, External::New(isolate, static_cast<Js::IAppHost*>(this)));

	// Enter the newly created execution environment.
	Js::InitRuntimeTemplate(isolate, global);

	{
		// Compile script in try/catch context.
		TryCatch trycatch;
		if(!Js::InitRuntime(isolate, context->Global()))
		{
			ReportException(isolate, trycatch);
			return;
		}
	}

	for(;;)
	{
		WaitForSingleObject(_hSem, INFINITE);
		std::function<void(Isolate*)> item;

		{
			AutoCS lock(_cs);
			item.swap(_InputQueue.front());
			_InputQueue.pop();
		}

		item(isolate);

		for(;;)
		{
			{
				AutoCS lock(_cs);
				if(!_InputQueue.empty())
				{
					break;
				}
			}
			if(isolate->IdleNotificationDeadline(0.1))
			{
				break;
			}
		}
	}

	CoUninitialize();
}

void JsHost::ReportException(Isolate* isolate, TryCatch& trycatch)
{
	std::stringstream ss;

	ss << "Exception in\r\n";

	HandleScope handleScope(isolate);
	String::Utf8Value exception(trycatch.Exception());
	const char * strException = *exception;
	Handle<Message> message = trycatch.Message();
	if (message.IsEmpty()) 
	{
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		if(strException)
		{
			ss << strException;
		}
	} 
	else 
	{
		// Print (filename):(line number): (message).
		String::Utf8Value filename(message->GetScriptResourceName());
		ss << *filename << ":" << message->GetLineNumber() << ": ";
		if(strException)
		{
			ss << strException;
		}
		ss << "\r\n";

		// Print line of source code.
		String::Utf8Value sourceline(message->GetSourceLine());
		ss << *sourceline << "\r\n";

#if 0
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			fprintf(stderr, " ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			fprintf(stderr, "^");
		}
		fprintf(stderr, "\n");
#endif

		String::Utf8Value stackTrace(trycatch.StackTrace());
		if (stackTrace.length() > 0) 
		{
			ss << *stackTrace << "\r\n";
		}
	}

	OutputLine(ss.str().c_str());
}

void JsHost::QueueInput(std::function<void(Isolate* iso)> && item)
{
	{
		AutoCS lock(_cs);
		_InputQueue.push(std::move(item));
	}

	ReleaseSemaphore(_hSem, 1, NULL);
}

void JsHost::ProcessInputLine(const char * pszLine)
{
	std::string line(pszLine);
	if (line.length() == 0)
	{
		LOG("@%p empty input");
		return;
	}

	QueueInput([this, line](Isolate* iso)
	{
		ExecuteString(iso, line);
	});
}

void JsHost::ProcessAccelerator(uint8_t modifier, uint16_t key)
{
	QueueInput([this, modifier, key](Isolate* iso)
	{
		if (_pShortcuts)
			_pShortcuts->Execute(iso, modifier, key);
	});
}

size_t JsHost::GetHistoryCount()
{
	return _pHistory->GetCount();
}

bool JsHost::GetHistoryEntry(size_t idx, std::string& entry)
{
	return _pHistory->GetEntry(idx, entry);
}

BYTE JsHost::GetLineColor(DWORD dwLine)
{
	if(!_pView)
	{
		return 0;
	}

	return _pTagger->GetLineColor(dwLine);
}

void JsHost::AddShortcut(uint8_t modifier, uint16_t key)
{
	_pApp->Post([this, modifier, key]()
	{
		_pApp->AddShortcut(modifier, key);
	});
}

void JsHost::ConsoleSetConsole(const std::string& szText)
{
	_pApp->Post([this, szText]()
	{
		_pApp->PCommandView()->SetText(szText);
	});
}

void JsHost::ConsoleSetFocus()
{
	_pApp->Post([this]()
	{
		_pApp->PCommandView()->SetFocus();
	});
}

void JsHost::ExecuteString(Isolate* iso, const std::string & line)
{
	// echo to output
	OutputLine(line.c_str());
	// save in history
	_pHistory->Append(line.c_str());

	auto idxNonWhite = line.find_first_not_of(" \t");
	if (idxNonWhite == std::string::npos)
	{
		LOG("@%p all white spaces", this);
		return;
	}

	if (line[idxNonWhite] == '.')
	{
		ExecuteStringAsDotExpression(iso, line);
	}
	else
	{
		ExecuteStringAsScript(iso, line);
	}
}

void JsHost::ExecuteStringAsDotExpression(Isolate* iso, const std::string & line)
{
	if (_pDotExpressions == nullptr)
	{
		assert(false);
		return;
	}

	_pDotExpressions->Execute(iso, line);
}

void JsHost::ExecuteStringAsScript(Isolate* iso, const std::string & line)
{
	Handle<Script> script;
	HandleScope handleScope(iso);

	auto scriptSource = String::NewFromUtf8(iso, line.c_str());
	auto scriptName = String::NewFromUtf8(iso, "unnamed");

	{
		// Compile script in try/catch context.
		TryCatch trycatch;
		script = Script::Compile(scriptSource, scriptName);
		if (script.IsEmpty())
		{
			ReportException(iso, trycatch);
			return;
		}
	}

	{
		TryCatch trycatch;

		script->Run();
		if (trycatch.HasCaught())
		{
			ReportException(iso, trycatch);
			return;
		}
	}
}

// console access
void JsHost::OutputLine(const char * psz)
{
	// output view expects call on app thread 
	std::string sz(psz);

	_pApp->Post([this, sz]() 
	{
		_pApp->POutputView()->OutputLineA(sz.c_str());
	});
}

void JsHost::SetViewLayout(double cmdHeight, double outHeight)
{
	_pApp->Post([this, cmdHeight, outHeight]() 
	{
		_pApp->SetDockLayout(cmdHeight, outHeight);
	});
}

void JsHost::SetColumns(const std::vector<std::string>& names)
{
	std::vector<ColumnId> ids;

	for (auto& name : names)
	{
		ColumnId id;

		if (_stricmp(name.c_str(), "line") == 0)
		{
			id = ColumnId::LineNumber;
		}
		else if (_stricmp(name.c_str(), "tid") == 0)
		{
			id = ColumnId::ThreadId;
		}
		else if (_stricmp(name.c_str(), "time") == 0)
		{
			id = ColumnId::Time;
		}
		else if (_stricmp(name.c_str(), "message") == 0)
		{
			id = ColumnId::Message;
		}
		else if (_stricmp(name.c_str(), "user1") == 0)
		{
			id = ColumnId::User1;
		}
		else if (_stricmp(name.c_str(), "user2") == 0)
		{
			id = ColumnId::User2;
		}
		else if (_stricmp(name.c_str(), "user3") == 0)
		{
			id = ColumnId::User3;
		}
		else if (_stricmp(name.c_str(), "user4") == 0)
		{
			id = ColumnId::User4;
		}
		else
		{
			assert(false);
			continue;
		}

		ids.push_back(id);
	}

	_pApp->Post([this, ids]()
	{
		_pApp->SetTraceColumns(ids);
	});
}

void JsHost::LoadTrace(const char* pszName, int startPos, int endPos)
{
	std::string name(pszName);
	_pApp->Post([this, name, startPos, endPos]() 
	{
		_pApp->LoadFile(name, startPos, endPos);
	});
}

std::shared_ptr<CTraceSource> JsHost::GetFileTraceSource()
{
	return _pFileTraceSource;
}

void JsHost::SetViewSource(const std::shared_ptr<CBitSet>& lines)
{
	_pApp->Post([this, lines]()
	{
		_pApp->PTraceView()->SetViewSource(lines);
	});
}

void JsHost::SetFocusLine(DWORD nLine)
{
	_pApp->Post([this, nLine]()
	{
		_pApp->PTraceView()->SetFocusLine(nLine);
	});
}

const LineInfo& JsHost::GetLine(size_t idx)
{
	if(!_pFileTraceSource)
	{
		static LineInfo line;
		return line;
	}

	return _pFileTraceSource->GetLine(idx);
}

size_t JsHost::GetLineCount()
{
	if(!_pFileTraceSource)
	{
		return CColor::DEFAULT_TEXT;
	}

	return _pFileTraceSource->GetLineCount();
}

size_t JsHost::GetCurrentLine()
{
	if (!_pApp)
	{
		return 0;
	}

	return _pApp->PTraceView()->GetFocusLine();
}

void JsHost::RefreshView()
{
	_pApp->Post([this]() 
	{
		_pApp->PTraceView()->Repaint();
	});
}

void JsHost::RequestViewLine()
{
	QueueInput([this](Isolate* iso)
	{
		if (!m_RequestLineHandler)
			return;

		DWORD idx = _pApp->PTraceView()->GetLineCache()->GetNextRequestedLine();
		if (idx == ViewLineCache::NoLine)
			return;

		auto line = m_RequestLineHandler(iso, idx);
		_pApp->PTraceView()->GetLineCache()->SetLine(idx, std::move(line));
	});
}

void JsHost::RegisterRequestLineHandler(const std::function<std::unique_ptr<ViewLine>(v8::Isolate*, DWORD idx)>& handler)
{
	m_RequestLineHandler = handler;
}

bool JsHost::SetTraceFormat(const char * pszFormat, const char* pszSep)
{
	// TODO: call can happen before file is loaded
	if(!_pFileTraceSource->SetTraceFormat(pszFormat, pszSep))
	{
		return false;
	}

	_pApp->Post([this]()
	{
		_pApp->PTraceView()->Repaint();
	});

	return true;
}

std::string JsHost::GetKnownPath(REFKNOWNFOLDERID id)
{
	CComPtr<IKnownFolderManager> kfm;
	CComPtr<IKnownFolder> kf;

	HRESULT hr = CoCreateInstance(CLSID_KnownFolderManager,
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IKnownFolderManager),
		(void**)&kfm);
	if (FAILED(hr))
	{
		assert(false);
		return false;
	}
	hr = kfm->GetFolder(id, &kf);
	if (FAILED(hr))
	{
		assert(false);
		return false;
	}
	WCHAR* pszPath;
	hr = kf->GetPath(KF_FLAG_DEFAULT_PATH, &pszPath);
	if (FAILED(hr))
	{
		assert(false);
		return false;
	}

	std::string path;
	WszToString(pszPath, path);
	CoTaskMemFree(pszPath);

	path += "\\trv.js";

	// make sure that folder exists
	if (!CreateDirectoryA(path.c_str(), NULL))
	{
		DWORD err = GetLastError();
		if (err != ERROR_ALREADY_EXISTS)
		{
			assert(false);
		}
	}

	return path;
}
