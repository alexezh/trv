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
#include "viewproxy.h"
#include "apphost.h"
#include "query.h"
#include "trace.h"
#include "tracecollection.h"
#include "filteritem.h"
#include "color.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> View::_Template;

///////////////////////////////////////////////////////////////////////////////
//
View::View(const v8::Handle<v8::Object>& handle)
	: _ShowFilters(true)
{
	Wrap(handle);
}

void View::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	auto tmpl_proto = tmpl->InstanceTemplate();

	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "setViewLayout"), FunctionTemplate::New(iso, jsSetViewLayout));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "setColumns"), FunctionTemplate::New(iso, jsSetColumns));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "setSource"), FunctionTemplate::New(iso, jsSetSource));


	tmpl->InstanceTemplate()->SetAccessor(String::NewFromUtf8(iso, "showFilters"), jsShowFiltersGetter, jsShowFiltersSetter);
	tmpl->InstanceTemplate()->SetAccessor(String::NewFromUtf8(iso, "currentLine"), jsCurrentLineGetter);

	_Template.Reset(iso, tmpl);

	FilterItemProxy::Init(iso);
}

void View::jsNew(const FunctionCallbackInfo<Value> &args)
{
	View *view = new View(args.This());
	GetCurrentHost()->OnViewCreated(view);

	args.GetReturnValue().Set(args.This());
}

void View::jsSetSource(const FunctionCallbackInfo<Value>& args)
{
	auto pThis = UnwrapThis<View>(args.This());
	if(!(args.Length() == 1 && (args[0]->IsObject() || args[0]->IsNull())))
		ThrowSyntaxError("expected $v.setSource(collection)\r\n");

	if (args[0]->IsObject())
	{
		auto traceColl = TraceCollection::TryGetCollection(args[0].As<Object>());
		if (traceColl == nullptr)
			ThrowSyntaxError("expected $v.setSource(collection)\r\n");

		GetCurrentHost()->SetViewSource(traceColl->GetLines());
	}
	else
	{
		GetCurrentHost()->SetViewSource(nullptr);
	}
}

void View::jsSetViewLayout(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args] () -> Local<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 2);

		if(args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsNumber())
		{
			ThrowSyntaxError("expected $v.setviewlayout(cmdheight, outheight)\r\n");
		}

		GetCurrentHost()->SetViewLayout(args[0]->NumberValue(), args[1]->NumberValue());
		return Local<Value>();
	});
}

void View::jsSetColumns(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 1);

		if (args.Length() != 1 || !args[0]->IsArray())
		{
			ThrowSyntaxError("expected $v.addcolumn(name)\r\n");
		}

		auto jsColumns = args[0].As<Array>();
		std::vector<std::string> columns;
		for (int i = 0; i < jsColumns->Length(); i++)
		{
			String::Utf8Value str(jsColumns->Get(i)->ToString());
			columns.push_back(std::string(*str, str.length()));
		}
		GetCurrentHost()->SetColumns(columns);

		return Local<Value>();
	});
}

void View::jsShowFiltersGetter(Local<String> property, const PropertyCallbackInfo<v8::Value>& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	info.GetReturnValue().Set(Boolean::New(Isolate::GetCurrent(), pThis->_ShowFilters));
}

void View::jsShowFiltersSetter(Local<String> property, Local<Value> value,
							const PropertyCallbackInfo<void>& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	pThis->ShowFiltersWorker(value->BooleanValue());
}

void View::jsCurrentLineGetter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), GetCurrentHost()->GetCurrentLine()));
}

void View::RefreshView()
{
	GetCurrentHost()->RefreshView();
}

void View::ShowFiltersWorker(bool val)
{
	LOG("@%p show=%d", this, (int)val);
	_ShowFilters = val;
	GetCurrentHost()->RefreshView();
}

} // Js

