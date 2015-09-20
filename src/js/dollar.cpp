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
#include "dollar.h"
#include "query.h"
#include "apphost.h"
#include "trace.h"
#include "viewproxy.h"
#include "history.h"
#include "dotexpressions.h"
#include "shortcuts.h"
#include "commandviewproxy.h"
#include "tagger.h"
#include "tracecollection.h"
#include "subtracesource.h"
#include "log.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> Dollar::_Template;

void Dollar::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));

	v8::Handle<v8::ObjectTemplate> tmpl_proto = tmpl->PrototypeTemplate();
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	tmpl_proto->Set(String::NewFromUtf8(iso, "import"), FunctionTemplate::New(iso, jsImport));
	tmpl_proto->Set(String::NewFromUtf8(iso, "print"), FunctionTemplate::New(iso, jsPrint));
	tmpl_proto->Set(String::NewFromUtf8(iso, "loadtrace"), FunctionTemplate::New(iso, jsLoadTrace));

	_Template = UniquePersistent<FunctionTemplate>(iso, tmpl);

	DotExpressions::Init(iso);
	Shortcuts::Init(iso);
	CommandViewProxy::Init(iso);
	Tagger::Init(iso);
}

void Dollar::InitInstance(Isolate* iso, Handle<Object> & target)
{
	auto tmpl = Local<FunctionTemplate>::New(iso, _Template);
	// create an instance of dollar object and make a property
	auto dollar = tmpl->GetFunction()->NewInstance();
	// target->SetAccessor(String::NewFromUtf8(iso, "$"), jsGetter, 0, dollar);
	target->Set(String::NewFromUtf8(iso, "$"), dollar);
}

void Dollar::jsNew(const FunctionCallbackInfo<Value> &args)
{
	Dollar *dollar;
	dollar = new Dollar();
	LOG("@%p", dollar);
	auto iso = Isolate::GetCurrent();
	args.This()->SetInternalField(0, v8::External::New(iso, dollar));

	auto source = GetCurrentHost()->GetFileTraceSource();
	auto sourceProxy = TraceSourceProxy::GetTemplate(iso)->GetFunction()->NewInstance(1, &v8::External::New(iso, &source).As<Value>());
	args.This()->SetAccessor(String::NewFromUtf8(iso, "trace"), jsGetter, 0, sourceProxy);

	// create an instance of view object and make a property
	auto view = View::GetTemplate(iso)->GetFunction()->NewInstance();
	args.This()->SetAccessor(String::NewFromUtf8(iso, "view"), jsGetter, 0, view);

	auto history = History::GetTemplate(iso)->GetFunction()->NewInstance();
	args.This()->SetAccessor(String::NewFromUtf8(iso, "history"), jsGetter, 0, history);

	auto dotExpressions = DotExpressions::GetTemplate(iso)->GetFunction()->NewInstance();
	args.This()->SetAccessor(String::NewFromUtf8(iso, "dotexpressions"), jsGetter, 0, dotExpressions);

	auto shortcuts = Shortcuts::GetTemplate(iso)->GetFunction()->NewInstance();
	args.This()->SetAccessor(String::NewFromUtf8(iso, "shortcuts"), jsGetter, 0, shortcuts);

	auto console = CommandViewProxy::GetTemplate(iso)->GetFunction()->NewInstance();
	args.This()->SetAccessor(String::NewFromUtf8(iso, "console"), jsGetter, 0, console);

	auto tagger = Tagger::GetTemplate(iso)->GetFunction()->NewInstance();
	args.This()->SetAccessor(String::NewFromUtf8(iso, "tagger"), jsGetter, 0, tagger);

	args.GetReturnValue().Set(args.This());
}

bool Dollar::ImportFile(const char * pszFile, bool opt)
{
	LOG(" %s", pszFile);
	return ImportWorker(pszFile, opt);
}
   
void Dollar::jsGetter(Local<String> property, const PropertyCallbackInfo<v8::Value>& info)
{
	auto data = info.Data();
	info.GetReturnValue().Set(data);
}

void Dollar::jsImport(const FunctionCallbackInfo<Value>& args)
{
	if(args.Length() != 1)
	{
		ThrowTypeError("$.import requires one parameter\r\n");
	}

	v8::String::Utf8Value str(args[0]);
	args.GetReturnValue().Set(ImportWorker(*str, false));
}

bool Dollar::OpenScriptStream(const char * pszName, std::fstream& stm)
{
	for(int i = 0;;i++)
	{
		std::string fileName;

		if(i == 0)
		{
			// use current dir
			fileName = pszName;
		}
		else if (i == 1)
		{
			fileName = GetCurrentHost()->GetAppDataDir();
			fileName += "\\";
			fileName += pszName;
		}
		else if (i == 2)
		{
			// exe directory 
			char szPath[_MAX_PATH];
			char szDrive[_MAX_PATH];
			char szDir[_MAX_PATH];
			GetModuleFileNameA(NULL, szPath, _countof(szPath)); 

			_splitpath(szPath, szDrive, szDir, NULL, NULL);
			fileName = szDrive;
			fileName += szDir;
			fileName += pszName;
		}
		else
		{
			break;
		}
		stm.open(fileName, std::fstream::in);
		if(!stm.fail())
		{
			return true;
		}
	}

	return false;
}

bool Dollar::ImportWorker(const char * pszName, bool opt)
{
	std::fstream stm;
	TryCatch try_catch;

	// search file in different locations
	if(!OpenScriptStream(pszName, stm))
	{
		if (opt)
		{
			return false;
		}
		else
		{
			std::string s = std::string("$.import cannot open file ") + pszName + "\r\n";
			ThrowTypeError(s.c_str());
		}
	}

	std::string scriptLine((std::istreambuf_iterator<char>(stm)), std::istreambuf_iterator<char>());

	auto scriptSource = v8::String::NewFromUtf8(Isolate::GetCurrent(), scriptLine.c_str());
	auto scriptName = v8::String::NewFromUtf8(Isolate::GetCurrent(), pszName);

	auto script = v8::Script::Compile(scriptSource, scriptName);
	if (script.IsEmpty()) 
	{
		GetCurrentHost()->ReportException(Isolate::GetCurrent(), try_catch);
		return false;
	}

	auto v = script->Run();
	if(v.IsEmpty())
	{
		GetCurrentHost()->ReportException(Isolate::GetCurrent(), try_catch);
		return false;
	}

	return true;
}

void Dollar::jsLoadTrace(const v8::FunctionCallbackInfo<Value>& args)
{
	if(args.Length() < 1)
	{
		ThrowTypeError("expected $.loadtrace(name, start(Mb), stop(Mb))\r\n");
	}

	v8::String::Utf8Value str(args[0]);
	int posStart = 0, posEnd = -1;

	if(args.Length() >= 2 && args[1]->IsInt32())
	{
		posStart = args[1]->Int32Value();
	}

	if(args.Length() >= 3 && args[2]->IsInt32())
	{
		posEnd = args[2]->Int32Value();
	}

	GetCurrentHost()->LoadTrace(*str, posStart, posEnd);

	args.GetReturnValue().SetUndefined();
}

void Dollar::jsPrint(const v8::FunctionCallbackInfo<Value>& args)
{
	bool first = true;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::HandleScope handle_scope(Isolate::GetCurrent());
		if (first) 
		{
			first = false;
		} else {
			printf(" ");
		}

		v8::String::Utf8Value str(args[i]);
		GetCurrentHost()->OutputLine(*str);
	}

	args.GetReturnValue().SetUndefined();
}

void ThrowInvalidOpenCollection()
{
	ThrowTypeError("expected $.openCollection(collection)\r\n");
}

} // Js
