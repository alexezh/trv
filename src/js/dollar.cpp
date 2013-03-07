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
#include "view.h"
#include "history.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> Dollar::_Template;

void Dollar::Init()
{
	_Template = Persistent<FunctionTemplate>::New(FunctionTemplate::New(jsNew));
	_Template->InstanceTemplate()->SetInternalFieldCount(1);
	_Template->InstanceTemplate()->Set(String::New("query"), FunctionTemplate::New(jsQuery));
	_Template->InstanceTemplate()->Set(String::New("import"), FunctionTemplate::New(jsImport));
	_Template->InstanceTemplate()->Set(String::New("print"), FunctionTemplate::New(jsPrint));
}

void Dollar::InitInstance(Handle<Object> & target)
{
	// create an instance of dollar object and make a property
	auto dollar = _Template->InstanceTemplate()->NewInstance();
	target->SetAccessor(String::New("$"), jsGetter, 0, dollar);
}

Handle<Value> Dollar::jsNew(const Arguments &args)
{
	Dollar *dollar;
	dollar = new Dollar();
	args.This()->SetInternalField(0, v8::External::New(dollar));

	auto trace = Trace::GetTemplate()->InstanceTemplate()->NewInstance();
	args.This()->SetAccessor(String::New("trace"), jsGetter, 0, trace);

	// create an instance of view object and make a property
	auto view = View::GetTemplate()->InstanceTemplate()->NewInstance();
	args.This()->SetAccessor(String::New("view"), jsGetter, 0, view);

	auto history = History::GetTemplate()->InstanceTemplate()->NewInstance();
	args.This()->SetAccessor(String::New("history"), jsGetter, 0, history);

	return args.This();
}

bool Dollar::ImportFile(const char * pszFile)
{
	bool success;
	ImportWorker(pszFile, success);
	assert(success);
	return success;
}

   
Handle<Value> Dollar::jsGetter(Local<String> property, 
												const AccessorInfo& info)
{
	auto data = info.Data();
	return data;
}

Handle<Value> Dollar::jsImport(const Arguments& args)
{
	bool success;
	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New(
				"$.import requires one parameter\r\n")));
	}

	v8::String::Utf8Value str(args[0]);
	return ImportWorker(*str, success);
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
		else if(i == 1)
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

Handle<Value> Dollar::ImportWorker(const char * pszName, bool& success)
{
	std::fstream stm;
	success = true;

	// search file in different locations
	if(!OpenScriptStream(pszName, stm))
	{
		success = false;
		std::string s = std::string("$.import cannot open file ") + pszName + "\r\n";
		return ThrowException(Exception::TypeError(String::New(s.c_str())));
	}

	std::string scriptLine((std::istreambuf_iterator<char>(stm)), std::istreambuf_iterator<char>());

	auto scriptSource = v8::String::New(scriptLine.c_str());
    auto scriptName = v8::String::New(pszName);

	auto script = v8::Script::Compile(scriptSource, scriptName);
	if (script.IsEmpty()) 
	{
		success = false;
		std::string s = std::string("$.import compile failed for file ") + pszName + "\r\n";
		return ThrowException(Exception::TypeError(String::New(s.c_str())));
	}

	auto v = script->Run();
	if(v.IsEmpty())
	{
		success = false;
	}

	return v;
}

Handle<Value> Dollar::jsQuery(const Arguments& args)
{
	if(args.Length() != 1)
	{
		return ThrowException(Exception::TypeError(String::New(
				"$.query requires one parameter\r\n")));
	}

	return Query::GetTemplate()->GetFunction()->NewInstance(1, &args[0]);
}

v8::Handle<v8::Value> Dollar::jsPrint(const v8::Arguments& args)
{
	bool first = true;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::HandleScope handle_scope;
		if (first) 
		{
			first = false;
		} else {
			printf(" ");
		}

		v8::String::Utf8Value str(args[i]);
		GetCurrentHost()->OutputLine(*str);
	}

	return v8::Undefined();
}

} // Js
