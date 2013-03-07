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
#include "funcwrap.h"
#include "apphost.h"
#include "error.h"
#include "stringutils.h"

using namespace v8;

namespace Js {

v8::Persistent<v8::FunctionTemplate> History::_Template;

void History::Init()
{
	_Template = Persistent<FunctionTemplate>(FunctionTemplate::New(jsNew));
	_Template->PrototypeTemplate()->Set("print", FunctionTemplate::New(&jsPrint));
	_Template->PrototypeTemplate()->Set("exec", FunctionTemplate::New(&jsExec));

	_Template->InstanceTemplate()->SetInternalFieldCount(1);
}

void History::Append(const char* pszLine)
{
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

v8::Handle<v8::Value> History::jsNew(const v8::Arguments &args)
{
	History *hist = new History(args.This());
	GetCurrentHost()->OnHistoryCreated(hist);
	return args.This();
}

History::History(const v8::Handle<v8::Object>& handle)
{
	Wrap(handle);
	InitFilePath();
}

bool History::InitFilePath()
{
    CComPtr<IKnownFolderManager> kfm;
    CComPtr<IKnownFolder> kf;

    HRESULT hr = CoCreateInstance(CLSID_KnownFolderManager, 
							NULL, 
							CLSCTX_INPROC_SERVER, 
							__uuidof(IKnownFolderManager), 
							(void**)&kfm);
	if(FAILED(hr))
	{
		assert(false);
		return false;
	}
	hr = kfm->GetFolder(FOLDERID_LocalAppData, &kf);
	if(FAILED(hr))
	{
		assert(false);
		return false;
	}
	WCHAR* pszPath;
	hr = kf->GetPath(KF_FLAG_DEFAULT_PATH, &pszPath);
	if(FAILED(hr))
	{
		assert(false);
		return false;
	}

	std::string path;
	WszToString(pszPath, path);
	CoTaskMemFree(pszPath);

	path += "\\trv.js";
	
	// make sure that folder exists
	CreateDirectoryA(path.c_str(), NULL);

	// read the file
	_FilePath = path;
	_FilePath += "\\.history";
	return true;
}

History::~History()
{
	assert(false);
	GetCurrentHost()->OnHistoryCreated(nullptr);
}

v8::Handle<v8::Value> History::jsPrint(const v8::Arguments& args)
{
	return TryCatchCpp([&args] 
	{
		auto pThis = UnwrapThis<History>(args.This());
		assert(args.Length() == 0);

		std::stringstream ss;
		int id = 0;
		ss << "History:\r\n";
		for(auto it = pThis->_Data.begin(); it != pThis->_Data.end(); it++, id++)
		{
			ss << id << ":" << (*(*it)) << "\r\n";
		}
		GetCurrentHost()->OutputLine(ss.str().c_str());

		return Undefined();
	});
}

v8::Handle<v8::Value> History::jsExec(const v8::Arguments& args)
{
	return TryCatchCpp([&args]() -> v8::Handle<v8::Value>
	{
		auto pThis = UnwrapThis<History>(args.This());
		assert(args.Length() == 1);

		if(args.Length() != 1 || !args[0]->IsInt32())
		{
			return ThrowException(Exception::SyntaxError(String::New(
					"expected exec(id)\r\n")));
		}

		auto id = args[0]->Int32Value();
		if(id >= pThis->_Data.size())
		{
			return ThrowException(Exception::SyntaxError(String::New(
					"invalid history index\r\n")));
		}

		auto scriptSource = v8::String::New(pThis->GetEntry(id)->c_str());
		auto scriptName = v8::String::New("*str");

		auto script = v8::Script::Compile(scriptSource, scriptName);
		if (script.IsEmpty()) 
		{
			return ThrowException(Exception::TypeError(String::New(
					"compile failed\r\n")));
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

