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
#include "view.h"
#include "funcwrap.h"
#include "apphost.h"
#include "query.h"
#include "trace.h"
#include "color.h"
#include "error.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> FilterItem::_Template;
Persistent<FunctionTemplate> SelectCursor::_Template;
Persistent<FunctionTemplate> View::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void FilterItem::Init(Handle<Object> & target)
{
	_Template = Persistent<FunctionTemplate>::New(FunctionTemplate::New(&jsNew));
	_Template->InstanceTemplate()->SetInternalFieldCount(1);
	_Template->SetClassName(v8::String::New("Filter"));
}

Handle<Value> FilterItem::jsNew(const Arguments &args)
{
	FilterItem *item = new FilterItem();
	args.This()->SetInternalField(0, External::New(item));

	return args.This();
}

///////////////////////////////////////////////////////////////////////////////
//
SelectCursor::SelectCursor(const v8::Handle<v8::Object>& handle)
	: _CurPos(0)
{
	Wrap(handle);
}

void SelectCursor::Init()
{
	_Template = Persistent<FunctionTemplate>::New(FunctionTemplate::New(&jsNew));
	_Template->InstanceTemplate()->SetInternalFieldCount(1);
	_Template->InstanceTemplate()->Set(String::New("next"), FunctionTemplate::New(jsNext));
	_Template->InstanceTemplate()->Set(String::New("prev"), FunctionTemplate::New(jsPrev));
}

Handle<Value> SelectCursor::jsNew(const Arguments &args)
{
	SelectCursor *cursor = new SelectCursor(args.This());
	return args.This();
}

Handle<Value> SelectCursor::jsNext(const Arguments& args)
{
	auto pThis = UnwrapThis<SelectCursor>(args.This());

	if(pThis->_CurPos+1 == pThis->_Cache.size())
	{
		if(pThis->_Iter->IsEnd())
		{
			return Undefined();
		}

		if(!pThis->_Iter->Next())
		{
			return Undefined();
		}

		pThis->_CurPos++;
		pThis->UpdateFilter();
	}
	else
	{
		pThis->_CurPos++;
		pThis->UpdateFilter();
	}

	return Undefined();
}

Handle<Value> SelectCursor::jsPrev(const Arguments& args)
{
	auto pThis = UnwrapThis<SelectCursor>(args.This());

	if(pThis->_CurPos == 0)
	{
		return Undefined();
	}
	pThis->_CurPos--;
	pThis->UpdateFilter();

	return Undefined();
}

void SelectCursor::UpdateFilter()
{
	auto sel = _Sel.lock();
	if(!sel)
	{
		return;
	}

	// undo previous Filter
	if(sel->Set)
	{
		GetCurrentHost()->UpdateLinesActive(*sel->Set, -1);
	}

	if(_CurPos == _Cache.size())
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
	GetCurrentHost()->UpdateLinesActive(*sel->Set, 1);

	// echo to output
	std::stringstream ss;
	ss << "Add Filters id=" << sel->Id << " match=" << sel->Set->GetSetBitCount() << "\r\n";
	GetCurrentHost()->OutputLine(ss.str().c_str());
}

///////////////////////////////////////////////////////////////////////////////
//
void QueryIteratorHelper::SelectLinesFromIteratorValue(QueryIterator* it, CBitSet& set)
{
	HandleScope scope;
	if(it->IsNative())
	{
		set.SetBit(it->NativeValue().Index);
	}
	else
	{
		auto res = it->JsValue();

		// if result is line, return it
		// if result is line set, return it
		// otherwise fail
		if(res->IsInt32())
		{
			set.SetBit(res->ToInteger()->Int32Value());
		}
		else if(res->IsObject())
		{
			Handle<Object> objRes = res.As<Object>();
			auto rangeJs = objRes->FindInstanceInPrototypeChain(TraceRange::GetTemplate());
			if(rangeJs.IsEmpty())
			{
				throw V8RuntimeException("Only tracerange is supported");
			}

			TraceRange * range = UnwrapThis<TraceRange>(rangeJs);
			range->GetLines(set);
		}
		else
		{
			throw V8RuntimeException("Only int or tracerange is supported");
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
View::View(const v8::Handle<v8::Object>& handle)
	: _ShowFilters(true)
{
	Wrap(handle);
}

void View::Init()
{
	_Template = Persistent<FunctionTemplate>::New(FunctionTemplate::New(&jsNew));
	_Template->InstanceTemplate()->SetInternalFieldCount(1);
	_Template->InstanceTemplate()->Set(String::New("filter"), FunctionTemplate::New(jsFilter));
	_Template->InstanceTemplate()->Set(String::New("filterinteractive"), FunctionTemplate::New(jsFilterInteractive));
	_Template->InstanceTemplate()->Set(String::New("printfilters"), FunctionTemplate::New(jsPrintFilters));
	_Template->InstanceTemplate()->Set(String::New("enablefilter"), FunctionTemplate::New(jsEnableFilter));
	_Template->InstanceTemplate()->Set(String::New("removefilter"), FunctionTemplate::New(jsRemoveFilter));
	_Template->InstanceTemplate()->Set(String::New("setviewlayout"), FunctionTemplate::New(jsSetViewLayout));
	_Template->InstanceTemplate()->SetAccessor(String::New("globalfilter"), jsGlobalFilterGetter, jsGlobalFilterSetter);
	_Template->InstanceTemplate()->SetAccessor(String::New("showfilters"), jsShowFiltersGetter, jsShowFiltersSetter);

	SelectCursor::Init();
}

Handle<Value> View::jsNew(const Arguments &args)
{
	View *view = new View(args.This());
	GetCurrentHost()->OnViewCreated(view);

	return args.This();
}

Handle<Value> View::jsFilter(const Arguments& args)
{
	return TryCatchCpp([&args] 
	{
		auto pThis = UnwrapThis<View>(args.This());
		return pThis->FilterWorker(args, false);
	});
}

Handle<Value> View::jsFilterInteractive(const Arguments& args)
{
	return TryCatchCpp([&args] 
	{
		auto pThis = UnwrapThis<View>(args.This());
		return pThis->FilterWorker(args, true);
	});
}

Handle<Value> View::jsPrintFilters(const Arguments& args)
{
	return TryCatchCpp([&args] 
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 0);

		std::stringstream ss;
		ss << "Filters:\r\n";
		for(auto it = pThis->_Filters.begin(); it != pThis->_Filters.end(); it++)
		{
			ss << (*it).first << " " << CColor::Name((*it).second->Color) << 
				" " << (*it).second->Name << 
				" " << (*it).second->Description << "\r\n";
		}
		GetCurrentHost()->OutputLine(ss.str().c_str());

		return Undefined();
	});
}

Handle<Value> View::jsRemoveFilter(const Arguments& args)
{
	return TryCatchCpp([&args] () -> Handle<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 1);

		if(args.Length() != 1 || !args[0]->IsInt32())
		{
			return ThrowException(Exception::SyntaxError(String::New(
					"expected $v.removeFilter(id)\r\n")));
		}

		auto id = args[0]->Int32Value();
		auto it = pThis->_Filters.find(id);
		if(it != pThis->_Filters.end())
		{
			pThis->_Filters.erase(it);

			std::stringstream ss;
			ss << "Remove Filters id=" << id << "\r\n";
			GetCurrentHost()->OutputLine(ss.str().c_str());
		}
		else
		{
			std::stringstream ss;
			ss << "Filter id=" << id << " not found\r\n";
			GetCurrentHost()->OutputLine(ss.str().c_str());
		}

		GetCurrentHost()->RefreshView();

		return Undefined();
	});
}

Handle<Value> View::jsEnableFilter(const Arguments& args)
{
	return TryCatchCpp([&args] () -> Handle<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 2);

		if(args.Length() != 2 || !args[0]->IsInt32() || !args[1]->IsBoolean())
		{
			return ThrowException(Exception::SyntaxError(String::New(
					"expected $v.enableFilter(id, val)\r\n")));
		}

		auto id = args[0]->Int32Value();
		auto val = args[1]->BooleanValue();
		auto it = pThis->_Filters.find(id);
		if(it != pThis->_Filters.end())
		{
			if((*it).second->Enable != val)
			{
				(*it).second->Enable = val;
				GetCurrentHost()->UpdateLinesActive(*(*it).second->Set, (val) ? 1 : -1);

				std::stringstream ss;
				ss << "Filters id=" << id << " " << ((val) ? "enabled" : "disabled") << "\r\n";
				GetCurrentHost()->OutputLine(ss.str().c_str());
			}
		}
		else
		{
			std::stringstream ss;
			ss << "Filter id=" << id << " not found\r\n";
			GetCurrentHost()->OutputLine(ss.str().c_str());
		}

		GetCurrentHost()->RefreshView();

		return Undefined();
	});
}

Handle<Value> View::jsSetViewLayout(const Arguments& args)
{
	return TryCatchCpp([&args] () -> Handle<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 2);

		if(args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsNumber())
		{
			return ThrowException(Exception::SyntaxError(String::New(
					"expected $v.setviewlayout(cmdheight, outheight)\r\n")));
		}

		GetCurrentHost()->SetViewLayout(args[0]->NumberValue(), args[1]->NumberValue());
		return Undefined();
	});
}

Handle<Value> View::jsShowFiltersGetter(Local<String> property, 
											const AccessorInfo& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	return Boolean::New(pThis->_ShowFilters);
}

void View::jsShowFiltersSetter(Local<String> property, Local<Value> value,
							const AccessorInfo& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	pThis->ShowFiltersWorker(value->BooleanValue());
}

Handle<Value> View::jsGlobalFilterGetter(Local<String> property, 
											const AccessorInfo& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	return pThis->_GlobalFilter;
}

void View::jsGlobalFilterSetter(Local<String> property, Local<Value> value,
							const AccessorInfo& info)
{
	auto pThis = UnwrapThis<View>(info.This());
	pThis->_GlobalFilter = Persistent<Object>(value->ToObject());
}

Handle<Value> View::VerifySelectArgs(const Arguments& args)
{
	if(args.Length() < 2)
	{
		return ThrowException(Exception::SyntaxError(String::New(
				"expected select(filter, color, name\r\n")));
	}

	if(!args[0]->IsObject())
	{
		std::stringstream ss;

		ss << "first parameter has to be a query object or an iterator\r\n";

		auto s = args[0]->ToString();
		String::Utf8Value str(s);
		ss << (*str);

		return ThrowException(Exception::SyntaxError(String::New(
				ss.str().c_str())));
	}

	if(args.Length() >= 2)
	{
		if(!args[1]->IsString())
		{
			std::stringstream ss;

			ss << "second parameter is color\r\n";
			ss << *String::Utf8Value(args[1]->ToString());

			return ThrowException(Exception::SyntaxError(String::New(
					ss.str().c_str())));
		}
	}

	if(args.Length() >= 3)
	{
		if(!args[2]->IsString())
		{
			std::stringstream ss;

			ss << "third parameter is description\r\n";
			ss << *String::Utf8Value(args[2]->ToString());

			return ThrowException(Exception::SyntaxError(String::New(
					ss.str().c_str())));
		}
	}

	return Undefined();
}

Handle<Value> View::FilterWorker(const Arguments& args, bool iter)
{
	auto verResult = VerifySelectArgs(args);
	if(!verResult->IsUndefined())
	{
		return verResult;
	}

	auto sel = std::make_shared<FilterItem>();

	sel->SelectFunc = Persistent<Object>::New(args[0].As<Object>());

	// allocate ID
	int id = 1;
	for(id = 1;;id++)
	{
		if(_Filters.find(id) == _Filters.end())
		{
			break;
		}
	}
	sel->Id = id;

	// check if we have color
	if(args.Length() >= 2)
	{
		sel->Color = CColor::FromName(*String::Utf8Value(args[1]->ToString()));
	}

	if(args.Length() >= 3)
	{
		sel->Name = *String::Utf8Value(args[2]->ToString());
	}

	Handle<Value> res(Undefined());

	// now we actually run the function
	Query * pQuery = Query::TryGetQuery(sel->SelectFunc);
	if(pQuery)
	{
		if(!iter)
		{
			GetCurrentHost()->OutputLine("Start query\r\n");

			DWORD dwStart = GetTickCount();
			{
				std::lock_guard<std::mutex> guard(sel->Lock);

				sel->Set = std::make_shared<CBitSet>();
				sel->Set->Init(GetCurrentHost()->GetLineCount());

				for(auto it = pQuery->Op()->CreateIterator(); !it->IsEnd(); it->Next())
				{
					QueryIteratorHelper::SelectLinesFromIteratorValue(it.get(), *sel->Set);
				}
			}
			DWORD dwEnd = GetTickCount();

			// mark lines as active
			GetCurrentHost()->UpdateLinesActive(*sel->Set, 1);
			char bitA[32];

			sel->Description = std::string("lines match: ") + 
					_itoa(sel->Set->GetSetBitCount(), bitA, 10) + 
					" query: " + pQuery->MakeDescription();

			// echo to output
			std::stringstream ss;
			ss << "Query execution time " << (dwEnd - dwStart) << "ms\r\n";
			ss << "Add Filters id=" << id << " match=" << sel->Set->GetSetBitCount() << "\r\n";
			GetCurrentHost()->OutputLine(ss.str().c_str());
		}
		else
		{
			sel->Description = std::string("interactive, query:") + pQuery->MakeDescription();
			auto it = pQuery->Op()->CreateIterator();
			
			auto curJs = SelectCursor::GetTemplate()->GetFunction()->NewInstance();
			SelectCursor* cur = UnwrapThis<SelectCursor>(curJs);
			cur->InitCursor(sel, std::move(it));

			sel->Cursor = Persistent<Object>::New(curJs);
			res = curJs;
		}
	}
	else
	{
		return ThrowException(Exception::Error(String::New(
				"failed to run query")));
	}

	// report to host
	_Filters[id] = std::move(sel);
	GetCurrentHost()->RefreshView();

	return res;
}

void View::ShowFiltersWorker(bool val)
{
	_ShowFilters = val;
	GetCurrentHost()->RefreshView();
}

BYTE View::GetLineColor(DWORD nLine)
{
	std::lock_guard<std::mutex> guard(_Lock);

	if(!_ShowFilters)
	{
		return CColor::DEFAULT_TEXT;
	}
	// go through all filters and read line
	for(auto it = _Filters.begin(); it != _Filters.end(); ++it)
	{
		if(it->second->Enable && it->second->IsLineSelected(nLine))
		{
			return it->second->Color;
		}
	}

	return CColor::DEFAULT_TEXT;
}

} // Js

