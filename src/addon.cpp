
#include "renderer_wrapper.h"
#include "input_wrapper.h"
#include "audio_wrapper.h"
#include "debugger.h"


Napi::Object Init(Napi::Env env, Napi::Object exports)
{

    DebuggerWrapperInit(env, exports);
    InputWrapper::Init(env, exports);
    AudioWrapper::Init(env, exports);
    // The new wrappers will be self contained can go here instead of directly littering Renderer_wrapper, so they'll be use like renderer.sprite.AnimatedSprite(atlas, {options})

    return  RendererWrapper::Init(env, exports);
}

// This macro registers your module with Node
NODE_API_MODULE(renderer, Init)