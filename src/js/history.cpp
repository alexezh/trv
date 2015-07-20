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
#include "history.h"
#include "apphost.h"
#include "error.h"
#include "stringutils.h"
#include "log.h"

using namespace v8;

namespace Js {

v8::UniquePersistent<v8::FunctionTemplate> History::_Template;

void History::Init(v8::Isolate* iso)
{
	LOG("");
	auto tmpl = FunctionTemplate::New(iso, jsNew);
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "print"), FunctionTemplate::New(iso, &jsPrint));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "exec"), FunctionTemplate::New(iso, &jsExec));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	_Template = UniquePersistent<FunctionTemplate>(iso, tmpl);

}

void History::Append(const char* pszLine)
{
	LOG("@%p %s", this, pszLine);
	bool found = false;
	// if we can find entry, append it to the end
	for(auto it = _Data.begin(); it != _Data.end(); it++)
	{
		if(*(*it) == pszLine)
		{
			auto s = (*it);
			_Data.erase(it);
			_Data.push_back(s);
			found = true;
			break;
		}
	}

	if(!found)
	{
		EntryPtr s = std::make_shared<std::string>(pszLine);
		_Data.push_back(s);
	}

	// append line to file
	std::fstream stm;
	if(!OpenStream(stm, std::fstream::out|std::fstream::trunc))
	{
		return;
	}

	for(auto it = _Data.begin(); it != _Data.end(); it++)
	{
		stm << (*(*it)) << "\n";
	}
}

void History::jsNew(const v8::FunctionCallbackInfo<Value> &args)
{
	History *hist = new History(args.This());
	LOG("@%p", hist);
	GetCurrentHost()->OnHistoryCreated(hist);
	args.GetReturnValue().Set(args.This());
}

History::History(const v8::Handle<v8::Object>& handle)
{
	Wrap(handle);
	InitFilePath();
}

bool History::InitFilePath()
{
	// read the file
	_FilePath = GetCurrentHost()->GetAppDataDir();
	_FilePath += "\\.history";
	return true;
}

History::~History()
{
	assert(false);
	GetCurrentHost()->OnHistoryCreated(nullptr);
}

void History::jsPrint(const v8::FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = UnwrapThis<History>(args.This());
		LOG("@%p", pThis);
		assert(args.Length() == 0);

		std::stringstream ss;
		int id = 0;
		ss << "History:\r\n";
		for(auto it = pThis->_Data.begin(); it != pThis->_Data.end(); it++, id++)
		{
			ss << id << ":" << (*(*it)) << "\r\n";
		}
		GetCurrentHost()->OutputLine(ss.str().c_str());
		return Local<Value>();
	});
}

void History::jsExec(const v8::FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> v8::Handle<v8::Value>
	{
		auto pThis = UnwrapThis<History>(args.This());
		LOG("@%p", pThis);
		assert(args.Length() == 1);

		if(args.Length() != 1 || !args[0]->IsInt32())
		{
			ThrowSyntaxError("expected exec(id)\r\n");
		}

		auto id = args[0]->Int32Value();
		if(id >= pThis->_Data.size())
		{
			ThrowSyntaxError("invalid history index\r\n");
		}

		auto scriptSource = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), pThis->GetEntry(id)->c_str());
		auto scriptName = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "*str");

		auto script = v8::Script::Compile(scriptSource, scriptName);
		if (script.IsEmpty()) 
		{
			ThrowTypeError("compile failed\r\n");
		}

		return script->Run();
	});
}

bool History::OpenStream(std::fstream& stm, std::ios_base::openmode mode)
{
	if(_FilePath.length() == 0)
	{
		return false;
	}

	stm.open(_FilePath, mode);
	if(stm.fail())
	{
		return false;
	}

	return true;
}

void History::Load()
{
	std::fstream stm;
	if(!OpenStream(stm, std::fstream::in))
	{
		return;
	}

	while(stm)
	{
		std::string line;
		std::getline(stm, line);
		if(stm.fail())
		{
			break;
		}
		if(line.length() == 0)
		{
			continue;
		}
		EntryPtr s = std::make_shared<std::string>(std::move(line));
		_Data.push_back(s);
	}
}

} // Js

