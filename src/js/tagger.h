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

#include "objectwrap.h"
#include "tracecollection.h"

namespace Js {

// TODO: I need tagger as an array of Collection items to responde to GetColor requests
// Everything else can be done in JS including filter item
class Tagger : public BaseObject<Tagger>
{
public:
	static void Init(v8::Isolate* iso);
	static void InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target);
	static v8::Local<v8::FunctionTemplate> GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	void OnTraceSourceChanged();
	BYTE GetLineColor(DWORD nLine);

private:
	Tagger(const v8::Handle<v8::Object>& handle);

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsAddFilter(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void jsRemoveFilter(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	std::mutex _Lock;

	struct Item
	{
		Item(const v8::Local<v8::Object>& collJs, TraceCollection* coll, uint8_t color)
		{
			CollJs.Reset(v8::Isolate::GetCurrent(), collJs);
			Coll = coll;
			Color = color;
		}
		Item(Item&& other)
			: CollJs(std::move(other.CollJs))
			, Coll(other.Coll)
			, Color(other.Color)
		{
		}
		Item& operator=(Item&& other)
		{
			if (this == &other)
				return *this;

			CollJs = std::move(other.CollJs);
			Coll = other.Coll;
			Color = other.Color;
			return *this;
		}
		v8::UniquePersistent<v8::Object> CollJs;
		TraceCollection* Coll;
		uint8_t Color;
	};

	std::vector<Item> _Filters;
	v8::Persistent<v8::Function> _OnChanged;
};


}