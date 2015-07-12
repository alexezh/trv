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

class CBitSet;

namespace Js {

class TraceLine : public BaseObject<TraceLine>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	const LineInfo& Line() { return _Line; }

private:
	TraceLine(const v8::Handle<v8::Object>& handle, int lineNum);

	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);

	static void jsPrint(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsTimeGetter(v8::Local<v8::String> property, 
												const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsThreadGetter(v8::Local<v8::String> property, 
												const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsMsgGetter(v8::Local<v8::String> property, 
												const v8::PropertyCallbackInfo<v8::Value>& info);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	LineInfo& _Line;
};

class TraceRange : public BaseObject<TraceRange>
{
public:
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

	void GetLines(CBitSet& set);

private:
	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);

	TraceRange(const v8::Handle<v8::Object>& handle, DWORD start, DWORD end);
	static bool ValueToLineIndex(v8::Local<v8::Value>& v, DWORD& idx);

private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	// for small sets it is better to use array
	// for bigger sets - bitset. Ideally it would be nice to have compressed
	// bitset where big ranges are collapsed
	DWORD _Start;
	CBitSet _Lines;
};

class Trace : public BaseObject<Trace>
{
public:
	Trace(const v8::Handle<v8::Object>& handle)
	{
		Wrap(handle);
	}
	static void Init(v8::Isolate* iso);
	static v8::Local<v8::FunctionTemplate> & GetTemplate(v8::Isolate* iso)
	{
		return v8::Local<v8::FunctionTemplate>::New(iso, _Template);
	}

private:
	static void jsNew(const v8::FunctionCallbackInfo<v8::Value> &args);
	// returns a line by index
	static void jsGetLine(const v8::FunctionCallbackInfo<v8::Value> &args);
	// returns set of lines based on range
	static void jsFromRange(const v8::FunctionCallbackInfo<v8::Value> &args);
	static void jsLineCountGetter(v8::Local<v8::String> property, 
												const v8::PropertyCallbackInfo<v8::Value>& info);
	static void jsFormatGetter(v8::Local<v8::String> property, 
												const v8::PropertyCallbackInfo<v8::Value>& info);

	static void jsFormatSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value,
								const v8::PropertyCallbackInfo<void>& info);

	void SetFormat(const char * pszFormat);
private:
	static v8::UniquePersistent<v8::FunctionTemplate> _Template;
	std::string _Format;
};

} // Js
