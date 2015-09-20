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

// base class for native objects owned by JS
// stores weak reference to Js
template <class T>
class BaseObject 
{
public:
	BaseObject() 
	{
	}


	virtual ~BaseObject() 
	{
	}

	static inline T* Unwrap(v8::Handle<v8::Object> handle) 
	{
		assert(!handle.IsEmpty());
		assert(handle->InternalFieldCount() > 0);
		void* ptr = handle->GetAlignedPointerFromInternalField(0);
		BaseObject* wrap = static_cast<BaseObject*>(ptr);
		return static_cast<T*>(wrap);
	}


	inline v8::Local<v8::Object> Handle() 
	{
		return Handle(v8::Isolate::GetCurrent());
	}


	inline v8::Local<v8::Object> Handle(v8::Isolate* isolate) 
	{
		return v8::Local<v8::Object>::New(isolate, m_Handle);
	}


	inline v8::Persistent<v8::Object>& PersistedHandle()
	{
		return m_Handle;
	}


protected:
	inline void Wrap(v8::Handle<v8::Object> handle) 
	{
		assert(m_Handle.IsEmpty());
		assert(handle->InternalFieldCount() > 0);
		handle->SetAlignedPointerInInternalField(0, static_cast<T*>(this));
		m_Handle.Reset(v8::Isolate::GetCurrent(), handle);
		MakeWeak();
	}


	inline void MakeWeak(void) 
	{
		m_Handle.SetWeak(this, WeakCallback);
		m_Handle.MarkIndependent();
	}

private:
	static void WeakCallback(
		const v8::WeakCallbackData<v8::Object, BaseObject>& data) 
	{
		v8::Isolate* isolate = data.GetIsolate();
		v8::HandleScope scope(isolate);
		BaseObject* wrap = data.GetParameter();
		wrap->m_Handle.Reset();
		delete wrap;
	}

	v8::UniquePersistent<v8::Object> m_Handle;
};

template <class T>
T * UnwrapThis(v8::Local<v8::Object> valThis)
{
	return T::Unwrap(valThis);
}

inline v8::MaybeLocal<v8::Value> GetObjectField(v8::Local<v8::Object>& obj, const char* pszField)
{
	return obj->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), pszField));
}

} // Js
