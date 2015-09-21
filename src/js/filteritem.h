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
#include "js/query.h"

using namespace v8;

namespace Js {

// TODO: rename to TraceViewProxy
class View;

// view maintains list of filters created for tracesource
// each filter points to collection (which technically can change)
class FilterItem : public std::enable_shared_from_this<FilterItem>
{
public:
	FilterItem(Local<Object> view);

	int Id = 0;
	UniquePersistent<Object> FilterFunc;
	UniquePersistent<Object> Collection;
	BYTE Color = 0;
	bool Enable = true;
	std::string Name;
	std::string Description;
	std::shared_ptr<CBitSet> Set;
	UniquePersistent<Object> Cursor;
	std::mutex Lock;

	bool IsLineSelected(DWORD nLine)
	{
		std::lock_guard<std::mutex> guard(Lock);
		if (nLine >= Set->GetTotalBitCount())
			return false;

		return Set->GetBit(nLine);
	}

	void UpdateFilter(Local<Value> val);
	void UpdateColor(BYTE c);

private:
	static void WeakViewCallback(
		const v8::WeakCallbackData<v8::Object, FilterItem>& data);

	// weak reference to tagger
	Persistent<Object> _Tagger;
};

class FilterItemProxy : public BaseObject<FilterItemProxy>
{
public:
	static void Init(Isolate* iso);
	static Local<FunctionTemplate> GetTemplate(Isolate* iso)
	{
		return Local<FunctionTemplate>::New(iso, _Template);
	}

	static Local<Object> CreateFromFilter(const std::shared_ptr<FilterItem>& filter)
	{
		auto filterProxyJs = FilterItemProxy::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance();
		auto filterProxy = FilterItemProxy::Unwrap(filterProxyJs);
		filterProxy->_Filter = filter;
		return filterProxyJs;
	}

private:
	std::shared_ptr<FilterItem> _Filter;

	FilterItemProxy(const Local<Object>& handle)
	{
		Wrap(handle);
	}

	static void jsNew(const FunctionCallbackInfo<Value> &args);
	static void jsSourceGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
	static void jsSourceSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info);
	static void jsColorGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
	static void jsColorSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info);
	static void jsDescriptionGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
	static void jsDescriptionSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info);

	static Persistent<FunctionTemplate> _Template;
};

} // Js

