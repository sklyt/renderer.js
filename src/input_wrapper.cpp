#include "input_wrapper.h"
#include "renderer.h"
#include "debugger.h"
#include <iostream>

Napi::FunctionReference InputWrapper::constructor;

Napi::Object InputWrapper::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Input", {

                                                        InstanceMethod("isKeyDown", &InputWrapper::IsKeyDown),
                                                        InstanceMethod("isKeyPressed", &InputWrapper::IsKeyPressed),
                                                        InstanceMethod("isKeyReleased", &InputWrapper::IsKeyReleased),

                                                        InstanceMethod("isMouseButtonDown", &InputWrapper::IsMouseButtonDown),
                                                        InstanceMethod("isMouseButtonPressed", &InputWrapper::IsMouseButtonPressed),
                                                        InstanceMethod("isMouseButtonReleased", &InputWrapper::IsMouseButtonReleased),

                                                        InstanceMethod("getMousePosition", &InputWrapper::GetMousePosition),
                                                        InstanceMethod("getMouseDelta", &InputWrapper::GetMouseDelta),
                                                        InstanceMethod("getMouseWheelDelta", &InputWrapper::GetMouseWheelDelta),

                                                        InstanceMethod("onKeyDown", &InputWrapper::OnKeyDown),
                                                        InstanceMethod("onKeyUp", &InputWrapper::OnKeyUp),
                                                        InstanceMethod("onMouseDown", &InputWrapper::OnMouseDown),
                                                        InstanceMethod("onMouseUp", &InputWrapper::OnMouseUp),
                                                        InstanceMethod("onMouseMove", &InputWrapper::OnMouseMove),
                                                        InstanceMethod("onMouseWheel", &InputWrapper::OnMouseWheel),

                                                        InstanceMethod("removeCallback", &InputWrapper::RemoveCallback),
                                                        InstanceMethod("removeAllCallbacks", &InputWrapper::RemoveAllCallbacks),
                                                        InstanceMethod("GetInput", &InputWrapper::ManualUpdateInputs),
                                                    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("Input", func);
    return exports;
}

InputWrapper::InputWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<InputWrapper>(info)
{
}

InputWrapper::~InputWrapper()
{
    for (auto &pair : callbacks_)
    {
        InputManager::Instance().UnregisterCallback(pair.first);
    }
    callbacks_.clear();
}

Napi::Value InputWrapper::IsKeyDown(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "isKeyDown requires a key name").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string keyName = info[0].As<Napi::String>().Utf8Value();
    bool isDown = InputManager::Instance().IsKeyDown(keyName);
    // std::cout << "isKeydown " << keyName << " " <<  isDown << std::endl;

    return Napi::Boolean::New(env, isDown);
}
Napi::Value InputWrapper::IsKeyPressed(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "isKeyPressed requires a key name").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string keyName = info[0].As<Napi::String>().Utf8Value();
    bool isDown = InputManager::Instance().IsKeyPressed(keyName);
    return Napi::Boolean::New(env, isDown);
}

Napi::Value InputWrapper::IsKeyReleased(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "isKeyPressed requires a key name").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string keyName = info[0].As<Napi::String>().Utf8Value();
    bool isDown = InputManager::Instance().IsKeyReleased(keyName);
    return Napi::Boolean::New(env, isDown);
}

Napi::Value InputWrapper::IsMouseButtonDown(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "isMouseButton requires a key name(of type number 0 for left)").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    int mousebtn = info[0].As<Napi::Number>();
    bool isMouseDown = InputManager::Instance().IsMouseButtonDown(mousebtn);
    // Debugger::Instance().LogInfo("Button down: " + std::to_string(mousebtn) + "res: " + std::to_string(isMouseDown));

    return Napi::Boolean::New(env, isMouseDown);
}

Napi::Value InputWrapper::IsMouseButtonPressed(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "isMouseButton requires a key name(of type number 1 for left)").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    int mousebtn = info[0].As<Napi::Number>();
    bool isMouseDown = InputManager::Instance().IsMouseButtonPressed(mousebtn);
    // Debugger::Instance().LogInfo("Button Pressed: " + std::to_string(mousebtn) + "res: " + std::to_string(isMouseDown));
    return Napi::Boolean::New(env, isMouseDown);
}

Napi::Value InputWrapper::IsMouseButtonReleased(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "isMouseButton requires a key name(of type number 1 for left)").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    int mousebtn = info[0].As<Napi::Number>();
    bool isMouseDown = InputManager::Instance().IsMouseButtonReleased(mousebtn);
    return Napi::Boolean::New(env, isMouseDown);
}

Napi::Value InputWrapper::OnKeyDown(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        Napi::TypeError::New(env, "onKeyDown requires key name and callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    std::string keyName = info[0].As<Napi::String>().Utf8Value();
    Napi::Function jsCallback = info[1].As<Napi::Function>();

    auto persistentCallback = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(jsCallback)
    );

    InputCallbackId id = InputManager::Instance().RegisterKeyCallback(
        keyName,
        InputEventType::KeyDown,
        [this, env, persistentCallback](const InputEvent &event)
        {
            try
            {
                Napi::Object eventObj = CreateInputEvent(env, event);
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cout << "Error in JavaScript key callback: " << e.Message() << std::endl;
            }
        });

    callbacks_.emplace_back(id, persistentCallback);

    return Napi::Number::New(env, id);
}

Napi::Value InputWrapper::OnKeyUp(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        Napi::TypeError::New(env, "onKeyDown requires key name and callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    std::string keyName = info[0].As<Napi::String>().Utf8Value();
    Napi::Function jsCallback = info[1].As<Napi::Function>();

    auto persistentCallback = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(jsCallback)
    );

    InputCallbackId id = InputManager::Instance().RegisterKeyCallback(
        keyName,
        InputEventType::KeyUp,
        [this, env, persistentCallback](const InputEvent &event)
        {
            try
            {
                Napi::Object eventObj = CreateInputEvent(env, event);
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cout << "Error in JavaScript key callback: " << e.Message() << std::endl;
            }
        });

    callbacks_.emplace_back(id, persistentCallback);

    return Napi::Number::New(env, id);
}

Napi::Value InputWrapper::OnMouseDown(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsFunction())
    {
        Napi::TypeError::New(env, "onMousedown requires key name and callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    int keyName = info[0].As<Napi::Number>();
    Napi::Function jsCallback = info[1].As<Napi::Function>();

   auto persistentCallback = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(jsCallback)
    );


    InputCallbackId id = InputManager::Instance().RegisterMouseCallback(
        keyName,
        InputEventType::MouseDown,
        [this, env, persistentCallback](const InputEvent &event)
        {
            try
            {
                Napi::Object eventObj = CreateInputEvent(env, event);
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cout << "Error in JavaScript key callback: " << e.Message() << std::endl;
            }
        });


    callbacks_.emplace_back(id, persistentCallback);

    return Napi::Number::New(env, id);
}

Napi::Value InputWrapper::OnMouseUp(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsFunction())
    {
        Napi::TypeError::New(env, "onMouseUp requires key name and callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    int keyName = info[0].As<Napi::Number>();
    Napi::Function jsCallback = info[1].As<Napi::Function>();

   auto persistentCallback = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(jsCallback)
    );



    InputCallbackId id = InputManager::Instance().RegisterMouseCallback(
        keyName,
        InputEventType::MouseUp,
        [this, env, persistentCallback](const InputEvent &event)
        {
            try
            {
                Napi::Object eventObj = CreateInputEvent(env, event);
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cout << "Error in JavaScript key callback: " << e.Message() << std::endl;
            }
        });

    callbacks_.emplace_back(id, persistentCallback);

    return Napi::Number::New(env, id);
}

Napi::Value InputWrapper::OnMouseMove(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        Napi::TypeError::New(env, "onMouseMove requires callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    Napi::Function jsCallback = info[0].As<Napi::Function>();

      auto persistentCallback = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(jsCallback)
    );



    InputCallbackId id = InputManager::Instance().RegisterMouseMoveCallback(
        [this, env, persistentCallback](const InputEvent &event)
        {
            try
            {
                Napi::Object eventObj = CreateInputEvent(env, event);
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cout << "Error in JavaScript key callback: " << e.Message() << std::endl;
            }
        });


    callbacks_.emplace_back(id, persistentCallback);

    return Napi::Number::New(env, id);
}

Napi::Value InputWrapper::OnMouseWheel(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        Napi::TypeError::New(env, "onMouseMove requires callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    Napi::Function jsCallback = info[0].As<Napi::Function>();

   auto persistentCallback = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(jsCallback)
    );



    InputCallbackId id = InputManager::Instance().RegisterMouseWheelCallback(
        [this, env, persistentCallback](const InputEvent &event)
        {
            try
            {
                Napi::Object eventObj = CreateInputEvent(env, event);
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cout << "Error in JavaScript key callback: " << e.Message() << std::endl;
            }
        });


    callbacks_.emplace_back(id, persistentCallback);

    return Napi::Number::New(env, id);
}

Napi::Value InputWrapper::RemoveCallback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "remove callback requires callback function").ThrowAsJavaScriptException();
        return Napi::Number::New(env, 0);
    }

    InputManager::Instance().UnregisterCallback(info[0].As<Napi::Number>());

    return env.Undefined();
}
Napi::Value InputWrapper::RemoveAllCallbacks(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    InputManager::Instance().UnregisterAllCallbacks();
        return env.Undefined();
}

Napi::Value InputWrapper::GetMousePosition(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    Vec2 pos = InputManager::Instance().GetMousePosition();

    Napi::Object posObj = Napi::Object::New(env);
    posObj.Set("x", Napi::Number::New(env, pos.x));
    posObj.Set("y", Napi::Number::New(env, pos.y));

    return posObj;
}

Napi::Value InputWrapper::GetMouseDelta(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Vec2 delta = InputManager::Instance().GetMouseDelta();
    Napi::Object posObj = Napi::Object::New(env);
    posObj.Set("x", Napi::Number::New(env, delta.x));
    posObj.Set("y", Napi::Number::New(env, delta.y));

    return posObj;
}

Napi::Value InputWrapper::GetMouseWheelDelta(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    float delta = InputManager::Instance().GetMouseWheelDelta();
    if (delta)
    {
        return Napi::Number::New(env, delta);
    }
    return Napi::Number::New(env, 0);
}

Napi::Object InputWrapper::CreateInputEvent(Napi::Env env, const InputEvent &event)
{
    Napi::Object eventObj = Napi::Object::New(env);

    // Event type
    std::string typeStr;
    switch (event.type)
    {
    case InputEventType::KeyDown:
        typeStr = "keydown";
        break;
    case InputEventType::KeyUp:
        typeStr = "keyup";
        break;
    case InputEventType::MouseDown:
        typeStr = "mousedown";
        break;
    case InputEventType::MouseUp:
        typeStr = "mouseup";
        break;
    case InputEventType::MouseMove:
        typeStr = "mousemove";
        break;
    case InputEventType::MouseWheel:
        typeStr = "mousewheel";
        break;
    }
    eventObj.Set("type", Napi::String::New(env, typeStr));

    // Key data
    eventObj.Set("keyCode", Napi::Number::New(env, event.keyCode));
    eventObj.Set("keyName", Napi::String::New(env, event.keyName));

    // Mouse data
    eventObj.Set("mouseButton", Napi::Number::New(env, event.mouseButton));

    Napi::Object posObj = Napi::Object::New(env);
    posObj.Set("x", Napi::Number::New(env, event.mousePosition.x));
    posObj.Set("y", Napi::Number::New(env, event.mousePosition.y));
    eventObj.Set("mousePosition", posObj);

    Napi::Object deltaObj = Napi::Object::New(env);
    deltaObj.Set("x", Napi::Number::New(env, event.mouseDelta.x));
    deltaObj.Set("y", Napi::Number::New(env, event.mouseDelta.y));
    eventObj.Set("mouseDelta", deltaObj);

    eventObj.Set("wheelDelta", Napi::Number::New(env, event.wheelDelta));
    eventObj.Set("timestamp", Napi::Number::New(env, event.timestamp));

    return eventObj;
}

Napi::Value InputWrapper::ManualUpdateInputs(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    InputManager::Instance().Update();
    return env.Undefined();
}