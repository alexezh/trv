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
#include "output.h"

namespace Js {

v8::Persistent<v8::FunctionTemplate> Output::_Template;

void Output::Init(v8::Isolate* iso, v8::Handle<v8::Object> & target)
{
	// create an instance of view object and make a property
	// auto view = _Template->InstanceTemplate()->NewInstance();
	// target->SetAccessor(v8::String::New("output"), v8::FunctionTemplate::New(jsRemoveSelection), v8::External::New());
}

void Output::jsNew(const v8::FunctionCallbackInfo<v8::Value> &args)
{
	Output *view = new Output(args.This());

	args.GetReturnValue().Set(args.This());
}

void Output::jsOutput(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	auto pThis = UnwrapThis<Output>(args.This());
	assert(args.Length() == 1);

	args.GetReturnValue().SetUndefined();
}

} // Js

