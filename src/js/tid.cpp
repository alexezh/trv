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
#include "tid.h"
#include "funcwrap.h"

using namespace v8;

namespace Js {

static char s_InvalidArgs[] = "Invalid number of arguments. Format matchtid(int)";
Persistent<FunctionTemplate> MatchTid::_Template;

///////////////////////////////////////////////////////////////////////////////
//
void MatchTid::Init(v8::Handle<v8::ObjectTemplate> & target)
{
	_Template = Persistent<FunctionTemplate>(FunctionTemplate::New(jsNew));
	_Template->InstanceTemplate()->SetInternalFieldCount(1);
	_Template->SetClassName(String::New("matchtid"));

	target->Set(String::New("matchtid"), v8::FunctionTemplate::New(jsMake));
}

///////////////////////////////////////////////////////////////////////////////
MatchTid::MatchTid(int tid)
	: _Tid(tid)
{
}

///////////////////////////////////////////////////////////////////////////////
Handle<Value> MatchTid::jsMake(const Arguments &args)
{
	if(args.Length() != 3)
	{
		return ThrowException(Exception::SyntaxError(String::New(s_InvalidArgs)));
	}

	return _Template->GetFunction()->NewInstance(1, &args[0]);
}

///////////////////////////////////////////////////////////////////////////////
Handle<Value> MatchTid::jsNew(const Arguments &args)
{
	if(args.Length() != 1 || !args[0]->IsInt32())
	{
		return ThrowException(Exception::TypeError(String::New(s_InvalidArgs)));
	}

	MatchTid *tid = new MatchTid(args[0]->Int32Value());
	args.This()->SetInternalField(0, External::New(tid));

	return args.This();
}

} // 
