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
#include "jshost.h"
#include "make_unique.h"
#include "color.h"
#include "js/view.h"
#include "js/history.h"

using namespace v8;

///////////////////////////////////////////////////////////////////////////////
//
void JsHost::Init(CTraceCollection * pColl)
{
	// TODO: add code to queue copy to another thread
	_pTraceColl = pColl;

	_hSem = CreateSemaphore(NULL, 0, 0x7fff, NULL);
	InitializeCriticalSection(&_cs);

	// run everything on separate thread
	QueueUserWorkItem((LPTHREAD_START_ROUTINE)ScriptThreadInit, this, 0);
}

void JsHost::OnViewCreated(Js::View* view)
{
	_pView = view;
}

void JsHost::OnHistoryCreated(Js::History* history)
{
	_pHistory = history;
	_pHistory->Load();
}

///////////////////////////////////////////////////////////////////////////////
//
void WINAPI JsHost::ScriptThreadInit(void * pCtx)
{
	JsHost * pHost = (JsHost*)pCtx;
	pHost->ScriptThread();
}

void JsHost::ScriptThread()
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	V8::Initialize();

	// now run
	HandleScope handleScope;

	// Create a template for the global object.
	_Global = Persistent<ObjectTemplate>::New(ObjectTemplate::New());

	// Create a new execution environment containing the built-in
	// functions
	_Context = Context::New(NULL, _Global);
	Context::Scope contextScope(_Context);

	_Context->SetEmbedderData(1, External::New(static_cast<Js::IAppHost*>(this)));

	// Enter the newly created execution environment.
	Js::InitRuntimeTemplate(_Global);

	{
		// Compile script in try/catch context.
		TryCatch trycatch;
		if(!Js::InitRuntime(_Context->Global()))
		{
			ReportException(trycatch);
			return;
		}
	}

	for(;;)
	{
		WaitForSingleObject(_hSem, INFINITE);
		std::unique_ptr<std::function<void()> > item;

		{
			AutoCS lock(_cs);
			item.swap(_InputQueue.front());
			_InputQueue.pop();
		}

		(*item)();

		for(;;)
		{
			{
				AutoCS lock(_cs);
				if(!_InputQueue.empty())
				{
					break;
				}
			}
			if(V8::IdleNotification(100))
			{
				break;
			}
		}
	}

	CoUninitialize();
}

void JsHost::ReportException(TryCatch& trycatch)
{
	std::stringstream ss;

	ss << "Exception in\r\n";

	HandleScope handleScope;
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

void JsHost::QueueInput(std::unique_ptr<std::function<void()> > && item)
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
	auto call = make_unique<std::function<void()> >([this, line]() 
	{
		ExecuteString(line);
	});

	QueueInput(std::move(call));
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

	return _pView->GetLineColor(dwLine);
}

void JsHost::ExecuteString(const std::string & line)
{
	Handle<Script> script;
    HandleScope handleScope;

	// echo to output
	OutputLine(line.c_str());
	// save in history
	_pHistory->Append(line.c_str());

    auto scriptSource = String::New(line.c_str());
    auto scriptName = String::New("unnamed");

	{
		// Compile script in try/catch context.
		TryCatch trycatch;
		script = Script::Compile(scriptSource, scriptName);
		if (script.IsEmpty()) 
		{
			ReportException(trycatch);
			return;
		}
	}

	{
		TryCatch trycatch;

		script->Run();
		if (trycatch.HasCaught()) 
		{
			ReportException(trycatch);
			return;
		}
    }
}

// console access
void JsHost::OutputLine(const char * psz)
{
	// output view expects call on app thread 
	std::string sz(psz);

	_pApp->PostWork([this, sz]() 
	{
		_pApp->POutputView()->OutputLineA(sz.c_str());
	});
}

void JsHost::SetViewLayout(double cmdHeight, double outHeight)
{
	_pApp->PostWork([this, cmdHeight, outHeight]() 
	{
		_pApp->SetDockLayout(cmdHeight, outHeight);
	});
}

LineInfo& JsHost::GetLine(size_t idx)
{
	if(!_pTraceColl)
	{
		static LineInfo line;
		return line;
	}

	return _pTraceColl->GetLine(idx);
}

size_t JsHost::GetLineCount()
{
	if(!_pTraceColl)
	{
		return CColor::DEFAULT_TEXT;
	}

	return _pTraceColl->GetLineCount();
}

void JsHost::UpdateLinesActive(CBitSet & set, int change)
{
	if(!_pTraceColl)
	{
		return;
	}

	_pTraceColl->UpdateLinesActive(set, change);

	_pApp->PostWork([this]() 
	{
		_pApp->PTraceView()->OnFilterChange();
	});
}

void JsHost::RefreshView()
{
	_pApp->PostWork([this]() 
	{
		_pApp->PTraceView()->Repaint();
	});
}

bool JsHost::SetTraceFormat(const char * psz)
{
	// TODO: call can happen before file is loaded
	if(!_pTraceColl->SetTraceFormat(psz))
	{
		return false;
	}

	_pApp->PostWork([this]() 
	{
		_pApp->PTraceView()->InitColumns();
	});

	return true;
}
