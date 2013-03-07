// Copyright Joyent, Inc. and other Node contributors.
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

#include <assert.h>

namespace Js {

class ObjectWrap 
{
public:
	ObjectWrap ( ) 
	{
	}


	virtual ~ObjectWrap ( ) 
	{
		if (!_Handle.IsEmpty()) 
		{
			assert(_Handle.IsNearDeath());
			_Handle.ClearWeak();
			_Handle->SetInternalField(0, v8::Undefined());
			_Handle.Dispose();
			_Handle.Clear();
		}
	}


	template <class T>
	static inline T* Unwrap (v8::Handle<v8::Object> handle) {
		assert(!handle.IsEmpty());
		assert(handle->InternalFieldCount() > 0);
		return static_cast<T*>(handle->GetPointerFromInternalField(0));
	}


	v8::Persistent<v8::Object> _Handle; // ro

protected:
	inline void Wrap (const v8::Handle<v8::Object>& handle) 
	{
		assert(_Handle.IsEmpty());
		assert(handle->InternalFieldCount() > 0);
		_Handle = v8::Persistent<v8::Object>::New(handle);
		_Handle->SetInternalField(0, v8::External::New(this));
		MakeWeak();
	}


	inline void MakeWeak (void) 
	{
		_Handle.MakeWeak(this, WeakCallback);
		_Handle.MarkIndependent();
	}

private:
	static void WeakCallback (v8::Persistent<v8::Value> value, void *data)
	{
		ObjectWrap *obj = static_cast<ObjectWrap*>(data);
		assert(value == obj->_Handle);
		assert(value.IsNearDeath());
		delete obj;
	}
};

} // Js
