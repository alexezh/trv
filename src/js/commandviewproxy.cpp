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
#include "commandviewproxy.h"
#include "apphost.h"
#include "color.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> CommandViewProxy::_Template;

///////////////////////////////////////////////////////////////////////////////
//
CommandViewProxy::CommandViewProxy(const v8::Handle<v8::Object>& handle)
{
	Wrap(handle);
}

void CommandViewProxy::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->Set(String::NewFromUtf8(iso, "setText"), FunctionTemplate::New(iso, jsSetText));
	tmpl_proto->Set(String::NewFromUtf8(iso, "setFocus"), FunctionTemplate::New(iso, jsSetFocus));
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "visible"), jsVisibleGetter, jsVisibleSetter);

	_Template.Reset(iso, tmpl);
}

void CommandViewProxy::jsNew(const FunctionCallbackInfo<Value> &args)
{
	LOG("");
	CommandViewProxy *proxy = new CommandViewProxy(args.This());

	args.GetReturnValue().Set(args.This());
}

void CommandViewProxy::jsSetText(const FunctionCallbackInfo<Value>& args)
{
	if (args.Length() != 1)
	{
		throw V8RuntimeException("One parameter required");
	}

	v8::String::Utf8Value str(args[0]);
	std::string strA(*str, str.length());

	return GetCurrentHost()->ConsoleSetConsole(strA.c_str());
}

void CommandViewProxy::jsSetFocus(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	return GetCurrentHost()->ConsoleSetFocus();
}

void CommandViewProxy::jsVisibleGetter(Local<String> property, const PropertyCallbackInfo<v8::Value>& info)
{
//	auto pThis = UnwrapThis<CommandViewProxy>(info.This());
//	info.GetReturnValue().Set(Boolean::New(Isolate::GetCurrent(), GetCurrentHost()->GetCommandViewVisible());
}

void CommandViewProxy::jsVisibleSetter(Local<String> property, Local<Value> value,
	const PropertyCallbackInfo<void>& info)
{
//	auto pThis = UnwrapThis<CommandViewProxy>(info.This());
//	pThis->ShowFiltersWorker(value->BooleanValue());
}



} // Js

