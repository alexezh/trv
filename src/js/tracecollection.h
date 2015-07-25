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
#include "bitset.h"
#include "objectwrap.h"
#include "queryable.h"

class CBitSet;

namespace Js {

// observable collection of trace lines
class TraceCollection : public Queryable
{
public:
	static void Init(v8::Isolate* iso);
	static void InitInstance(v8::Isolate* iso, v8::Handle<v8::Object> & target);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	static TraceCollection* TryGetCollection(const v8::Local<v8::Object> & obj);

	static inline TraceCollection* Unwrap(v8::Handle<v8::Object> handle)
	{
		return static_cast<TraceCollection*>(Queryable::Unwrap(handle));
	}

	const std::shared_ptr<CBitSet>& GetLines() { return _Lines; }

	using ChangeListener = std::function<void(TraceCollection* pSender, CBitSet& old, CBitSet& cur)>;
	void SetChangeListener(const ChangeListener& listner);

	const std::shared_ptr<QueryOp>& Op() override { return _Op; }
	v8::Local<v8::Object> Source() override { return Handle(); }

protected:
	size_t TraceCollection::ComputeCount() override;

private:
	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsAddLine(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsRemoveLine(const v8::FunctionCallbackInfo<v8::Value> &args);

	TraceCollection(const v8::Handle<v8::Object>& handle, DWORD lineCount);
	TraceCollection(const v8::Handle<v8::Object>& handle, DWORD start, DWORD end);
	static bool ValueToLineIndex(v8::Local<v8::Value>& v, DWORD& idx);

	void AddLine(DWORD dwLine);
	void RemoveLine(DWORD dwLine);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	// for small sets it is better to use array
	// for bigger sets - bitset. Ideally it would be nice to have compressed
	// bitset where big ranges are collapsed
	std::shared_ptr<CBitSet> _Lines;
	ChangeListener _Listener;

	// operation exposing collection as source
	std::shared_ptr<QueryOp> _Op;
};

} // Js
