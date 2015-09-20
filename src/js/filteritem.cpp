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
Persistent<FunctionTemplate> SelectCursor::_Template;

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

///////////////////////////////////////////////////////////////////////////////
//
SelectCursor::SelectCursor(const v8::Handle<v8::Object>& handle)
	: _CurPos(0)
{
	Wrap(handle);
}

void SelectCursor::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "next"), FunctionTemplate::New(iso, jsNext));
	tmpl->PrototypeTemplate()->Set(String::NewFromUtf8(iso, "prev"), FunctionTemplate::New(iso, jsPrev));
	_Template.Reset(iso, tmpl);
}

void SelectCursor::jsNew(const FunctionCallbackInfo<Value> &args)
{
	SelectCursor *cursor = new SelectCursor(args.This());
	args.GetReturnValue().Set(args.This());
}

void SelectCursor::jsNext(const FunctionCallbackInfo<Value>& args)
{
	auto pThis = UnwrapThis<SelectCursor>(args.This());

	if (pThis->_CurPos + 1 == pThis->_Cache.size())
	{
		if (pThis->_Iter->IsEnd())
		{
			return;
		}

		if (!pThis->_Iter->Next())
		{
			return;
		}

		pThis->_CurPos++;
		pThis->UpdateFilter();
	}
	else
	{
		pThis->_CurPos++;
		pThis->UpdateFilter();
	}
}

void SelectCursor::jsPrev(const FunctionCallbackInfo<Value>& args)
{
	auto pThis = UnwrapThis<SelectCursor>(args.This());

	if (pThis->_CurPos == 0)
	{
		return;
	}
	pThis->_CurPos--;
	pThis->UpdateFilter();
}

void SelectCursor::UpdateFilter()
{
	auto sel = _Sel.lock();
	if (!sel)
	{
		return;
	}

	// undo previous Filter
	if (sel->Set)
	{
		// GetCurrentHost()->UpdateLinesActive(sel->Set, -1);
		assert(false);
	}

	if (_CurPos == _Cache.size())
	{
		// TODO: sparse bitset
		sel->Set = std::make_shared<CBitSet>();
		sel->Set->Init(GetCurrentHost()->GetLineCount());

		QueryIteratorHelper::SelectLinesFromIteratorValue(_Iter.get(), *sel->Set);

		// cache the result
		_Cache.push_back(sel->Set);
	}
	else
	{
		assert(_CurPos < _Cache.size());
		sel->Set = _Cache[_CurPos];
	}

	// mark lines as active
	// GetCurrentHost()->UpdateLinesActive(sel->Set, 1);
	assert(false);

	// echo to output
	std::stringstream ss;
	ss << "Add Filters id=" << sel->Id << " match=" << sel->Set->GetSetBitCount() << "\r\n";
	GetCurrentHost()->OutputLine(ss.str().c_str());
}

#if 0

Local<Value> Tagger::RunQuery(const std::shared_ptr<FilterItem>& filter, Query* pQuery, bool iter)
{
	Local<Value> res;
	if (!iter)
	{
		// mark lines as active
		// GetCurrentHost()->UpdateLinesActive(filter->Set, 1);
		assert(false);
		char bitA[32];

		filter->Description = std::string("lines match: ") +
			_itoa(filter->Set->GetSetBitCount(), bitA, 10) +
			" query: " + pQuery->MakeDescription();

	}
	else
	{
		filter->Description = std::string("interactive, query:") + pQuery->MakeDescription();
		auto it = pQuery->Op()->CreateIterator();

		auto curJs = SelectCursor::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance();
		SelectCursor* cur = UnwrapThis<SelectCursor>(curJs);
		cur->InitCursor(filter, std::move(it));

		filter->Cursor.Reset(Isolate::GetCurrent(), curJs);
		res = Local<Value>::New(Isolate::GetCurrent(), curJs);
	}

	return res;
}


#endif

} // Js

