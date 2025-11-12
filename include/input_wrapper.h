#pragma once
#include "input_manager.h"
#include <napi.h>
#include <vector>


class InputWrapper : public Napi::ObjectWrap<InputWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    InputWrapper(const Napi::CallbackInfo& info);
    ~InputWrapper();
 static Napi::FunctionReference constructor;
private:
   
    

    Napi::Value IsKeyDown(const Napi::CallbackInfo& info);
    Napi::Value IsKeyPressed(const Napi::CallbackInfo& info);
    Napi::Value IsKeyReleased(const Napi::CallbackInfo& info);
    
    Napi::Value IsMouseButtonDown(const Napi::CallbackInfo& info);
    Napi::Value IsMouseButtonPressed(const Napi::CallbackInfo& info);
    Napi::Value IsMouseButtonReleased(const Napi::CallbackInfo& info);
    
    Napi::Value GetMousePosition(const Napi::CallbackInfo& info);
    Napi::Value GetMouseDelta(const Napi::CallbackInfo& info);
    Napi::Value GetMouseWheelDelta(const Napi::CallbackInfo& info);
    

    Napi::Value OnKeyDown(const Napi::CallbackInfo& info);
    Napi::Value OnKeyUp(const Napi::CallbackInfo& info);
    Napi::Value OnMouseDown(const Napi::CallbackInfo& info);
    Napi::Value OnMouseUp(const Napi::CallbackInfo& info);
    Napi::Value OnMouseMove(const Napi::CallbackInfo& info);
    Napi::Value OnMouseWheel(const Napi::CallbackInfo& info);
    

    Napi::Value RemoveCallback(const Napi::CallbackInfo& info);
    Napi::Value RemoveAllCallbacks(const Napi::CallbackInfo& info);
    

    Napi::Object CreateInputEvent(Napi::Env env, const InputEvent& event);
    Napi::Value ManualUpdateInputs(const Napi::CallbackInfo& info);
    

    // store JavaScript callback references to prevent GC
    std::vector<std::pair<InputCallbackId, std::shared_ptr<Napi::FunctionReference>>> callbacks_;
};