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
#include <locale>
#include "shortcuts.h"
#include "apphost.h"
#include "color.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> Shortcuts::_Template;

///////////////////////////////////////////////////////////////////////////////
//
Shortcuts::Shortcuts(const v8::Handle<v8::Object>& handle)
{
	Wrap(handle);
}

void Shortcuts::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->Set(String::NewFromUtf8(iso, "add"), FunctionTemplate::New(iso, jsAdd));

	_Template.Reset(iso, tmpl);
}

void Shortcuts::jsNew(const FunctionCallbackInfo<Value> &args)
{
	LOG("");
	Shortcuts *obj = new Shortcuts(args.This());
	GetCurrentHost()->OnShortcutsCreated(obj);

	args.GetReturnValue().Set(args.This());
}

void Shortcuts::jsAdd(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]
	{
		auto pThis = UnwrapThis<Shortcuts>(args.This());
		return pThis->AddWorker(args);
	});
}

v8::Local<v8::Value> Shortcuts::AddWorker(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	LOG("@%p", this);
	if (args.Length() != 2)
	{
		ThrowSyntaxError("expected add(name, function)\r\n");
	}

	if (!args[0]->IsString())
	{
		std::stringstream ss;

		ss << "first parameter has to be string\r\n";

		auto s = args[0]->ToString();
		String::Utf8Value str(s);
		ss << (*str);

		ThrowSyntaxError(ss.str().c_str());
	}

	if (!args[1]->IsFunction())
	{
		std::stringstream ss;

		ss << "second parameter has to be a function\r\n";
		ss << *String::Utf8Value(args[1]->ToString());

		ThrowSyntaxError(ss.str().c_str());
	}

	auto key = args[0]->ToString();
	String::Utf8Value strKey(key);
	auto szKey = std::string(*strKey, strKey.length());
	size_t idx = szKey.find('+');

	if (idx == std::string::npos)
		ThrowKeyError(szKey);

	// parse key
	auto szModifier = szKey.substr(0, idx);
	auto szVKey = szKey.substr(idx+1);
	if (szVKey.length() != 1)
		ThrowKeyError(szKey);

	uint8_t modifier = 0;
	if (_stricmp(szModifier.c_str(), "ctrl") == 0)
	{
		modifier |= FCONTROL;
	}
	else if (_stricmp(szModifier.c_str(), "alt") == 0)
	{
		modifier |= FALT;
	}
	else
	{
		ThrowKeyError(szKey);
	}

	auto code = GetVKey(szVKey[0]);
	_Keys.emplace(GetKey(modifier, code), UniquePersistent<Function>(Isolate::GetCurrent(), args[1].As<Function>()));

	GetCurrentHost()->AddShortcut(modifier, code);

	return v8::Local<v8::Value>();
}

uint16_t Shortcuts::GetVKey(char c)
{
	return toupper(c);
}

void Shortcuts::ThrowKeyError(const std::string& key)
{
	std::stringstream ss;

	ss << "string should be in form Ctrl+X or Alt+X\r\n";
	ss << key;

	ThrowSyntaxError(ss.str().c_str());
}

void Shortcuts::Execute(Isolate* iso, uint8_t modifier, uint16_t key)
{
	TryCatch try_catch;
	try_catch.SetVerbose(true);

	auto it = _Keys.find(GetKey(modifier, key));
	if (it == _Keys.end())
	{
		LOG("@%p cannot find key", this);
		return;
	}

	auto func(v8::Local<v8::Function>::New(v8::Isolate::GetCurrent(), it->second));

	func->Call(v8::Isolate::GetCurrent()->GetCurrentContext()->Global(), 0, nullptr);

	if (try_catch.HasCaught())
	{
		GetCurrentHost()->ReportException(v8::Isolate::GetCurrent(), try_catch);
	}
}

} // Js

