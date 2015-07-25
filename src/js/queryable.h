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

#include "strstr.h"
#include "stringref.h"
#include "objectwrap.h"
#include "queryop.h"

class CBitSet;

namespace Js {

class Queryable : public BaseObject<Queryable>
{
public:
	enum OP
	{
		MAP,
		WHERE,
		PAIR
	};

	static void Init(v8::Isolate* iso);
	static Queryable * TryGetQueryable(const v8::Local<v8::Object> & obj);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	virtual const std::shared_ptr<QueryOp>& Op() = 0;
	virtual v8::Local<v8::Object> Source() = 0;
protected:
	Queryable(const v8::Handle<v8::Object>& handle)
	{
		Wrap(handle);
	}

	virtual size_t ComputeCount() = 0;

private:
	static void jsWhere(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsSelect(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsPair(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsCount(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsSkipWhile(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsTakeWhile(const v8::FunctionCallbackInfo<v8::Value> &args);
	// returns an element which matches condition
	static void jsFind(const v8::FunctionCallbackInfo<v8::Value> &args);
	// index. adds index to a query so find works faster
	static void jsIndex(const v8::FunctionCallbackInfo<v8::Value> &args);
	static v8::Handle<v8::Value> BuildWhereExpr(const v8::FunctionCallbackInfo<v8::Value> &args, OP op);

	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
};

}

