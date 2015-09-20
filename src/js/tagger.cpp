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
#include "tagger.h"
#include "objectwrap.h"
#include "error.h"
#include "log.h"
#include "filteritem.h"
#include "color.h"
#include "tracecollection.h"

using namespace v8;

namespace Js {

UniquePersistent<FunctionTemplate> Tagger::_Template;

void Tagger::Init(v8::Isolate* iso)
{
	LOG("");
	auto tmpl(FunctionTemplate::New(iso, jsNew));

	auto tmpl_proto = tmpl->InstanceTemplate();
	tmpl_proto->Set(String::NewFromUtf8(iso, "addFilter"), FunctionTemplate::New(iso, jsAddFilter));
	tmpl_proto->Set(String::NewFromUtf8(iso, "filterinteractive"), FunctionTemplate::New(iso, jsFilterInteractive));
	tmpl_proto->Set(String::NewFromUtf8(iso, "printFilters"), FunctionTemplate::New(iso, jsPrintFilters));
	tmpl_proto->Set(String::NewFromUtf8(iso, "enableFilter"), FunctionTemplate::New(iso, jsEnableFilter));
	tmpl_proto->Set(String::NewFromUtf8(iso, "removeFilter"), FunctionTemplate::New(iso, jsRemoveFilter));
	tmpl_proto->Set(String::NewFromUtf8(iso, "removeAllFilters"), FunctionTemplate::New(iso, jsRemoveAllFilters));
	tmpl_proto->Set(String::NewFromUtf8(iso, "asCollection"), FunctionTemplate::New(iso, jsAsCollection));
	tmpl_proto->Set(String::NewFromUtf8(iso, "onChanged"), FunctionTemplate::New(iso, jsOnChanged));

	tmpl->InstanceTemplate()->SetInternalFieldCount(1);
	tmpl->SetClassName(String::NewFromUtf8(iso, "Tagger"));
	_Template.Reset(iso, tmpl);
}

Tagger::Tagger(const v8::Handle<v8::Object>& handle)
{
	Wrap(handle);
}

void Tagger::jsNew(const FunctionCallbackInfo<Value> &args)
{
	Tagger *tagger = new Tagger(args.This());
	GetCurrentHost()->OnTaggerCreated(tagger);

	args.GetReturnValue().Set(args.This());
}

void Tagger::jsAddFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]
	{
		auto pThis = Unwrap(args.This());
		return pThis->AddFilterWorker(args, false);
	});
}

void Tagger::jsFilterInteractive(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]
	{
		auto pThis = Unwrap(args.This());
		return pThis->AddFilterWorker(args, true);
	});
}

void Tagger::jsPrintFilters(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());
		assert(args.Length() == 0);
		LOG("@%p", pThis);

		std::stringstream ss;
		ss << "Filters:\r\n";
		for (auto it = pThis->_Filters.begin(); it != pThis->_Filters.end(); it++)
		{
			FilterItem* filter = it->second.get();
			ss << (*it).first << " " << CColor::Name(filter->Color) <<
				" name:" << filter->Name <<
				" description: \"" << filter->Description << "\"\r\n";
		}
		GetCurrentHost()->OutputLine(ss.str().c_str());
		return Local<Value>();
	});
}

void Tagger::jsRemoveFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());
		assert(args.Length() == 1);

		int id;
		if (args.Length() == 1 && !args[0]->IsInt32())
		{
			id = args[0]->Int32Value();
		}
		else if (args.Length() == 1 && !args[0]->IsString())
		{
			auto strId = *String::Utf8Value(args[0]->ToString());
			id = atoi(strId);
		}
		else
		{
			ThrowSyntaxError("expected $v.removeFilter(id)\r\n");
		}

		auto it = pThis->_Filters.find(id);
		if (it != pThis->_Filters.end())
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

		pThis->InvokeOnChanged();
		GetCurrentHost()->RefreshView();
		return Local<Value>();
	});
}

void Tagger::jsRemoveAllFilters(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());
		assert(args.Length() == 1);

		if (args.Length() != 0)
		{
			ThrowSyntaxError("expected $v.removeAllFilters()\r\n");
		}

		pThis->_Filters.clear();

		GetCurrentHost()->RefreshView();
		return Local<Value>();
	});
}

void Tagger::jsEnableFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());
		assert(args.Length() == 2);

		if (args.Length() != 2 || !args[0]->IsInt32() || !args[1]->IsBoolean())
		{
			ThrowSyntaxError("expected $v.enableFilter(id, val)\r\n");
		}

		auto id = args[0]->Int32Value();
		auto val = args[1]->BooleanValue();
		auto it = pThis->_Filters.find(id);
		if (it != pThis->_Filters.end())
		{
			FilterItem* filter = it->second.get();
			if (filter->Enable != val)
			{
				filter->Enable = val;
				// GetCurrentHost()->UpdateLinesActive(filter->Set, (val) ? 1 : -1);
				assert(false);

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

void Tagger::jsOnChanged(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());

		if (!(args.Length() == 1 && args[0]->IsFunction()))
		{
			ThrowSyntaxError("expected $.tagger.onChanged(function)\r\n");
		}
		pThis->_OnChanged.Reset(Isolate::GetCurrent(), args[0].As<Function>());

		return Local<Value>();
	});
}

void Tagger::jsAsCollection(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());
		auto collJs(TraceCollection::GetTemplate(Isolate::GetCurrent())->GetFunction()->NewInstance());
		auto coll = TraceCollection::Unwrap(collJs);

		// args.GetReturnValue().Set();
		for (auto& flt : pThis->_Filters)
		{
			coll->AddLines(*(flt.second->Set));
		}

		args.GetReturnValue().Set(collJs);
		return Local<Value>();
	});
}

void Tagger::OnTraceSourceChanged()
{
	for (auto& filter : _Filters)
	{
		UpdateFilter(filter.second.get());
	}
}

void Tagger::UpdateFilter(FilterItem* filter)
{
	LOG("@%p", this);

	auto filterFunc = Local<Object>::New(Isolate::GetCurrent(), filter->FilterFunc);
	// now we actually run the function
	//auto selectFunc(Local<Object>::New(Isolate::GetCurrent(), sel->SelectFunc));
	Query * pQuery = Query::TryGetQuery(filterFunc);
	Local<Object> collJs;
	if (pQuery)
	{
		LOG("@%p input is query. Compute", this);
		filter->Description = pQuery->MakeDescription();
		collJs = pQuery->GetCollection();
	}
	else
	{
		collJs = filterFunc;
	}

	// update collection on filter
	filter->Collection.Reset(Isolate::GetCurrent(), collJs.As<Object>());

	// update items
	TraceCollection* pColl = TraceCollection::TryGetCollection(collJs);
	if (pColl)
	{
		LOG("@%p input is collection", this);

		// remove lines which selected now
		if (filter->Set && filter->Set->GetSetBitCount() != 0)
		{
			// GetCurrentHost()->UpdateLinesActive(filter->Set, -1);
			assert(false);
		}

		filter->Set = pColl->GetLines();

		// save reference to collection and register for notifications
		std::weak_ptr<FilterItem> weakFilter(filter->shared_from_this());
		pColl->SetChangeListener([this, weakFilter](TraceCollection* pColl, const std::shared_ptr<CBitSet>& oldSet, const std::shared_ptr<CBitSet>& newSet)
		{
			auto filter(weakFilter.lock());
			if (filter == nullptr)
				return;

			//GetCurrentHost()->UpdateLinesActive(oldSet, -1);
			//GetCurrentHost()->UpdateLinesActive(newSet, 1);
			filter->Set = pColl->GetLines();
			GetCurrentHost()->RefreshView();
		});

		// we do not know if we already displaying this collection
		filter->Set = pColl->GetLines();

		InvokeOnChanged();
	}
	else
	{
		ThrowError("unknown argument type");
	}
}

void Tagger::InvokeOnChanged()
{
	if (_OnChanged.IsEmpty())
		return;

	TryCatch try_catch;

	Local<Function> onChanged(Local<Function>::New(Isolate::GetCurrent(), _OnChanged));
	onChanged->Call(v8::Isolate::GetCurrent()->GetCurrentContext()->Global(), 0, nullptr);

	if(try_catch.HasCaught())
	{
		GetCurrentHost()->ReportException(Isolate::GetCurrent(), try_catch);
	}
}

void Tagger::VerifySelectArgs(const FunctionCallbackInfo<Value>& args)
{
	if (args.Length() < 2)
	{
		ThrowSyntaxError("expected select(filter, color, name\r\n");
	}

	if (!args[0]->IsObject())
	{
		std::stringstream ss;

		ss << "first parameter has to be a query object or an iterator\r\n";

		auto s = args[0]->ToString();
		String::Utf8Value str(s);
		ss << (*str);

		ThrowSyntaxError(ss.str().c_str());
	}

	if (args.Length() >= 2)
	{
		if (!args[1]->IsString())
		{
			std::stringstream ss;

			ss << "second parameter is color\r\n";
			ss << *String::Utf8Value(args[1]->ToString());

			ThrowSyntaxError(ss.str().c_str());
		}
	}

	if (args.Length() >= 3)
	{
		if (!(args[2]->IsUndefined() || args[2]->IsNull() || args[2]->IsString()))
		{
			std::stringstream ss;

			ss << "third parameter is description\r\n";
			ss << *String::Utf8Value(args[2]->ToString());

			ThrowSyntaxError(ss.str().c_str());
		}
	}
}

Local<Value> Tagger::AddFilterWorker(const FunctionCallbackInfo<Value>& args, bool iter)
{
	LOG("@%p iter=%d", this, (int) iter);
	VerifySelectArgs(args);

	auto filter = std::make_shared<FilterItem>(args.This());

	auto selectFunc(args[0].As<Object>());

	// allocate ID
	int id = 1;
	for (id = 1;; id++)
	{
		if (_Filters.find(id) == _Filters.end())
		{
			break;
		}
	}
	filter->Id = id;

	// check if we have color
	if (args.Length() >= 2)
	{
		filter->Color = CColor::FromName(*String::Utf8Value(args[1]->ToString()));
	}

	if (args.Length() >= 3)
	{
		filter->Name = *String::Utf8Value(args[2]->ToString());
	}

	filter->FilterFunc.Reset(Isolate::GetCurrent(), args[0].As<Object>());
	UpdateFilter(filter.get());

	// report to host
	_Filters[id] = filter;
	GetCurrentHost()->RefreshView();

	std::stringstream ss;
	ss << "Add filter id=" << id << "\r\n";
	GetCurrentHost()->OutputLine(ss.str().c_str());

	// wrap filter into filter proxy and return
	return FilterItemProxy::CreateFromFilter(filter);
}

BYTE Tagger::GetLineColor(DWORD nLine)
{
	std::lock_guard<std::mutex> guard(_Lock);

	// go through all filters and read line
	for (auto it = _Filters.begin(); it != _Filters.end(); ++it)
	{
		if (it->second->Enable && it->second->IsLineSelected(nLine))
		{
			return it->second->Color;
		}
	}

	return CColor::DEFAULT_TEXT;
}


}

