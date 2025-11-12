#include <napi.h>
#include "debugger.h"

Napi::Value SetDebugEnabledWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::TypeError::New(env, "Boolean expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    bool enabled = info[0].As<Napi::Boolean>().Value();
    Debugger::Instance().SetEnabled(enabled);
    return env.Undefined();
}

Napi::Value IsDebugEnabledWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool enabled = Debugger::Instance().IsEnabled();
    return Napi::Boolean::New(env, enabled);
}

Napi::Object DebuggerWrapperInit(Napi::Env env, Napi::Object exports) {
    exports.Set("SetDebugEnabled", Napi::Function::New(env, SetDebugEnabledWrapped));
    exports.Set("IsDebugEnabled", Napi::Function::New(env, IsDebugEnabledWrapped));
    return exports;
}
