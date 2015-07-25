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
#include "lineinfo.h"
#include "queryop.h"
#include "objectwrap.h"
#include "queryable.h"

class CBitSet;

namespace Js {

class QueryIteratorHelper
{
public:
	static void SelectLinesFromIteratorValue(QueryIterator* it, CBitSet& set);
};


// query is entry function for query language
// defines set of methods such as
//   where
//   or
//   and
//   not
//   map
class Query : public Queryable
{
public:
	Query(const v8::Handle<v8::Object>& handle, const v8::FunctionCallbackInfo<v8::Value> &args);
	~Query()
	{
	}

	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	// returns native Filter object (or null)
	static Query * TryGetQuery(const v8::Local<v8::Object> & obj);

	static inline Query* Unwrap(v8::Handle<v8::Object> handle)
	{
		return static_cast<Query*>(Queryable::Unwrap(handle));
	}

	// returns a description for query
	std::string MakeDescription();

	const std::shared_ptr<QueryOp>& Op() override { return _Op; }
	v8::Local<v8::Object> Source() override { return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), _Source); }

	// generate collection from query
	v8::Local<v8::Object> GetCollection();

protected:
	size_t ComputeCount() override;

private:
	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);

	static v8::UniquePersistent<v8::FunctionTemplate> _Template;

	v8::Persistent<v8::Object> _Source;
	// filter is either string expression or an object
	std::shared_ptr<QueryOp> _Op;
};

} // Js
