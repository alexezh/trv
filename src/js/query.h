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

class CBitSet;

namespace Js {

// query is entry function for query language
// defines set of methods such as
//   where
//   or
//   and
//   not
//   map
class Query : public ObjectWrap
{
public:
	enum OP
	{
		MAP,
		WHERE,
		PAIR,
		AND,
		OR
	};

	Query(const v8::Handle<v8::Object>& handle, const v8::Arguments &args);
	~Query()
	{
	}

	static void Init();

	static v8::Persistent<v8::FunctionTemplate> & GetTemplate() { return _Template; }

	// returns native Filter object (or null)
	static Query * TryGetQuery(const v8::Persistent<v8::Object> & obj);

	// returns a description for query
	std::string MakeDescription();

	const std::shared_ptr<QueryOp>& Op() { return _Op; }

private:
	static v8::Handle<v8::Value> jsNew(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsOr(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsAnd(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsWhere(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsMap(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsPair(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsCount(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsSkipWhile(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsTakeWhile(const v8::Arguments &args);
	// returns an element which matches condition
	static v8::Handle<v8::Value> jsFind(const v8::Arguments &args);
	// index. adds index to a query so find works faster
	static v8::Handle<v8::Value> jsIndex(const v8::Arguments &args);
	static v8::Handle<v8::Value> BuildWhereExpr(const v8::Arguments &args, Query::OP op);

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;

	v8::Persistent<v8::Object> _Source;
	// filter is either string expression or an object
	std::shared_ptr<QueryOp> _Op;
};

} // Js
