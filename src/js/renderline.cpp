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
#include "renderline.h"
#include "apphost.h"
#include "bitset.h"
#include "stringreader.h"
#include "make_unique.h"
#include "RenderLineparser.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> RenderLine::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void RenderLine::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "index"), jsIndexGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "time"), jsTimeGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "thread"), jsThreadGetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "msg"), jsMsgGetter);
	tmpl_proto->Set(String::NewFromUtf8(iso, "user"), FunctionTemplate::New(iso, jsGetUser));

	tmpl->SetClassName(String::NewFromUtf8(iso, "ViewLine"));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	_Template.Reset(iso, tmpl);
}

void RenderLine::InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	target->Set(String::NewFromUtf8(iso, "ViewLine"), RenderLine::GetTemplate(iso)->GetFunction());
}

///////////////////////////////////////////////////////////////////////////////
RenderLine::RenderLine(const v8::Handle<v8::Object>& handle, int lineNum)
	: _Line(GetCurrentHost()->GetLine(lineNum))
{
	Wrap(handle);
}

///////////////////////////////////////////////////////////////////////////////
void RenderLine::jsNew(const FunctionCallbackInfo<Value> &args)
{
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		ThrowTypeError("Invalid number of FunctionCallbackInfo<Value>. Format RenderLine(int)");
	}

	RenderLine *line = new RenderLine(args.This(), args[0]->Int32Value());

	args.GetReturnValue().Set(args.This());
}

void RenderLine::jsGetUser(const FunctionCallbackInfo<Value> &args)
{
}

void RenderLine::jsIndexGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	RenderLine * pThis = UnwrapThis<RenderLine>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Line.Index));
}

void RenderLine::jsTimeGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
}

void RenderLine::jsThreadGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	RenderLine * pThis = UnwrapThis<RenderLine>(info.This());
	info.GetReturnValue().Set(Integer::New(Isolate::GetCurrent(), pThis->_Line.Tid));
}

void RenderLine::jsMsgGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	RenderLine * pThis = UnwrapThis<RenderLine>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), pThis->_Line.Msg.psz, String::kNormalString, pThis->_Line.Msg.cch));
}


} // Js
