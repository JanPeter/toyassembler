#include <node.h>
#include <v8.h>
#include <iostream>
#include <string>
#include "toyasm.h"

using v8::Local;
using v8::Value;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::HandleScope;
using v8::Handle;
using v8::Object;

char* get(Local<Value> value, char* fallback = "")
{
  if(value->IsString()) {
    v8::String::Utf8Value str(value);
    return *str;
  }
  return fallback;
}

void Create(const FunctionCallbackInfo<Value>& args) {
  try
  {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (args.Length() < 1) {
      isolate->ThrowException(v8::Exception::TypeError(
          v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    char* val = get(args[0]);

    Toyasm toasm;
    toasm.create(val, false);

    args.GetReturnValue().Set(1);
  }
  catch(...)
  { }
}

void Init(Handle<Object> exports) {
  NODE_SET_METHOD(exports, "create", Create);
}

NODE_MODULE(toyasm, Init)
