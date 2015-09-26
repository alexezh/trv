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

#include "bitset.h"
#include "objectwrap.h"

namespace Js {

class Shortcuts : public BaseObject<Shortcuts>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> GetTemplate(v8::Isolate* iso) 
	{ 
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	void Execute(v8::Isolate* iso, uint8_t modifier, uint16_t key);

private:
	Shortcuts(const v8::Handle<v8::Object>& handle);

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsAdd(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
	v8::Local<v8::Value> AddWorker(const v8::FunctionCallbackInfo<v8::Value>& args);
	void ThrowKeyError(const std::string& key);

	static DWORD GetKey(uint8_t mod, uint16_t key)
	{
		return (static_cast<int>(mod) << 16) | key;
	}

	uint16_t GetVKey(const std::string& key);

	static v8::Persistent<v8::FunctionTemplate> _Template;
	std::mutex _Lock;
	std::unordered_map<DWORD, v8::UniquePersistent<v8::Function>> _Keys;
};

} // Js

