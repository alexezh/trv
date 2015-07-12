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

namespace Js {

// view should have format registry which is identified by id
class FilterItem
{
public:
	static void Init(v8::Isolate* iso, v8::Handle<v8::Object> & target);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	FilterItem()
		: Id(0)
		, Color(0)
		, Enable(true)
	{
	}

	~FilterItem()
	{
		Cursor.Reset();
	}


	int Id;
	v8::Persistent<v8::Object> SelectFunc;
	BYTE Color;
	bool Enable;
	std::string Name;
	std::string Description;
	std::shared_ptr<CBitSet> Set;
	v8::Persistent<v8::Object> Cursor;
	std::mutex Lock;

	bool IsLineSelected(DWORD nLine)
	{
		std::lock_guard<std::mutex> guard(Lock);
		return Set->GetBit(nLine);
	}

private:
	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static v8::Persistent<v8::FunctionTemplate> _Template;
};

// iterate through select
class SelectCursor : public BaseObject<SelectCursor>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	void InitCursor(const std::weak_ptr<FilterItem>& sel, std::unique_ptr<QueryIterator>&& it)
	{
		_Sel = sel;
		_Iter = std::move(it);
		UpdateFilter();
	}

private:
	SelectCursor(const v8::Handle<v8::Object>& handle);
	void UpdateFilter();

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsNext(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsPrev(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsFirst(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsLast(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
	std::unique_ptr<QueryIterator> _Iter;
	std::weak_ptr<FilterItem> _Sel;

	// cache values so prev works
	std::vector<std::shared_ptr<CBitSet> > _Cache;

	// current position of the cursor
	// if current position is less than _Cache.size, we take from cache
	size_t _CurPos;
};

class QueryIteratorHelper
{
public:
	static void SelectLinesFromIteratorValue(QueryIterator* it, CBitSet& set);
};

class View : public BaseObject<View>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso) 
	{ 
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	BYTE GetLineColor(DWORD nLine);

private:
	View(const v8::Handle<v8::Object>& handle);

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsFilter(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsFilterInteractive(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsRemoveFilter(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsPrintFilters(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsEnableFilter(const v8::FunctionCallbackInfo<v8::Value>& args);

	static void jsShowFiltersGetter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsShowFiltersSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value,	const v8::PropertyCallbackInfo<void>& info);

	static void jsCurrentLineGetter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);

	static void jsSetViewLayout(const v8::FunctionCallbackInfo<v8::Value>& args);

	void VerifySelectArgs(const v8::FunctionCallbackInfo<v8::Value>& args);
	v8::Local<v8::Value> FilterWorker(const v8::FunctionCallbackInfo<v8::Value>& args, bool iter);
	void ShowFiltersWorker(bool val);

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
	bool _ShowFilters;
	std::mutex _Lock;
	std::map<int, std::shared_ptr<FilterItem> > _Filters;
};

} // Js

