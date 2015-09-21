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
#include "filteritem.h"
#include "apphost.h"
#include "query.h"
#include "trace.h"
#include "tracecollection.h"
#include "tagger.h"
#include "color.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> FilterItemProxy::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void FilterItemProxy::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));

	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "source"), jsSourceGetter, jsSourceSetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "color"), jsColorGetter, jsColorSetter);
	tmpl_proto->SetAccessor(String::NewFromUtf8(iso, "description"), jsDescriptionGetter, jsDescriptionSetter);

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	tmpl->SetClassName(v8::String::NewFromUtf8(iso, "Filter"));
	_Template.Reset(iso, tmpl);
}

void FilterItemProxy::jsNew(const FunctionCallbackInfo<Value> &args)
{
	FilterItemProxy *item = new FilterItemProxy(args.This());

	args.GetReturnValue().Set(args.This());
}

void FilterItemProxy::jsSourceGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	FilterItemProxy * proxy = UnwrapThis<FilterItemProxy>(info.This());
	info.GetReturnValue().Set(proxy->_Filter->Collection);
}

void FilterItemProxy::jsSourceSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	FilterItemProxy * proxy = UnwrapThis<FilterItemProxy>(info.This());
	proxy->_Filter->UpdateFilter(value);
}

void FilterItemProxy::jsColorGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	FilterItemProxy * proxy = UnwrapThis<FilterItemProxy>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), CColor::Name(proxy->_Filter->Color)));
}

void FilterItemProxy::jsColorSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	FilterItemProxy * proxy = UnwrapThis<FilterItemProxy>(info.This());
	proxy->_Filter->UpdateColor(CColor::FromName(*String::Utf8Value(value->ToString())));
}

void FilterItemProxy::jsDescriptionGetter(Local<String> property,
	const PropertyCallbackInfo<v8::Value>& info)
{
	FilterItemProxy * proxy = UnwrapThis<FilterItemProxy>(info.This());
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), proxy->_Filter->Description.c_str()));
}

void FilterItemProxy::jsDescriptionSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
{
	FilterItemProxy * proxy = UnwrapThis<FilterItemProxy>(info.This());
	proxy->_Filter->Description = *String::Utf8Value(value->ToString());
}

///////////////////////////////////////////////////////////////////////////////
//
FilterItem::FilterItem(Local<Object> view)
{
	_Tagger.Reset(Isolate::GetCurrent(), view);
	_Tagger.SetWeak(this, WeakViewCallback);
	_Tagger.MarkIndependent();
}

void FilterItem::WeakViewCallback(
	const v8::WeakCallbackData<v8::Object, FilterItem>& data)
{
	v8::Isolate* isolate = data.GetIsolate();
	v8::HandleScope scope(isolate);
	FilterItem* wrap = data.GetParameter();
	LOG("@%p", wrap);
	wrap->_Tagger.Reset();
}

void FilterItem::UpdateFilter(Local<Value> val)
{
	auto tagger = Tagger::Unwrap(Local<Object>::New(Isolate::GetCurrent(), _Tagger));
	FilterFunc.Reset(Isolate::GetCurrent(), val.As<Object>());
	tagger->UpdateFilter(this);
}

void FilterItem::UpdateColor(BYTE c)
{
	Color = c;
	GetCurrentHost()->RefreshView();
}

} // Js

