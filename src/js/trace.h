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

class TraceLine : public ObjectWrap
{
public:
	static void Init();
	static v8::Persistent<v8::FunctionTemplate> & GetTemplate() { return _Template; }

	const LineInfo& Line() { return _Line; }

private:
	TraceLine(const v8::Handle<v8::Object>& handle, int lineNum);

	static v8::Handle<v8::Value> jsNew(const v8::Arguments &args);

	static v8::Handle<v8::Value> jsPrint(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsTimeGetter(v8::Local<v8::String> property, 
												const v8::AccessorInfo& info);
	static v8::Handle<v8::Value> jsThreadGetter(v8::Local<v8::String> property, 
												const v8::AccessorInfo& info);
	static v8::Handle<v8::Value> jsMsgGetter(v8::Local<v8::String> property, 
												const v8::AccessorInfo& info);

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
	LineInfo& _Line;
};

class TraceRange : public ObjectWrap
{
public:
	static void Init();
	static v8::Persistent<v8::FunctionTemplate> & GetTemplate() { return _Template; }

	void GetLines(CBitSet& set);

private:
	static v8::Handle<v8::Value> jsNew(const v8::Arguments &args);

	TraceRange(const v8::Handle<v8::Object>& handle, DWORD start, DWORD end);
	static bool ValueToLineIndex(v8::Local<v8::Value>& v, DWORD& idx);

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
	// for small sets it is better to use array
	// for bigger sets - bitset. Ideally it would be nice to have compressed
	// bitset where big ranges are collapsed
	DWORD _Start;
	CBitSet _Lines;
};

class Trace
{
public:

	static void Init();
	static v8::Persistent<v8::FunctionTemplate> & GetTemplate() { return _Template; }

private:
	static v8::Handle<v8::Value> jsNew(const v8::Arguments &args);
	// returns a line by index
	static v8::Handle<v8::Value> jsGetLine(const v8::Arguments &args);
	// returns set of lines based on range
	static v8::Handle<v8::Value> jsFromRange(const v8::Arguments &args);
	static v8::Handle<v8::Value> jsLineCountGetter(v8::Local<v8::String> property, 
												const v8::AccessorInfo& info);
	static v8::Handle<v8::Value> jsFormatGetter(v8::Local<v8::String> property, 
												const v8::AccessorInfo& info);

	static void jsFormatSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value,
								const v8::AccessorInfo& info);

	void SetFormat(const char * pszFormat);
private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
	std::string _Format;
};

} // Js
