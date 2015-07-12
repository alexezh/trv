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
#include <locale>
#include "DotExpressions.h"
#include "apphost.h"
#include "color.h"
#include "error.h"
#include "log.h"

using namespace v8;

namespace Js {

Persistent<FunctionTemplate> DotExpressions::_Template;

///////////////////////////////////////////////////////////////////////////////
//
DotExpressions::DotExpressions(const v8::Handle<v8::Object>& handle)
{
	Wrap(handle);
}

void DotExpressions::Init(Isolate* iso)
{
	auto tmpl(FunctionTemplate::New(iso, jsNew));
	tmpl->InstanceTemplate()->SetInternalFieldCount(1);

	auto tmpl_proto = tmpl->PrototypeTemplate();
	tmpl_proto->Set(String::NewFromUtf8(iso, "add"), FunctionTemplate::New(iso, jsAdd));

	_Template.Reset(iso, tmpl);
}

void DotExpressions::jsNew(const FunctionCallbackInfo<Value> &args)
{
	LOG("");
	DotExpressions *dotExpressions = new DotExpressions(args.This());
	GetCurrentHost()->OnDotExpressionsCreated(dotExpressions);

	args.GetReturnValue().Set(args.This());
}

void DotExpressions::jsAdd(const FunctionCallbackInfo<Value>& args)
{
	TryCatchCpp(args, [&args] 
	{
		auto pThis = UnwrapThis<DotExpressions>(args.This());
		return pThis->AddWorker(args);
	});
}

v8::Local<v8::Value> DotExpressions::AddWorker(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	LOG("@%p", this);
	if (args.Length() != 2)
	{
		ThrowSyntaxError("expected add(name, function)\r\n");
	}

	if (!args[0]->IsString())
	{
		std::stringstream ss;

		ss << "first parameter has to be string\r\n";

		auto s = args[0]->ToString();
		String::Utf8Value str(s);
		ss << (*str);

		ThrowSyntaxError(ss.str().c_str());
	}

	if (!args[1]->IsFunction())
	{
		std::stringstream ss;

		ss << "second parameter has to be a function\r\n";
		ss << *String::Utf8Value(args[1]->ToString());

		ThrowSyntaxError(ss.str().c_str());
	}

	auto token = args[0]->ToString();
	String::Utf8Value strToken(token);

	_Expressions.emplace(std::string(*strToken, strToken.length()), UniquePersistent<Function>(Isolate::GetCurrent(), args[1].As<Function>()));

	return v8::Local<v8::Value>();
}

std::vector<std::string> Tokenize(const std::string& line) 
{
	std::vector<std::string> res;
	std::locale loc;
	size_t idxToken = std::string::npos;

	for (size_t i = 0; i < line.length(); i++)
	{
		char c = line[i];
		if (std::isspace(c, loc))
		{
			if (idxToken == std::string::npos)
			{
				continue;
			}
			else
			{
				res.push_back(std::string(&line[idxToken], &line[i]));
				idxToken = std::string::npos;
			}
		}
		else
		{
			if (idxToken == std::string::npos)
				idxToken = i;
		}
	}

	if (idxToken != std::string::npos)
	{
		res.push_back(std::string(&line[idxToken], &line[line.length()]));
	}

	return res;
}

void DotExpressions::Execute(Isolate* iso, const std::string & line)
{
	LOG("@%p line=%s", this, line.c_str());
	auto tokens = Tokenize(line);
	if (tokens.size() == 0)
		return;

	assert(tokens[0][0] == '.');
	auto key = tokens[0].substr(1);
	auto it = _Expressions.find(key);
	if (it == _Expressions.end())
	{
		LOG("@%p cannot find expression %s", this, key.c_str());
	}

	auto func(v8::Local<v8::Function>::New(v8::Isolate::GetCurrent(), it->second));
	std::vector<v8::Local<Value>> args;

	for (size_t i = 1; i < tokens.size(); i++)
	{
		args.push_back(String::NewFromUtf8(iso, tokens[i].c_str()));
	}

	func->Call(v8::Isolate::GetCurrent()->GetCurrentContext()->Global(), args.size(), args.data());
}

} // Js

