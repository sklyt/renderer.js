
#include "renderer_wrapper.h"
#include "input_wrapper.h"
#include "debugger.h"


Napi::Object Init(Napi::Env env, Napi::Object exports)
{

    DebuggerWrapperInit(env, exports);
    InputWrapper::Init(env, exports);

    return  RendererWrapper::Init(env, exports);
}

// This macro registers your module with Node
NODE_API_MODULE(renderer, Init)