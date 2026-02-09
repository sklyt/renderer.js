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

        if (renderer_->onResize_)
        {
            int newWidth = ::GetScreenWidth();
            int newHeight = ::GetScreenHeight();
            renderer_->onResize_(newWidth, newHeight);
        }

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

    // Window-related methods
    Napi::Value CloseWindow(const Napi::CallbackInfo &info);
    Napi::Value IsWindowReady(const Napi::CallbackInfo &info);
    Napi::Value IsWindowFullscreen(const Napi::CallbackInfo &info);
    Napi::Value IsWindowHidden(const Napi::CallbackInfo &info);
    Napi::Value IsWindowMinimized(const Napi::CallbackInfo &info);
    Napi::Value IsWindowMaximized(const Napi::CallbackInfo &info);
    Napi::Value IsWindowFocused(const Napi::CallbackInfo &info);
    Napi::Value IsWindowResized(const Napi::CallbackInfo &info);
    Napi::Value IsWindowState(const Napi::CallbackInfo &info);
    Napi::Value ClearWindowState(const Napi::CallbackInfo &info);
    Napi::Value ToggleFullscreen(const Napi::CallbackInfo &info);
    Napi::Value ToggleBorderlessWindowed(const Napi::CallbackInfo &info);
    Napi::Value MaximizeWindow(const Napi::CallbackInfo &info);
    Napi::Value MinimizeWindow(const Napi::CallbackInfo &info);
    Napi::Value RestoreWindow(const Napi::CallbackInfo &info);
    Napi::Value SetWindowIcon(const Napi::CallbackInfo &info);
    Napi::Value SetWindowIcons(const Napi::CallbackInfo &info);
    Napi::Value SetWindowTitle(const Napi::CallbackInfo &info);
    Napi::Value SetWindowPosition(const Napi::CallbackInfo &info);
    Napi::Value SetWindowMonitor(const Napi::CallbackInfo &info);
    Napi::Value SetWindowMinSize(const Napi::CallbackInfo &info);
    Napi::Value SetWindowMaxSize(const Napi::CallbackInfo &info);
    Napi::Value SetWindowSize(const Napi::CallbackInfo &info);
    Napi::Value SetWindowOpacity(const Napi::CallbackInfo &info);
    Napi::Value SetWindowFocused(const Napi::CallbackInfo &info);
    Napi::Value GetWindowHandle(const Napi::CallbackInfo &info);
    Napi::Value GetScreenWidth(const Napi::CallbackInfo &info);
    Napi::Value GetScreenHeight(const Napi::CallbackInfo &info);
    Napi::Value GetRenderWidth(const Napi::CallbackInfo &info);
    Napi::Value GetRenderHeight(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorCount(const Napi::CallbackInfo &info);
    Napi::Value GetCurrentMonitor(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorPosition(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorWidth(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorHeight(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorPhysicalWidth(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorPhysicalHeight(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorRefreshRate(const Napi::CallbackInfo &info);
    Napi::Value GetWindowPosition(const Napi::CallbackInfo &info);
    Napi::Value GetWindowScaleDPI(const Napi::CallbackInfo &info);
    Napi::Value GetMonitorName(const Napi::CallbackInfo &info);
    Napi::Value SetClipboardText(const Napi::CallbackInfo &info);
    Napi::Value GetClipboardText(const Napi::CallbackInfo &info);
    Napi::Value GetClipboardImage(const Napi::CallbackInfo &info);
    Napi::Value EnableEventWaiting(const Napi::CallbackInfo &info);
    Napi::Value DisableEventWaiting(const Napi::CallbackInfo &info);

    // Cursor-related methods
    Napi::Value ShowCursor(const Napi::CallbackInfo &info);
    Napi::Value HideCursor(const Napi::CallbackInfo &info);
    Napi::Value IsCursorHidden(const Napi::CallbackInfo &info);
    Napi::Value EnableCursor(const Napi::CallbackInfo &info);
    Napi::Value DisableCursor(const Napi::CallbackInfo &info);
    Napi::Value IsCursorOnScreen(const Napi::CallbackInfo &info);

    // extend for partial updates

    Napi::Value ProcessPendingRegions(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Expected bufferRefId").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        size_t bufRefId = info[0].As<Napi::Number>().Uint32Value();
        renderer_->ProcessPendingRegions(bufRefId);

        return env.Undefined();
    }

    // sprite 

      Napi::Value LoadAtlas(const Napi::CallbackInfo &info);
    Napi::Value GetAtlasPixel(const Napi::CallbackInfo &info);
    Napi::Value IsAtlasOpaque(const Napi::CallbackInfo &info);
    Napi::Value GetAtlasData(const Napi::CallbackInfo &info);
    Napi::Value GetAtlasDataAndFree(const Napi::CallbackInfo &info);
    Napi::Value FreeAtlas(const Napi::CallbackInfo &info);

    // Napi::Value PartialTextureUpdate(const Napi::CallbackInfo &info)
    // {
    //     Napi::Env env = info.Env();

    //     if (info.Length() < 5)
    //     {
    //         Napi::TypeError::New(env, "Expected (bufRefId, x, y, w, h)").ThrowAsJavaScriptException();
    //         return env.Undefined();
    //     }

    //     size_t bufRefId = info[0].As<Napi::Number>().Uint32Value();
    //     uint32_t x = info[1].As<Napi::Number>().Uint32Value();
    //     uint32_t y = info[2].As<Napi::Number>().Uint32Value();
    //     uint32_t w = info[3].As<Napi::Number>().Uint32Value();
    //     uint32_t h = info[4].As<Napi::Number>().Uint32Value();

    //     renderer_->PartialTextureUpdate(bufRefId, x, y, w, h);

    //     return env.Undefined();
    // }

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
