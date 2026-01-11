#pragma once
#include <napi.h>
#include "renderer.h"
#include "vector"
#include "input_wrapper.h"
#include "audio_wrapper.h"
#include <iostream>

class RendererWrapper : public Napi::ObjectWrap<RendererWrapper>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    RendererWrapper(const Napi::CallbackInfo &info);

    // Core lifecycle
    Napi::Value Initialize(const Napi::CallbackInfo &info);
    Napi::Value Shutdown(const Napi::CallbackInfo &info);
    Napi::Value BeginFrame(const Napi::CallbackInfo &info);
    Napi::Value EndFrame(const Napi::CallbackInfo &info);
    Napi::Value Step(const Napi::CallbackInfo &info);

    // Drawing methods
    Napi::Value Clear(const Napi::CallbackInfo &info);
    Napi::Value DrawRectangle(const Napi::CallbackInfo &info);
    Napi::Value DrawCircle(const Napi::CallbackInfo &info);
    Napi::Value DrawLine(const Napi::CallbackInfo &info);
    Napi::Value DrawText(const Napi::CallbackInfo &info);

    // Texture methods
    Napi::Value LoadTexture(const Napi::CallbackInfo &info);
    Napi::Value UnloadTexture(const Napi::CallbackInfo &info);
    Napi::Value DrawTexture(const Napi::CallbackInfo &info);
    Napi::Value DrawTexturePro(const Napi::CallbackInfo &info);

    // Render target methods
    Napi::Value CreateRenderTexture(const Napi::CallbackInfo &info);
    Napi::Value DestroyRenderTexture(const Napi::CallbackInfo &info);
    Napi::Value SetRenderTarget(const Napi::CallbackInfo &info);
    Napi::Value OnRender(const Napi::CallbackInfo &info);

    // Accessors
    Napi::Value GetWidth(const Napi::CallbackInfo &info);
    Napi::Value GetHeight(const Napi::CallbackInfo &info);
    Napi::Value GetInput(const Napi::CallbackInfo &info);
    Napi::Value GetAudio(const Napi::CallbackInfo &info);
    void SetTargetFPS(const Napi::CallbackInfo &info, const Napi::Value &value);
    Napi::Value IsWindowClosed(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        return Napi::Boolean::New(env, renderer_->IsWindowClosed());
    };

    void onResize(const Napi::CallbackInfo &info, const Napi::Value &value)
    {
        Napi::Env env = info.Env();

        if (!value.IsFunction())
        {
            Napi::Error::New(env, "Expected Function Type for OnResize callback").ThrowAsJavaScriptException();
            return;
        }

        Napi::Function callback = value.As<Napi::Function>();

        auto persistentCallback = std::make_shared<Napi::FunctionReference>(
            Napi::Persistent(callback));

        // std::cout << "set func on resize" << std::endl;
        renderer_->onResize_ = [persistentCallback](int width, int height)
        {
            Napi::Env env = persistentCallback->Env();
            Napi::HandleScope scope(env); // ensure a scope for V8 handles

            try
            {
                Napi::Object eventObj = Napi::Object::New(env);
                eventObj.Set("width", Napi::Number::New(env, width));
                eventObj.Set("height", Napi::Number::New(env, height));
                persistentCallback->Call({eventObj});
            }
            catch (const Napi::Error &e)
            {
                std::cerr << "Error in JS resize callback: " << e.Message() << std::endl;
            }
        };
    }

    Napi::Value SetWindowState(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (renderer_->IsWindowClosed())
        {
            Napi::Error::New(env, "Window not initialized").ThrowAsJavaScriptException();
            return env.Null();
        }

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Expected flags (number) argument").ThrowAsJavaScriptException();
            return env.Null();
        }

        unsigned int flags = info[0].As<Napi::Number>().Uint32Value();
        ::SetWindowState(flags);

        return env.Undefined();
    }
    Napi::Value GetFPS(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        return Napi::Number::New(env, renderer_->GetCurrentFPS());
    }
    void HandleRenderCallback();

    // buffer

    Napi::Value CreateSharedBuffer(const Napi::CallbackInfo &info);
    Napi::Value IsBufferDirty(const Napi::CallbackInfo &info);
    Napi::Value MarkBufferDirty(const Napi::CallbackInfo &info);
    Napi::Value GetBufferData(const Napi::CallbackInfo &info);
    Napi::Value UpdateBufferData(const Napi::CallbackInfo &info);
    Napi::Value MarkBufferRegionDirty(const Napi::CallbackInfo &info);
    Napi::Value SetBufferDimensions(const Napi::CallbackInfo &info);
    Napi::Value GetBufferStats(const Napi::CallbackInfo &info);

    Napi::Value LoadTextureFromBuffer(const Napi::CallbackInfo &info);
    Napi::Value UpdateTextureFromBuffer(const Napi::CallbackInfo &info);
    Napi::Value DrawTextureSized(const Napi::CallbackInfo &info);

    Napi::Value LoadImage(const Napi::CallbackInfo &info);
    Napi::Value UnloadImage(const Napi::CallbackInfo &info);
    Napi::Value InitSharedBuffers(const Napi::CallbackInfo &info);

    Napi::Value SetClearColor(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        Color4 color(0.1f, 0.1f, 0.1f, 1.0f);
        if (info.Length() > 0)
        {
            color = ParseColor(info[0], color);
        }

        if (renderer_)
        {
            renderer_->clearColor = color;
        }

         return env.Undefined();
    }

private:
    std::unique_ptr<Renderer> renderer_;
    std::vector<Napi::FunctionReference> renderCallbacks_;
    Napi::ObjectReference inputWrapper_;
    Napi::ObjectReference audioWrapper_;

    // Helper methods
    static Color4 ParseColor(const Napi::Value &colorValue, const Color4 &defaultColor = Color4(0.1f, 0.1f, 0.1f, 1.0f));
    static Vec2 ParseVec2(const Napi::Value &vecValue, const Vec2 &defaultVec = Vec2(0, 0));

    static Napi::FunctionReference constructor;
};

namespace WindowFlags
{
    const int FULLSCREEN = FLAG_FULLSCREEN_MODE;
    const int RESIZABLE = FLAG_WINDOW_RESIZABLE;
    const int UNDECORATED = FLAG_WINDOW_UNDECORATED;
    const int ALWAYS_RUN = FLAG_WINDOW_ALWAYS_RUN;
    const int VSYNC_HINT = FLAG_VSYNC_HINT;
    const int MSAA_4X_HINT = FLAG_MSAA_4X_HINT;
}
