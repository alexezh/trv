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
#pragma once

#include "objectwrap.h"

namespace Js {

// history should be accessible from script
// $h.show, $h.find("match"), $h.run(id)
// use ctrl+r for interactive find?

class History : public BaseObject<History>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso) 
	{ 
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}
	
	void Append(const char* pszLine);
	void Load();
	size_t GetCount()
	{
		return _Data.size();
	}

	bool GetEntry(size_t idx, std::string& entry)
	{
		if(idx >= _Data.size())
		{
			return false;
		}

		entry = *(GetEntry(idx));

		return true;
	}

	std::shared_ptr<std::string> GetEntry(size_t idx)
	{
		int i = 0;
		for(auto it = _Data.begin(); it != _Data.end(); ++it, i++)
		{
			if(i == idx)
			{
				return (*it);
			}
		}

		assert(false);
		return nullptr;
	}

private:
	History(const v8::Handle<v8::Object>& handle);
	~History();

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsPrint(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsExec(const v8::FunctionCallbackInfo<v8::Value> &args);
	bool InitFilePath();
	bool OpenStream(std::fstream& stm, std::ios_base::openmode mode);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	typedef std::shared_ptr<std::string> EntryPtr;
	struct EntryCompr
	{
		bool operator()(const EntryPtr& e1, const EntryPtr& e2) const
		{
			return (*e1) < (*e2);
		}
	};
	std::string _FilePath;
	std::list<EntryPtr> _Data;
};

} // Js

