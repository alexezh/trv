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
#include "apphost.h"
#include "query.h"
#include "trace.h"
#include "color.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> FilterItem::_Template;
Persistent<FunctionTemplate> SelectCursor::_Template;
Persistent<FunctionTemplate> View::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void FilterItem::Init(Isolate* iso, Local<Object> & target)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	tmpl->SetClassName(v8::String::NewFromUtf8(iso, "Filter"));
	_Template.Reset(iso, tmpl);
}

void FilterItem::jsNew(const FunctionCallbackInfo<Value> &args)
{
	FilterItem *item = new FilterItem();
	args.This()->SetInternalField(0, External::New(Isolate::GetCurrent(), item));

	args.GetReturnValue().Set(args.This());
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

	if(pThis->_CurPos+1 == pThis->_Cache.size())
	{
		if(pThis->_Iter->IsEnd())
		{
			return;
		}

		if(!pThis->_Iter->Next())
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

	if(pThis->_CurPos == 0)
	{
		return;
	}
	pThis->_CurPos--;
	pThis->UpdateFilter();
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
	HandleScope scope(Isolate::GetCurrent());
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
			auto rangeJs = objRes->FindInstanceInPrototypeChain(TraceRange::GetTemplate(Isolate::GetCurrent()));
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

void View::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	auto tmpl_proto = tmpl->InstanceTemplate();
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "filter"), FunctionTemplate::New(iso, jsFilter));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "filterinteractive"), FunctionTemplate::New(iso, jsFilterInteractive));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "printfilters"), FunctionTemplate::New(iso, jsPrintFilters));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "enablefilter"), FunctionTemplate::New(iso, jsEnableFilter));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "removefilter"), FunctionTemplate::New(iso, jsRemoveFilter));
	tmpl->InstanceTemplate()->Set(String::NewFromUtf8(iso, "setviewlayout"), FunctionTemplate::New(iso, jsSetViewLayout));
	tmpl->InstanceTemplate()->SetAccessor(String::NewFromUtf8(iso, "showfilters"), jsShowFiltersGetter, jsShowFiltersSetter);
	tmpl->InstanceTemplate()->SetAccessor(String::NewFromUtf8(iso, "currentline"), jsCurrentLineGetter);

	_Template.Reset(iso, tmpl);

	SelectCursor::Init(iso);
}

void View::jsNew(const FunctionCallbackInfo<Value> &args)
{
	View *view = new View(args.This());
	GetCurrentHost()->OnViewCreated(view);

	args.GetReturnValue().Set(args.This());
}

void View::jsFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args] 
	{
		auto pThis = UnwrapThis<View>(args.This());
		return pThis->FilterWorker(args, false);
	});
}

void View::jsFilterInteractive(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args] 
	{
		auto pThis = UnwrapThis<View>(args.This());
		return pThis->FilterWorker(args, true);
	});
}

void View::jsPrintFilters(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 0);
		LOG("@%p", pThis);

		std::stringstream ss;
		ss << "Filters:\r\n";
		for(auto it = pThis->_Filters.begin(); it != pThis->_Filters.end(); it++)
		{
			ss << (*it).first << " " << CColor::Name((*it).second->Color) << 
				" " << (*it).second->Name << 
				" " << (*it).second->Description << "\r\n";
		}
		GetCurrentHost()->OutputLine(ss.str().c_str());
		return Local<Value>();
	});
}

void View::jsRemoveFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args] () -> Local<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 1);

		if(args.Length() != 1 || !args[0]->IsInt32())
		{
			ThrowSyntaxError("expected $v.removeFilter(id)\r\n");
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
		return Local<Value>();
	});
}

void View::jsEnableFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args] () -> Local<Value>
	{
		auto pThis = UnwrapThis<View>(args.This());
		assert(args.Length() == 2);

		if(args.Length() != 2 || !args[0]->IsInt32() || !args[1]->IsBoolean())
		{
			ThrowSyntaxError("expected $v.enableFilter(id, val)\r\n");
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
		return Local<Value>();
	});
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

void View::VerifySelectArgs(const FunctionCallbackInfo<Value>& args)
{
	if(args.Length() < 2)
	{
		ThrowSyntaxError("expected select(filter, color, name\r\n");
	}

	if(!args[0]->IsObject())
	{
		std::stringstream ss;

		ss << "first parameter has to be a query object or an iterator\r\n";

		auto s = args[0]->ToString();
		String::Utf8Value str(s);
		ss << (*str);

		ThrowSyntaxError(ss.str().c_str());
	}

	if(args.Length() >= 2)
	{
		if(!args[1]->IsString())
		{
			std::stringstream ss;

			ss << "second parameter is color\r\n";
			ss << *String::Utf8Value(args[1]->ToString());

			ThrowSyntaxError(ss.str().c_str());
		}
	}

	if(args.Length() >= 3)
	{
		if(!(args[2]->IsUndefined() || args[2]->IsNull() || args[2]->IsString()))
		{
			std::stringstream ss;

			ss << "third parameter is description\r\n";
			ss << *String::Utf8Value(args[2]->ToString());

			ThrowSyntaxError(ss.str().c_str());
		}
	}
}

Local<Value> View::FilterWorker(const FunctionCallbackInfo<Value>& args, bool iter)
{
	LOG("@%p", this);
	VerifySelectArgs(args);

	auto sel = std::make_shared<FilterItem>();

	sel->SelectFunc.Reset(Isolate::GetCurrent(), args[0].As<Object>());

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

	Local<Value> res;

	// now we actually run the function
	auto selectFunc(Local<Object>::New(Isolate::GetCurrent(), sel->SelectFunc));
	Query * pQuery = Query::TryGetQuery(selectFunc);
	if(pQuery)
	{
		if(!iter)
		{
			std::string startMsg;
			startMsg = std::string("Start query: ") + pQuery->MakeDescription() + "\r\n";

			GetCurrentHost()->OutputLine(startMsg.c_str());

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
			
			auto curJs = SelectCursor::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance();
			SelectCursor* cur = UnwrapThis<SelectCursor>(curJs);
			cur->InitCursor(sel, std::move(it));

			sel->Cursor.Reset(Isolate::GetCurrent(), curJs);
			res = Local<Value>::New(Isolate::GetCurrent(), curJs);
		}
	}
	else
	{
		ThrowError("failed to run query");
	}

	// report to host
	_Filters[id] = std::move(sel);
	GetCurrentHost()->RefreshView();

	return std::move(res);
}

void View::ShowFiltersWorker(bool val)
{
	LOG("@%p show=%d", this, (int)val);
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

