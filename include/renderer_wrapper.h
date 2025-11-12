#pragma once
#include <napi.h>
#include "renderer.h"
#include "vector"
#include "input_wrapper.h"

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

    // Render target methods
    Napi::Value CreateRenderTexture(const Napi::CallbackInfo &info);
    Napi::Value DestroyRenderTexture(const Napi::CallbackInfo &info);
    Napi::Value SetRenderTarget(const Napi::CallbackInfo &info);
    Napi::Value OnRender(const Napi::CallbackInfo &info);

    // Accessors
    Napi::Value GetWidth(const Napi::CallbackInfo &info);
    Napi::Value GetHeight(const Napi::CallbackInfo &info);
    Napi::Value GetInput(const Napi::CallbackInfo &info);
    void SetTargetFPS(const Napi::CallbackInfo &info, const Napi::Value &value);
    Napi::Value IsWindowClosed(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();
        return Napi::Boolean::New(env, renderer_->IsWindowClosed());
    };

    Napi::Value GetFPS(const Napi::CallbackInfo &info){
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

    Napi::Value LoadTextureFromBuffer(const Napi::CallbackInfo &info);
    Napi::Value UpdateTextureFromBuffer(const Napi::CallbackInfo &info);
    Napi::Value DrawTextureSized(const Napi::CallbackInfo &info);

private:
    std::unique_ptr<Renderer> renderer_;
    std::vector<Napi::FunctionReference> renderCallbacks_;
    Napi::ObjectReference inputWrapper_;
    // Helper methods
    static Color4 ParseColor(const Napi::Value &colorValue, const Color4 &defaultColor = Color4(0.1f, 0.1f, 0.1f, 1.0f));
    static Vec2 ParseVec2(const Napi::Value &vecValue, const Vec2 &defaultVec = Vec2(0, 0));

    static Napi::FunctionReference constructor;
};