#pragma once

class Dollar
{
public:
	// static v8::Persistent<v8::FunctionTemplate> GetTemplate(v8::Handle<v8::Object> & target)
	//		if(!_Template.IsEmpty())
	//	{
	//		return _Template;
	//	}
	static void Init(v8::Handle<v8::Object> & target)
	{
		_Template = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
		_Template->InstanceTemplate()->SetInternalFieldCount(1);
		_Template->InstanceTemplate()->Set(v8::String::New("filter"), v8::FunctionTemplate::New(jsFilter));

		target->Set(v8::String::NewSymbol("$"), _Template->GetFunction());
	}

	static v8::Handle<v8::Value> jsFilter(const v8::Arguments& args)
	{
		// create new filter
	}

private:
	static v8::Persistent<v8::FunctionTemplate> _Template;
};

