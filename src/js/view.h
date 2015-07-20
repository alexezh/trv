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
class FilterItem
{
public:
	FilterItem(Local<Object> view);

	int Id = 0;
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
		return Set->GetBit(nLine);
	}

	void UpdateCollection(Local<Value> val);

private:
	static void WeakViewCallback(
		const v8::WeakCallbackData<v8::Object, FilterItem>& data);

	// weak reference to view
	Persistent<Object> _View;
};

class FilterItemProxy : public BaseObject<FilterItemProxy>
{
public:
	static void Init(Isolate* iso);
	static Local<FunctionTemplate> & GetTemplate(Isolate* iso)
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

// iterate through select
class SelectCursor : public BaseObject<SelectCursor>
{
public:
	static void Init(Isolate* iso);
	static Local<FunctionTemplate> & GetTemplate(Isolate* iso)
	{
		return Local<FunctionTemplate>::New(iso, _Template);
	}

	void InitCursor(const std::weak_ptr<FilterItem>& sel, std::unique_ptr<QueryIterator>&& it)
	{
		_Sel = sel;
		_Iter = std::move(it);
		UpdateFilter();
	}

private:
	SelectCursor(const Local<Object>& handle);
	void UpdateFilter();

	static void jsNew(const FunctionCallbackInfo<Value> &args);
	static void jsNext(const FunctionCallbackInfo<Value>& args);
	static void jsPrev(const FunctionCallbackInfo<Value>& args);
	static void jsFirst(const FunctionCallbackInfo<Value>& args);
	static void jsLast(const FunctionCallbackInfo<Value>& args);

private:
	static Persistent<FunctionTemplate> _Template;
	std::unique_ptr<QueryIterator> _Iter;
	std::weak_ptr<FilterItem> _Sel;

	// cache values so prev works
	std::vector<std::shared_ptr<CBitSet> > _Cache;

	// current position of the cursor
	// if current position is less than _Cache.size, we take from cache
	size_t _CurPos;
};

class View : public BaseObject<View>
{
public:
	static void Init(Isolate* iso);
	static Local<FunctionTemplate> & GetTemplate(Isolate* iso) 
	{ 
		return Local<FunctionTemplate>::New(iso, _Template);
	}

	BYTE GetLineColor(DWORD nLine);

	void UpdateFilter(FilterItem* filter);

private:
	View(const Local<Object>& handle);

	static void jsNew(const FunctionCallbackInfo<Value> &args);
	static void jsFilter(const FunctionCallbackInfo<Value>& args);
	static void jsFilterInteractive(const FunctionCallbackInfo<Value>& args);
	static void jsRemoveFilter(const FunctionCallbackInfo<Value>& args);
	static void jsPrintFilters(const FunctionCallbackInfo<Value>& args);
	static void jsEnableFilter(const FunctionCallbackInfo<Value>& args);

	static void jsShowFiltersGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);
	static void jsShowFiltersSetter(Local<String> property, Local<Value> value,	const PropertyCallbackInfo<void>& info);

	static void jsCurrentLineGetter(Local<String> property, const PropertyCallbackInfo<Value>& info);

	static void jsSetViewLayout(const FunctionCallbackInfo<Value>& args);

	void VerifySelectArgs(const FunctionCallbackInfo<Value>& args);
	Local<Value> FilterWorker(const FunctionCallbackInfo<Value>& args, bool iter);
	void ShowFiltersWorker(bool val);
	Local<Value> RunQuery(const std::shared_ptr<FilterItem>& filter, Query* pQuery, bool iter);

private:
	static Persistent<FunctionTemplate> _Template;
	bool _ShowFilters;
	std::mutex _Lock;
	std::map<int, std::shared_ptr<FilterItem>> _Filters;
};

} // Js

