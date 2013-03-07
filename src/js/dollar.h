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

namespace Js {

class Dollar
{
public:
	// static v8::Persistent<v8::FunctionTemplate> GetTemplate(v8::Handle<v8::Object> & target)
	//		if(!_Template.IsEmpty())
	//	{
	//		return _Template;
	//	}
	static void Init();
	static void InitInstance(v8::Handle<v8::Object> & target);
	static bool ImportFile(const char * pszFile);

private:
	static v8::Handle<v8::Value> jsNew(const v8::Arguments& args);
	static v8::Handle<v8::Value> jsImport(const v8::Arguments& args);
	static v8::Handle<v8::Value> jsQuery(const v8::Arguments& args);
	static v8::Handle<v8::Value> jsPrint(const v8::Arguments& args);

	static v8::Handle<v8::Value> jsGetter(v8::Local<v8::String> property, 
												const v8::AccessorInfo& info);

	static v8::Handle<v8::Value> ImportWorker(const char * pszName, bool& success);
	static bool OpenScriptStream(const char * pszName, std::fstream& stm);

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
	v8::Persistent<v8::Object> _Dollar;
	v8::Persistent<v8::Object> _History;
	v8::Persistent<v8::Object> _View;
	v8::Persistent<v8::Object> _Trace;
};

} // Js
