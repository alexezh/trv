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
#include "traceline.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> View::_Template;

///////////////////////////////////////////////////////////////////////////////
//
View::View(const v8::Handle<v8::Object>& handle)
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
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "setFocusLine"), FunctionTemplate::New(iso, jsSetFocusLine));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "onRender"), FunctionTemplate::New(iso, jsOnRender));

	_Template.Reset(iso, tmpl);

	FilterItemProxy::Init(iso);
}

void View::jsNew(const FunctionCallbackInfo<Value> &args)
{
	View *view = new View(args.This());
	GetCurrentHost()->OnViewCreated(view);

	args.GetReturnValue().Set(args.This());

	GetCurrentHost()->RegisterRequestLineHandler([view](Isolate* iso, DWORD idx) -> std::unique_ptr<ViewLine>
	{
		return view->HandleLineRequest(iso, idx);
	});
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

void View::jsSetFocusLine(const FunctionCallbackInfo<Value>& args)
{
	if(args.Length() != 1 || !args[0]->IsInt32())
		ThrowSyntaxError("expected $v.setFocusLine(idx)\r\n");

	GetCurrentHost()->SetFocusLine(args[0]->IntegerValue());
}

void View::jsOnRender(const FunctionCallbackInfo<Value>& args)
{
	if (args.Length() != 1 || !(args[0]->IsFunction() || args[0]->IsNull()))
		ThrowSyntaxError("expected $v.onRender(function)");

	auto pThis = UnwrapThis<View>(args.This());
	pThis->m_OnRender.Reset(Isolate::GetCurrent(), args[0].As<Function>());
}

std::unique_ptr<ViewLine> View::HandleLineRequest(Isolate* iso, DWORD idx)
{
	std::unique_ptr<ViewLine> viewLine(new ViewLine());
	auto& line = GetCurrentHost()->GetLine(idx);
	viewLine->SetLineIndex(line.Index);
	viewLine->SetThreadId(line.Tid);

	if (m_OnRender.IsEmpty())
	{
		viewLine->SetTime(std::string(line.Time.psz, line.Time.cch));
		viewLine->SetMsg(std::string(line.Msg.psz, line.Msg.cch));
		viewLine->SetUser(0, std::string(line.User[0].psz, line.User[0].cch));
		viewLine->SetUser(1, std::string(line.User[1].psz, line.User[1].cch));
		viewLine->SetUser(2, std::string(line.User[2].psz, line.User[2].cch));
		viewLine->SetUser(3, std::string(line.User[3].psz, line.User[3].cch));
	}
	else
	{
		auto onRender = Local<Function>::New(iso, m_OnRender);
		auto idxJs(Integer::New(Isolate::GetCurrent(), idx).As<Value>());
		auto lineJs(TraceLine::GetTemplate(iso)->GetFunction()->NewInstance(1, &idxJs).As<Value>());
		auto viewLineJs = onRender->Call(iso->GetCurrentContext()->Global(), 1, &lineJs).As<Object>();

		viewLine->SetTime(GetPropertyString(iso, viewLineJs, "time"));
		viewLine->SetMsg(GetPropertyString(iso, viewLineJs, "msg"));
		viewLine->SetUser(0, GetPropertyString(iso, viewLineJs, "user1"));
		viewLine->SetUser(1, GetPropertyString(iso, viewLineJs, "user2"));
		viewLine->SetUser(2, GetPropertyString(iso, viewLineJs, "user3"));
		viewLine->SetUser(3, GetPropertyString(iso, viewLineJs, "user4"));
	}

	return viewLine;
}

void View::jsCurrentLineGetter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), GetCurrentHost()->GetCurrentLine()));
}

void View::jsGetSelected(const FunctionCallbackInfo<Value>& args)
{

}

void View::RefreshView()
{
	GetCurrentHost()->RefreshView();
}

} // Js

