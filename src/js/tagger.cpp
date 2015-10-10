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
	tmpl_proto->Set(String::NewFromUtf8(iso, "removeFilter"), FunctionTemplate::New(iso, jsRemoveFilter));

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

static void ThrowAddFilter()
{
	ThrowSyntaxError("expected $v.addFilter(collection, color)\r\n");
}

void Tagger::jsAddFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]
	{
		auto pThis = Unwrap(args.This());
		std::lock_guard<std::mutex> guard(pThis->_Lock);
		if (!(args.Length() == 2 && args[0]->IsObject() && args[1]->IsString()))
		{
			ThrowAddFilter();
		}
		auto coll = TraceCollection::TryGetCollection(args[0].As<Object>());
		if (coll == nullptr)
		{
			ThrowAddFilter();
		}

		v8::String::Utf8Value strColor(args[1]);
		auto color = CColor::FromName(std::string(*strColor, strColor.length()).c_str());

		pThis->_Filters.push_back(Item(args[0].As<Object>(), coll, color));

		return Local<Value>();
	});
}

static void ThrowRemoveFilter()
{
	ThrowSyntaxError("expected $v.removeFilter(collection)\r\n");
}

void Tagger::jsRemoveFilter(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args]() -> Local<Value>
	{
		auto pThis = Unwrap(args.This());
		std::lock_guard<std::mutex> guard(pThis->_Lock);
		assert(args.Length() == 1);

		if (!args.Length() == 1 || !args[0]->IsInt32())
		{
			ThrowRemoveFilter();
		}
		auto coll = TraceCollection::TryGetCollection(args[0].As<Object>());
		if (coll == nullptr)
		{
			ThrowRemoveFilter();
		}

		std::remove_if(pThis->_Filters.begin(), pThis->_Filters.end(), [coll](const Item& item)
		{
			return item.Coll == coll;
		});

		return Local<Value>();
	});
}


void Tagger::OnTraceSourceChanged()
{
}

BYTE Tagger::GetLineColor(DWORD nLine)
{
	std::lock_guard<std::mutex> guard(_Lock);

	// go through all filters in reverse order and read line
	for (auto it = _Filters.rbegin(); it != _Filters.rend(); ++it)
	{
		if (it->Coll->GetLines()->GetBit(nLine))
		{
			return it->Color;
		}
	}

	return CColor::DEFAULT_TEXT;
}


}

