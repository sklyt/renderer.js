
#include "renderer_wrapper.h"
#include <iostream>
#include "console_control.h"

// Forward declare stb_image functions
extern "C" {
    unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    void stbi_image_free(void *retval_from_stbi_load);
}

Napi::FunctionReference RendererWrapper::constructor;

Napi::Object RendererWrapper::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Renderer", {
                                                           // Core lifecycle
                                                           InstanceMethod("initialize", &RendererWrapper::Initialize),
                                                           InstanceMethod("shutdown", &RendererWrapper::Shutdown),
                                                           InstanceMethod("beginFrame", &RendererWrapper::BeginFrame),
                                                           InstanceMethod("endFrame", &RendererWrapper::EndFrame),

                                                           InstanceMethod("clear", &RendererWrapper::Clear),
                                                           InstanceMethod("drawRectangle", &RendererWrapper::DrawRectangle),
                                                           InstanceMethod("drawCircle", &RendererWrapper::DrawCircle),
                                                           InstanceMethod("drawLine", &RendererWrapper::DrawLine),
                                                           InstanceMethod("drawText", &RendererWrapper::DrawText),

                                                           InstanceMethod("loadTexture", &RendererWrapper::LoadTexture),
                                                           InstanceMethod("unloadTexture", &RendererWrapper::UnloadTexture),
                                                           InstanceMethod("drawTexture", &RendererWrapper::DrawTexture),
                                                           InstanceMethod("drawTexturePro", &RendererWrapper::DrawTexturePro),

                                                           InstanceMethod("createRenderTexture", &RendererWrapper::CreateRenderTexture),
                                                           InstanceMethod("destroyRenderTexture", &RendererWrapper::DestroyRenderTexture),
                                                           InstanceMethod("setRenderTarget", &RendererWrapper::SetRenderTarget),

                                                           InstanceMethod("createSharedBuffer", &RendererWrapper::CreateSharedBuffer),
                                                           InstanceMethod("markBufferDirty", &RendererWrapper::MarkBufferDirty),
                                                           InstanceMethod("isBufferDirty", &RendererWrapper::IsBufferDirty),
                                                           InstanceMethod("getBufferData", &RendererWrapper::GetBufferData),
                                                           InstanceMethod("updateBufferData", &RendererWrapper::UpdateBufferData),
                                                           InstanceMethod("updateTextureFromBuffer", &RendererWrapper::UpdateTextureFromBuffer),

                                                           InstanceMethod("loadTextureFromBuffer", &RendererWrapper::LoadTextureFromBuffer),
                                                           InstanceMethod("drawTextureSized", &RendererWrapper::DrawTextureSized),

                                                           InstanceAccessor("width", &RendererWrapper::GetWidth, nullptr),
                                                           InstanceAccessor("height", &RendererWrapper::GetHeight, nullptr),
                                                           InstanceAccessor("targetFPS", nullptr, &RendererWrapper::SetTargetFPS),
                                                           InstanceAccessor("WindowShouldClose", &RendererWrapper::IsWindowClosed, nullptr),
                                                           InstanceMethod("onRender", &RendererWrapper::OnRender),
                                                           InstanceAccessor("onResize", nullptr, &RendererWrapper::onResize),
                                                           InstanceMethod("step", &RendererWrapper::Step),
                                                           InstanceAccessor("input", &RendererWrapper::GetInput, nullptr),
                                                           InstanceAccessor("audio", &RendererWrapper::GetAudio, nullptr),
                                                           InstanceAccessor("FPS", &RendererWrapper::GetFPS, nullptr),

                                                           InstanceMethod("setWindowState", &RendererWrapper::SetWindowState),
                                                           InstanceMethod("markBufferRegionDirty", &RendererWrapper::MarkBufferRegionDirty),
                                                           InstanceMethod("setBufferDimensions", &RendererWrapper::SetBufferDimensions),
                                                           InstanceMethod("getBufferStats", &RendererWrapper::GetBufferStats),
                                                           InstanceMethod("loadImage", &RendererWrapper::LoadImage),
                                                           InstanceMethod("unloadImage", &RendererWrapper::UnloadImage),
                                                           InstanceMethod("ClearColor", &RendererWrapper::SetClearColor),

                                                           // Window-related methods
                                                           InstanceMethod("closeWindow", &RendererWrapper::CloseWindow),
                                                           InstanceMethod("isWindowReady", &RendererWrapper::IsWindowReady),
                                                           InstanceMethod("isWindowFullscreen", &RendererWrapper::IsWindowFullscreen),
                                                           InstanceMethod("isWindowHidden", &RendererWrapper::IsWindowHidden),
                                                           InstanceMethod("isWindowMinimized", &RendererWrapper::IsWindowMinimized),
                                                           InstanceMethod("isWindowMaximized", &RendererWrapper::IsWindowMaximized),
                                                           InstanceMethod("isWindowFocused", &RendererWrapper::IsWindowFocused),
                                                           InstanceMethod("isWindowResized", &RendererWrapper::IsWindowResized),
                                                           InstanceMethod("isWindowState", &RendererWrapper::IsWindowState),
                                                           InstanceMethod("clearWindowState", &RendererWrapper::ClearWindowState),
                                                           InstanceMethod("toggleFullscreen", &RendererWrapper::ToggleFullscreen),
                                                           InstanceMethod("toggleBorderlessWindowed", &RendererWrapper::ToggleBorderlessWindowed),
                                                           InstanceMethod("maximizeWindow", &RendererWrapper::MaximizeWindow),
                                                           InstanceMethod("minimizeWindow", &RendererWrapper::MinimizeWindow),
                                                           InstanceMethod("restoreWindow", &RendererWrapper::RestoreWindow),
                                                           InstanceMethod("setWindowIcon", &RendererWrapper::SetWindowIcon),
                                                           InstanceMethod("setWindowIcons", &RendererWrapper::SetWindowIcons),
                                                           InstanceMethod("setWindowTitle", &RendererWrapper::SetWindowTitle),
                                                           InstanceMethod("setWindowPosition", &RendererWrapper::SetWindowPosition),
                                                           InstanceMethod("setWindowMonitor", &RendererWrapper::SetWindowMonitor),
                                                           InstanceMethod("setWindowMinSize", &RendererWrapper::SetWindowMinSize),
                                                           InstanceMethod("setWindowMaxSize", &RendererWrapper::SetWindowMaxSize),
                                                           InstanceMethod("setWindowSize", &RendererWrapper::SetWindowSize),
                                                           InstanceMethod("setWindowOpacity", &RendererWrapper::SetWindowOpacity),
                                                           InstanceMethod("setWindowFocused", &RendererWrapper::SetWindowFocused),
                                                           InstanceMethod("getWindowHandle", &RendererWrapper::GetWindowHandle),
                                                           InstanceMethod("getScreenWidth", &RendererWrapper::GetScreenWidth),
                                                           InstanceMethod("getScreenHeight", &RendererWrapper::GetScreenHeight),
                                                           InstanceMethod("getRenderWidth", &RendererWrapper::GetRenderWidth),
                                                           InstanceMethod("getRenderHeight", &RendererWrapper::GetRenderHeight),
                                                           InstanceMethod("getMonitorCount", &RendererWrapper::GetMonitorCount),
                                                           InstanceMethod("getCurrentMonitor", &RendererWrapper::GetCurrentMonitor),
                                                           InstanceMethod("getMonitorPosition", &RendererWrapper::GetMonitorPosition),
                                                           InstanceMethod("getMonitorWidth", &RendererWrapper::GetMonitorWidth),
                                                           InstanceMethod("getMonitorHeight", &RendererWrapper::GetMonitorHeight),
                                                           InstanceMethod("getMonitorPhysicalWidth", &RendererWrapper::GetMonitorPhysicalWidth),
                                                           InstanceMethod("getMonitorPhysicalHeight", &RendererWrapper::GetMonitorPhysicalHeight),
                                                           InstanceMethod("getMonitorRefreshRate", &RendererWrapper::GetMonitorRefreshRate),
                                                           InstanceMethod("getWindowPosition", &RendererWrapper::GetWindowPosition),
                                                           InstanceMethod("getWindowScaleDPI", &RendererWrapper::GetWindowScaleDPI),
                                                           InstanceMethod("getMonitorName", &RendererWrapper::GetMonitorName),
                                                           InstanceMethod("setClipboardText", &RendererWrapper::SetClipboardText),
                                                           InstanceMethod("getClipboardText", &RendererWrapper::GetClipboardText),
                                                           InstanceMethod("getClipboardImage", &RendererWrapper::GetClipboardImage),
                                                           InstanceMethod("enableEventWaiting", &RendererWrapper::EnableEventWaiting),
                                                           InstanceMethod("disableEventWaiting", &RendererWrapper::DisableEventWaiting),

                                                           // Cursor-related methods
                                                           InstanceMethod("showCursor", &RendererWrapper::ShowCursor),
                                                           InstanceMethod("hideCursor", &RendererWrapper::HideCursor),
                                                           InstanceMethod("isCursorHidden", &RendererWrapper::IsCursorHidden),
                                                           InstanceMethod("enableCursor", &RendererWrapper::EnableCursor),
                                                           InstanceMethod("disableCursor", &RendererWrapper::DisableCursor),
                                                           InstanceMethod("isCursorOnScreen", &RendererWrapper::IsCursorOnScreen),

                                                           // migrating to zero copy
                                                           InstanceMethod("initSharedBuffers", &RendererWrapper::InitSharedBuffers),

                                                           // extending to support internal cpp commands
                                                             InstanceMethod("processPendingRegions", &RendererWrapper::ProcessPendingRegions),
                                                                 InstanceMethod("loadAtlas", &RendererWrapper::LoadAtlas),
                                                                 InstanceMethod("getAtlasPixel", &RendererWrapper::GetAtlasPixel),
                                                                 InstanceMethod("isAtlasOpaque", &RendererWrapper::IsAtlasOpaque),
                                                                 InstanceMethod("getAtlasData", &RendererWrapper::GetAtlasData),
                                                                 InstanceMethod("getAtlasDataAndFree", &RendererWrapper::GetAtlasDataAndFree),
                                                                 InstanceMethod("freeAtlas", &RendererWrapper::FreeAtlas),
                                                           


                                                           });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("FULLSCREEN", Napi::Number::New(env, WindowFlags::FULLSCREEN));
    exports.Set("RESIZABLE", Napi::Number::New(env, WindowFlags::RESIZABLE));
    exports.Set("UNDECORATED", Napi::Number::New(env, WindowFlags::UNDECORATED));
    exports.Set("ALWAYS_RUN", Napi::Number::New(env, WindowFlags::ALWAYS_RUN));
    exports.Set("VSYNC_HINT", Napi::Number::New(env, WindowFlags::VSYNC_HINT));
    exports.Set("MSAA_4X_HINT", Napi::Number::New(env, WindowFlags::MSAA_4X_HINT));
    exports.Set("Renderer", func);

    // Console control functions
    exports.Set("hideConsole", Napi::Function::New(env, ConsoleControl::HideConsole));
    exports.Set("showConsole", Napi::Function::New(env, ConsoleControl::ShowConsole));

    return exports;
}

RendererWrapper::RendererWrapper(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<RendererWrapper>(info), renderer_(std::make_unique<Renderer>()) {}



// sprite 

Napi::Value RendererWrapper::LoadAtlas(const Napi::CallbackInfo &info)
{
     Napi::Env env = info.Env();
     
     if (info.Length() < 1 || !info[0].IsString())
     {
         Napi::TypeError::New(env, "Expected string path").ThrowAsJavaScriptException();
         return env.Undefined();
     }
     
     std::string path = info[0].As<Napi::String>().Utf8Value();
     uint32_t atlasId = renderer_->LoadAtlas(path);
     
     if (atlasId == 0) {
         Napi::Error::New(env, "Failed to load atlas: " + path).ThrowAsJavaScriptException();
         return env.Undefined();
     }
     
     return Napi::Number::New(env, atlasId);
}

Napi::Value RendererWrapper::GetAtlasPixel(const Napi::CallbackInfo &info)
{
     Napi::Env env = info.Env();

     if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber())
     {
         Napi::TypeError::New(env, "Expected (atlasId, x, y)").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     uint32_t atlasId = info[0].As<Napi::Number>().Uint32Value();
     uint32_t x = info[1].As<Napi::Number>().Uint32Value();
     uint32_t y = info[2].As<Napi::Number>().Uint32Value();

     SpriteAtlas *atlas = renderer_->GetAtlas(atlasId);
     if (!atlas)
     {
         Napi::Error::New(env, "Atlas not found").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     uint32_t pixel = renderer_->GetAtlasPixel(atlas, x, y);
     return Napi::Number::New(env, pixel);
}

Napi::Value RendererWrapper::IsAtlasOpaque(const Napi::CallbackInfo &info)
{
     Napi::Env env = info.Env();

     if (info.Length() < 1 || !info[0].IsNumber())
     {
         Napi::TypeError::New(env, "Expected atlasId").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     uint32_t atlasId = info[0].As<Napi::Number>().Uint32Value();
     SpriteAtlas *atlas = renderer_->GetAtlas(atlasId);
     if (!atlas)
     {
         Napi::Error::New(env, "Atlas not found").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     bool opaque = renderer_->IsAtlasOpaque(atlas);
     return Napi::Boolean::New(env, opaque);
}

Napi::Value RendererWrapper::GetAtlasData(const Napi::CallbackInfo &info)
{
     Napi::Env env = info.Env();

     if (info.Length() < 1 || !info[0].IsNumber())
     {
         Napi::TypeError::New(env, "Expected atlasId").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     uint32_t atlasId = info[0].As<Napi::Number>().Uint32Value();
     SpriteAtlas *atlas = renderer_->GetAtlas(atlasId);
     if (!atlas)
     {
         Napi::Error::New(env, "Atlas not found").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     Napi::Object result = Napi::Object::New(env);
     result.Set("width", Napi::Number::New(env, atlas->width));
     result.Set("height", Napi::Number::New(env, atlas->height));

     // Create Uint8Array from pixel data
     size_t dataSize = atlas->width * atlas->height * 4;
     Napi::ArrayBuffer arrayBuffer = Napi::ArrayBuffer::New(env, dataSize);
     memcpy(arrayBuffer.Data(), atlas->data, dataSize);
     Napi::Uint8Array uint8Array = Napi::Uint8Array::New(env, dataSize, arrayBuffer, 0);

     result.Set("data", uint8Array);

     return result;
}

Napi::Value RendererWrapper::GetAtlasDataAndFree(const Napi::CallbackInfo &info)
{
     Napi::Env env = info.Env();

     if (info.Length() < 1 || !info[0].IsNumber())
     {
         Napi::TypeError::New(env, "Expected atlasId").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     uint32_t atlasId = info[0].As<Napi::Number>().Uint32Value();
     SpriteAtlas *atlas = renderer_->GetAtlas(atlasId);
     if (!atlas)
     {
         Napi::Error::New(env, "Atlas not found").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     Napi::Object result = Napi::Object::New(env);
     result.Set("width", Napi::Number::New(env, atlas->width));
     result.Set("height", Napi::Number::New(env, atlas->height));

     // Create Uint8Array from pixel data
     size_t dataSize = atlas->width * atlas->height * 4;
     Napi::ArrayBuffer arrayBuffer = Napi::ArrayBuffer::New(env, dataSize);
     memcpy(arrayBuffer.Data(), atlas->data, dataSize);
     Napi::Uint8Array uint8Array = Napi::Uint8Array::New(env, dataSize, arrayBuffer, 0);

     result.Set("data", uint8Array);

     // Free the atlas after copying data
     renderer_->FreeAtlas(atlasId);

     return result;
}

Napi::Value RendererWrapper::FreeAtlas(const Napi::CallbackInfo &info)
{
     Napi::Env env = info.Env();

     if (info.Length() < 1 || !info[0].IsNumber())
     {
         Napi::TypeError::New(env, "Expected atlasId").ThrowAsJavaScriptException();
         return env.Undefined();
     }

     uint32_t atlasId = info[0].As<Napi::Number>().Uint32Value();
     renderer_->FreeAtlas(atlasId);

     return env.Undefined();
}

// migrate to shared buffer

Napi::Value RendererWrapper::InitSharedBuffers(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 4)
    {
        Napi::Error::New(env, "Need 4 ArrayBuffers: pixel0, pixel1, pixel2, control").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Create new SharedBufferRefs instance
    SharedBufferRefs *refs = new SharedBufferRefs();
    refs->buffer_size = 0;
    refs->control = nullptr;
    refs->pixel_buffers[0] = refs->pixel_buffers[1] = refs->pixel_buffers[2] = nullptr;
    refs->width = info[4].As<Napi::Number>().Uint32Value();  // set appropriately
    refs->height = info[5].As<Napi::Number>().Uint32Value(); // set appropriately

    // Pixel buffers (args 0..2)
    for (int i = 0; i < 3; ++i)
    {
        if (!info[i].IsArrayBuffer())
        {
            delete refs;
            Napi::Error::New(env, "Args 0-2 must be ArrayBuffers").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::ArrayBuffer pixelbuffer = info[i].As<Napi::ArrayBuffer>();
        void *p = pixelbuffer.Data();
        refs->pixel_buffers[i] = static_cast<uint8_t *>(p);
        refs->refs[i] = Napi::Reference<Napi::ArrayBuffer>::New(pixelbuffer, 1);

        if (i == 0)
        {
            refs->buffer_size = pixelbuffer.ByteLength();
            // Optional: validate expected size matches width*height*4
            size_t expected = static_cast<size_t>(refs->width) * refs->height * 4u;
            if (refs->buffer_size != expected)
            {
                // clean up
                refs->refs[0].Reset();
                delete refs;
                Napi::Error::New(env, "Pixel0 buffer size doesn't match expected texture size").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
    }

    // control buffer (arg 3)
    if (!info[3].IsArrayBuffer())
    {
        // cleanup references we've set
        for (int i = 0; i < 3; ++i)
            if (!refs->refs[i].IsEmpty())
                refs->refs[i].Reset();
        delete refs;
        Napi::TypeError::New(env, "Arg 3 must be an ArrayBuffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::ArrayBuffer control = info[3].As<Napi::ArrayBuffer>();
    size_t control_size = control.ByteLength();
    if (control_size < 4156)
    {
        for (int i = 0; i < 3; ++i)
            if (!refs->refs[i].IsEmpty())
                refs->refs[i].Reset();
        delete refs;
        Napi::Error::New(env, "Control buffer must be at least 4156 bytes").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    void *control_ptr = control.Data();
    if (reinterpret_cast<uintptr_t>(control_ptr) % alignof(std::atomic<uint32_t>) != 0)
    {
        for (int i = 0; i < 3; ++i)
            if (!refs->refs[i].IsEmpty())
                refs->refs[i].Reset();
        delete refs;
        Napi::Error::New(env, "Control buffer is not properly aligned for atomic access").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    refs->control = static_cast<uint32_t *>(control_ptr);
    refs->refs[3] = Napi::Reference<Napi::ArrayBuffer>::New(control, 1);

    // initialize control buffer state (atomic)
    {
        std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(refs->control);
        ctrl[CTRL_JS_WRITE_IDX].store(0u, std::memory_order_relaxed);
        ctrl[CTRL_CPP_READ_IDX].store(1u, std::memory_order_relaxed);
        ctrl[CTRL_GPU_RENDER_IDX].store(2u, std::memory_order_relaxed);
        ctrl[CTRL_DIRTY_FLAG].store(0u, std::memory_order_relaxed);
        ctrl[CTRL_DIRTY_COUNT].store(0u, std::memory_order_relaxed);
    }

    // create raylib texture from pixel buffer 0
    Image image;
    image.data = refs->pixel_buffers[0]; // NOTE: raylib will NOT own this pointer
    image.width = refs->width;
    image.height = refs->height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    // Create texture (make a copy into GPU memory)
    Texture2D texture = LoadTextureFromImage(image);

    Renderer::TextureId texId = renderer_->GenerateTextureId();
    {
        std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
        renderer_->textures_.emplace(texId, texture);

        // store the texture id in the refs and push refs into the vector
        refs->texture_id = texId;
        renderer_->shared_buffers_ref.push_back(refs);
    }

    renderer_->buffers_initialized_ = true;

    // return the bufferRefId = index into the vector
    size_t bufferRefId = renderer_->shared_buffers_ref.size() - 1;
    return Napi::Number::New(env, static_cast<double>(bufferRefId));
}

// Core lifecycle methods
Napi::Value RendererWrapper::Initialize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int width = 800, height = 600;
    std::string title = "Standalone Renderer";

    if (info.Length() > 0 && info[0].IsNumber())
        width = info[0].As<Napi::Number>().Int32Value();
    if (info.Length() > 1 && info[1].IsNumber())
        height = info[1].As<Napi::Number>().Int32Value();
    if (info.Length() > 2 && info[2].IsString())
        title = info[2].As<Napi::String>().Utf8Value();

    bool success = renderer_->Initialize(width, height, title.c_str());

    if (success)
    {

        renderer_->RegisterRenderCallback([this]()
                                          { this->HandleRenderCallback(); });
    }
    return Napi::Boolean::New(env, success);
}

Napi::Value RendererWrapper::Shutdown(const Napi::CallbackInfo &info)
{
    if (renderer_)
    {
        renderer_->Shutdown();
    }
    return info.Env().Undefined();
}

Napi::Value RendererWrapper::BeginFrame(const Napi::CallbackInfo &info)
{
    if (renderer_)
    {
        renderer_->BeginFrame();
    }
    return info.Env().Undefined();
}

Napi::Value RendererWrapper::EndFrame(const Napi::CallbackInfo &info)
{
    if (renderer_)
    {
        renderer_->EndFrame();
    }
    return info.Env().Undefined();
}

// img

Napi::Value RendererWrapper::LoadImage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected path (string) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string path = info[0].As<Napi::String>().Utf8Value();

    // Load image data
    Renderer::ImageData imageData = renderer_->LoadImageFromFile(path);

    if (!imageData.success)
    {
        Napi::Error::New(env, "Failed to load image: " + path).ThrowAsJavaScriptException();
        return env.Null();
    }

    // Create result object with image data and metadata
    Napi::Object result = Napi::Object::New(env);
    result.Set("width", Napi::Number::New(env, imageData.width));
    result.Set("height", Napi::Number::New(env, imageData.height));
    result.Set("format", Napi::Number::New(env, imageData.format));

    // Create ArrayBuffer from image data
    Napi::ArrayBuffer arrayBuffer = Napi::ArrayBuffer::New(env, imageData.data.size());
    memcpy(arrayBuffer.Data(), imageData.data.data(), imageData.data.size());

    // Create Uint8Array view of the buffer
    Napi::Uint8Array uint8Array = Napi::Uint8Array::New(env, imageData.data.size(), arrayBuffer, 0);
    result.Set("data", uint8Array);

    // Store a reference to the image data for cleanup (optional)
    // You could implement a cleanup system if needed

    return result;
}

Napi::Value RendererWrapper::UnloadImage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    // In the current implementation, image data is managed by JavaScript
    // and will be garbage collected automatically. This method is provided
    // for future use if manual memory management is needed.

    // If you implement an image cache system, you would use this method
    // to remove images from the cache.

    return env.Undefined();
}


// Drawing methods
Napi::Value RendererWrapper::Clear(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Color4 color(0.1f, 0.1f, 0.1f, 1.0f);

    if (info.Length() > 0)
    {
        color = ParseColor(info[0], color);
    }

    if (renderer_)
    {
       
        renderer_->Clear(color);
    }
    return env.Undefined();
}

Napi::Value RendererWrapper::DrawRectangle(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 3)
    {
        Napi::TypeError::New(env, "drawRectangle requires position, size, and color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Vec2 position = ParseVec2(info[0]);
    Vec2 size = ParseVec2(info[1]);
    Color4 color = ParseColor(info[2]);

    renderer_->DrawRectangle(position, size, color);
    return env.Undefined();
}

Napi::Value RendererWrapper::DrawCircle(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 3)
    {
        Napi::TypeError::New(env, "drawCircle requires center, radius, and color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Vec2 center = ParseVec2(info[0]);
    float radius = info[1].As<Napi::Number>().FloatValue();
    Color4 color = ParseColor(info[2]);

    renderer_->DrawCircle(center, radius, color);
    return env.Undefined();
}

Napi::Value RendererWrapper::DrawLine(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 3)
    {
        Napi::TypeError::New(env, "drawLine requires start, end, and color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Vec2 start = ParseVec2(info[0]);
    Vec2 end = ParseVec2(info[1]);
    Color4 color = ParseColor(info[2]);
    float thickness = 1.0f;

    if (info.Length() > 3 && info[3].IsNumber())
    {
        thickness = info[3].As<Napi::Number>().FloatValue();
    }

    renderer_->DrawLine(start, end, color, thickness);
    return env.Undefined();
}

Napi::Value RendererWrapper::DrawText(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 4)
    {
        Napi::TypeError::New(env, "drawText requires text, position, fontSize, and color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string text = info[0].As<Napi::String>().Utf8Value();
    Vec2 position = ParseVec2(info[1]);
    int fontSize = info[2].As<Napi::Number>().Int32Value();
    Color4 color = ParseColor(info[3]);

    renderer_->DrawText(text, position, fontSize, color);
    return env.Undefined();
}

// Texture methods
Napi::Value RendererWrapper::LoadTexture(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "loadTexture requires a path string").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string path = info[0].As<Napi::String>().Utf8Value();
    Renderer::TextureId textureId = renderer_->LoadTexture(path);

    return Napi::Number::New(env, textureId);
}

Napi::Value RendererWrapper::UnloadTexture(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "unloadTexture requires a texture ID").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Renderer::TextureId textureId = info[0].As<Napi::Number>().Uint32Value();
    renderer_->UnloadTexture(textureId);

    return env.Undefined();
}

Napi::Value RendererWrapper::DrawTexture(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 2)
    {
        Napi::TypeError::New(env, "drawTexture requires texture ID and position").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    size_t bufferId = info[0].As<Napi::Number>().Uint32Value();
    // TODO: check bounds
    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    Renderer::TextureId textureId = renderer_->shared_buffers_ref[bufferId]->texture_id;
    Vec2 position = ParseVec2(info[1]);

    Color4 tint(1, 1, 1, 1);
    Vec2 size(0, 0);

    if (info.Length() > 2 && info[2].IsObject())
    {
        // Check if it's a size or tint parameter
        Napi::Object param = info[2].As<Napi::Object>();
        if (param.Has("x") && param.Has("y"))
        {
            size = ParseVec2(info[2]);
            if (info.Length() > 3)
            {
                tint = ParseColor(info[3], tint);
            }
        }
        else
        {
            tint = ParseColor(info[2], tint);
        }
    }

    if (size.x == 0 && size.y == 0)
    {
        renderer_->DrawTexture(textureId, position, tint);
    }
    else
    {
        renderer_->DrawTexture(textureId, position, size, tint);
    }

    return env.Undefined();
}

// Render target methods
Napi::Value RendererWrapper::CreateRenderTexture(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 2)
    {
        Napi::TypeError::New(env, "createRenderTexture requires width and height").ThrowAsJavaScriptException();
        return env.Null();
    }

    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();

    Renderer::TextureId textureId = renderer_->CreateRenderTexture(width, height);
    return Napi::Number::New(env, textureId);
}

Napi::Value RendererWrapper::DestroyRenderTexture(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "destroyRenderTexture requires a texture ID").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Renderer::TextureId textureId = info[0].As<Napi::Number>().Uint32Value();
    renderer_->DestroyRenderTexture(textureId);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetRenderTarget(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (!renderer_ || info.Length() < 1)
    {
        Napi::TypeError::New(env, "setRenderTarget requires a texture ID").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Renderer::TextureId textureId = 0; // Default to screen
    if (info[0].IsNumber())
    {
        textureId = info[0].As<Napi::Number>().Uint32Value();
    }

    renderer_->SetRenderTarget(textureId);
    return env.Undefined();
}

// Accessors
Napi::Value RendererWrapper::GetWidth(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (!renderer_)
        return Napi::Number::New(env, 0);
    return Napi::Number::New(env, renderer_->GetWidth());
}

Napi::Value RendererWrapper::GetHeight(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (!renderer_)
        return Napi::Number::New(env, 0);
    return Napi::Number::New(env, renderer_->GetHeight());
}

void RendererWrapper::SetTargetFPS(const Napi::CallbackInfo &info, const Napi::Value &value)
{
    if (!renderer_ || !value.IsNumber())
        return;
    renderer_->SetTargetFPS(value.As<Napi::Number>().Int32Value());
}

// Helper methods
Color4 RendererWrapper::ParseColor(const Napi::Value &colorValue, const Color4 &defaultColor)
{
    if (!colorValue.IsObject())
        return defaultColor;

    Napi::Object colorObj = colorValue.As<Napi::Object>();
    float r = defaultColor.r, g = defaultColor.g, b = defaultColor.b, a = defaultColor.a;

    if (colorObj.Has("r") && colorObj.Get("r").IsNumber())
        r = colorObj.Get("r").As<Napi::Number>().FloatValue();
    if (colorObj.Has("g") && colorObj.Get("g").IsNumber())
        g = colorObj.Get("g").As<Napi::Number>().FloatValue();
    if (colorObj.Has("b") && colorObj.Get("b").IsNumber())
        b = colorObj.Get("b").As<Napi::Number>().FloatValue();
    if (colorObj.Has("a") && colorObj.Get("a").IsNumber())
        a = colorObj.Get("a").As<Napi::Number>().FloatValue();

    return Color4(r, g, b, a);
}

Vec2 RendererWrapper::ParseVec2(const Napi::Value &vecValue, const Vec2 &defaultVec)
{
    if (!vecValue.IsObject())
        return defaultVec;

    Napi::Object vecObj = vecValue.As<Napi::Object>();
    float x = defaultVec.x, y = defaultVec.y;

    if (vecObj.Has("x") && vecObj.Get("x").IsNumber())
        x = vecObj.Get("x").As<Napi::Number>().FloatValue();
    if (vecObj.Has("y") && vecObj.Get("y").IsNumber())
        y = vecObj.Get("y").As<Napi::Number>().FloatValue();

    return Vec2(x, y);
}

Napi::Value RendererWrapper::OnRender(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        Napi::TypeError::New(env, "onRender requires a function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Function callback = info[0].As<Napi::Function>();
    renderCallbacks_.push_back(Napi::Persistent(callback));

    return env.Undefined();
}

void RendererWrapper::HandleRenderCallback()
{
    for (auto &callbackRef : renderCallbacks_)
    {
        try
        {
            Napi::Function callback = callbackRef.Value();
            callback.Call({});
        }
        catch (const Napi::Error &e)
        {
            std::cout << "Error in render callback: " << e.Message() << std::endl;
        }
    }
}

Napi::Value RendererWrapper::Step(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    bool continuing = renderer_->Step();
    return Napi::Boolean::New(env, continuing);
}

Napi::Value RendererWrapper::GetInput(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (inputWrapper_.IsEmpty())
    {
        Napi::Object inputObj = InputWrapper::constructor.New({});
        inputWrapper_ = Napi::Persistent(inputObj);
    }

    return inputWrapper_.Value();
}

Napi::Value RendererWrapper::GetAudio(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (audioWrapper_.IsEmpty())
    {
        Napi::Object audioObj = AudioWrapper::constructor.New({});
        audioWrapper_ = Napi::Persistent(audioObj);
    }

    return audioWrapper_.Value();
}

// buffer

Napi::Value RendererWrapper::CreateSharedBuffer(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected size (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    size_t size = info[0].As<Napi::Number>().Uint32Value();

    // optional: width and height for dirty region tracking
    int width = 0, height = 0;
    if (info.Length() >= 3 && info[1].IsNumber() && info[2].IsNumber())
    {
        width = info[1].As<Napi::Number>().Int32Value();
        height = info[2].As<Napi::Number>().Int32Value();
    }

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    SharedBuffer *buffer = new SharedBuffer(size, width, height);
    renderer_->shared_buffers_.push_back(buffer);

    // index act as id
    return Napi::Number::New(env, renderer_->shared_buffers_.size() - 1);
}

Napi::Value RendererWrapper::IsBufferDirty(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Boolean::New(env, renderer_->shared_buffers_[bufferId]->IsDirty());
}

Napi::Value RendererWrapper::MarkBufferDirty(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    renderer_->shared_buffers_[bufferId]->MarkDirty();
    return env.Undefined();
}

Napi::Value RendererWrapper::MarkBufferRegionDirty(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 5 || !info[0].IsNumber() || !info[1].IsNumber() ||
        !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId, x, y, width, height (numbers)").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();
    int x = info[1].As<Napi::Number>().Int32Value();
    int y = info[2].As<Napi::Number>().Int32Value();
    int width = info[3].As<Napi::Number>().Int32Value();
    int height = info[4].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    renderer_->shared_buffers_[bufferId]->MarkRegionDirty(x, y, width, height);
    return env.Undefined();
}

Napi::Value RendererWrapper::SetBufferDimensions(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId, width, height (numbers)").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();
    int width = info[1].As<Napi::Number>().Int32Value();
    int height = info[2].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    renderer_->shared_buffers_[bufferId]->SetDimensions(width, height);
    return env.Undefined();
}

Napi::Value RendererWrapper::GetBufferStats(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId (number)").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    SharedBuffer *buffer = renderer_->shared_buffers_[bufferId];
    std::vector<DirtyRect> regions = buffer->GetDirtyRegions();

    Napi::Object stats = Napi::Object::New(env);
    stats.Set("dirtyRegionCount", Napi::Number::New(env, regions.size()));
    stats.Set("isDirty", Napi::Boolean::New(env, buffer->IsDirty()));
    stats.Set("width", Napi::Number::New(env, buffer->GetWidth()));
    stats.Set("height", Napi::Number::New(env, buffer->GetHeight()));

    // Calculate total dirty area
    int total_dirty_pixels = 0;
    for (const auto &rect : regions)
    {
        total_dirty_pixels += rect.Area();
    }
    stats.Set("dirtyPixels", Napi::Number::New(env, total_dirty_pixels));

    int total_pixels = buffer->GetWidth() * buffer->GetHeight();
    float coverage = total_pixels > 0 ? static_cast<float>(total_dirty_pixels) / total_pixels : 0.0f;
    stats.Set("dirtyCoverage", Napi::Number::New(env, coverage));

    return stats;
}

Napi::Value RendererWrapper::GetBufferData(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    SharedBuffer *buffer = renderer_->shared_buffers_[bufferId];

    Napi::ArrayBuffer arrayBuffer = Napi::ArrayBuffer::New(env, buffer->GetSize());
    memcpy(arrayBuffer.Data(), buffer->GetReadData(), buffer->GetSize()); // read is always up to date
    return Napi::Uint8Array::New(env, buffer->GetSize(), arrayBuffer, 0);
}

Napi::Value RendererWrapper::UpdateBufferData(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsTypedArray())
    {
        Napi::TypeError::New(env, "Expected bufferId (number) and data (Uint8Array) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();
    Napi::Uint8Array jsData = info[1].As<Napi::Uint8Array>();

    bool has_region = false;
    int region_x = 0, region_y = 0, region_width = 0, region_height = 0;

    if (info.Length() >= 6 &&
        info[2].IsNumber() && info[3].IsNumber() &&
        info[4].IsNumber() && info[5].IsNumber())
    {
        region_x = info[2].As<Napi::Number>().Int32Value();
        region_y = info[3].As<Napi::Number>().Int32Value();
        region_width = info[4].As<Napi::Number>().Int32Value();
        region_height = info[5].As<Napi::Number>().Int32Value();
        has_region = true;
    }

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    SharedBuffer *buffer = renderer_->shared_buffers_[bufferId];
    lock.~lock_guard();

    if (!buffer->TryLockForWrite())
    {
        Napi::Error::New(env, "Buffer is currently being written to").ThrowAsJavaScriptException();
        return env.Null();
    }

    // resize buffer if needed
    if (!has_region && jsData.ByteLength() != buffer->GetSize())
    {
        buffer->Resize(jsData.ByteLength());
    }

    void *writeData = buffer->GetWriteData();

    if (has_region)
    {
        int buffer_width = buffer->GetWidth();
        int bytes_per_pixel = 4; // Assuming RGBA

        if (buffer_width <= 0)
        {
            Napi::Error::New(env, "Buffer dimensions not set. Cannot perform region update.").ThrowAsJavaScriptException();
            return env.Null();
        }

        if (region_x < 0 || region_y < 0 ||
            region_x + region_width > buffer_width ||
            region_y + region_height > buffer->GetHeight())
        {
            Napi::Error::New(env, "Region out of bounds").ThrowAsJavaScriptException();
            return env.Null();
        }

        uint8_t *dest = static_cast<uint8_t *>(writeData);
        const uint8_t *src = jsData.Data();

        for (int row = 0; row < region_height; ++row)
        {
            int dest_offset = ((region_y + row) * buffer_width + region_x) * bytes_per_pixel;
            int src_offset = row * region_width * bytes_per_pixel;
            memcpy(dest + dest_offset, src + src_offset, region_width * bytes_per_pixel);
        }

        // Mark only the updated region as dirty
        buffer->MarkRegionDirty(region_x, region_y, region_width, region_height);
    }
    else
    {
        memcpy(writeData, jsData.Data(), jsData.ByteLength());
        buffer->MarkFullyDirty();
    }

    buffer->MarkDirty(); // for swap to trigger in render
    buffer->UnlockWrite();

    return env.Undefined();
}

Napi::Value RendererWrapper::LoadTextureFromBuffer(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (renderer_->IsWindowClosed())
    {
        Napi::Error::New(env, "Window not initialized").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber())
    {
        Napi::TypeError::New(env, "Expected bufferId, width, and height (numbers) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int bufferId = info[0].As<Napi::Number>().Int32Value();
    int width = info[1].As<Napi::Number>().Int32Value();
    int height = info[2].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);
    if (bufferId < 0 || bufferId >= static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    SharedBuffer *buffer = renderer_->shared_buffers_[bufferId];
    lock.~lock_guard();                       // release lock early from here it's duble buffers
    size_t expectedSize = width * height * 4; // RGBA

    if (buffer->GetSize() != expectedSize)
    {
        Napi::Error::New(env, "Buffer size doesn't match texture dimensions").ThrowAsJavaScriptException();
        return env.Null();
    }
    buffer->SwapBuffers();

    // create texture from buffer data
    Image image = {
        .data = buffer->GetReadData(),
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,

    };

    Texture2D texture = LoadTextureFromImage(image);

    Renderer::TextureId id = renderer_->GenerateTextureId();
    renderer_->textures_.emplace(id, texture);

    return Napi::Number::New(env, id);
}

Napi::Value RendererWrapper::UpdateTextureFromBuffer(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (renderer_->IsWindowClosed())
    {
        Napi::Error::New(env, "Window not initialized").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected textureId (number) and bufferId (number) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int textureId = info[0].As<Napi::Number>().Int32Value();
    int bufferId = info[1].As<Napi::Number>().Int32Value();

    std::lock_guard<std::mutex> lock(renderer_->buffers_mutex_);

    if (textureId < 0 || textureId > static_cast<int>(renderer_->textures_.size()))
    {
        Napi::Error::New(env, "Invalid texture ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (bufferId < 0 || bufferId > static_cast<int>(renderer_->shared_buffers_.size()))
    {
        Napi::Error::New(env, "Invalid buffer ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    Texture2D &texture = renderer_->textures_[textureId];
    SharedBuffer *buffer = renderer_->shared_buffers_[bufferId];

    if (texture.id == 0)
    {
        Napi::Error::New(env, "Texture already unloaded").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Expect RGBA8 (4 bytes per pixel)
    size_t expectedSize = static_cast<size_t>(texture.width) * static_cast<size_t>(texture.height) * 4;
    if (buffer->GetSize() != expectedSize)
    {
        Napi::Error::New(env, "Buffer size doesn't match texture dimensions (expected width*height*4 bytes)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::vector<DirtyRect> dirty_regions = buffer->GetDirtyRegions();
    buffer->SwapBuffers();
    if (dirty_regions.empty())
    {
        // cause if full dirty there exist one region
        buffer->MarkClean();
        return env.Undefined();
    }

    const void *data = buffer->GetReadData();
    // const void *data = buffer->GetWriteData(); // DEBUG: use write directly w/o swapping
    const uint8_t *pixel_data = static_cast<const uint8_t *>(data);

    for (const auto &region : dirty_regions)
    {
        if (!region.IsValid())
            continue;

        // calculate offset into buffer for this region
        Rectangle rect = {
            static_cast<float>(region.x),
            static_cast<float>(region.y),
            static_cast<float>(region.width),
            static_cast<float>(region.height)};

        // extract region data
        std::vector<uint8_t> region_data(region.width * region.height * 4);

        for (int row = 0; row < region.height; ++row)
        {
            int src_offset = ((region.y + row) * texture.width + region.x) * 4;
            int dst_offset = row * region.width * 4;
            memcpy(region_data.data() + dst_offset,
                   pixel_data + src_offset,
                   region.width * 4);
        }

        // Update only this rectangle on GPU
        ::UpdateTextureRec(texture, rect, region_data.data());
    }

    // ::UpdateTexture(texture, buffer->GetReadData());

    buffer->MarkClean();

    return env.Undefined();
}

Napi::Value RendererWrapper::DrawTextureSized(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (renderer_->IsWindowClosed())
    {
        Napi::Error::New(env, "Window not initialized").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Expect: textureId, x, y, width, height, [colorObj]
    if (info.Length() < 5 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber())
    {
        Napi::TypeError::New(env, "Expected textureId, x, y, width, height (numbers) and optional color object").ThrowAsJavaScriptException();
        return env.Null();
    }

    int textureId = info[0].As<Napi::Number>().Int32Value();
    int x = info[1].As<Napi::Number>().Int32Value();
    int y = info[2].As<Napi::Number>().Int32Value();
    int width = info[3].As<Napi::Number>().Int32Value();
    int height = info[4].As<Napi::Number>().Int32Value();

    if (textureId < 0 || textureId >= static_cast<int>(renderer_->textures_.size()))
    {
        Napi::Error::New(env, "Invalid texture ID").ThrowAsJavaScriptException();
        return env.Null();
    }

    Texture2D &texture = renderer_->textures_[textureId];

    if (texture.id == 0)
    {
        Napi::Error::New(env, "Texture already unloaded").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Default tint
    Color tint = WHITE;
    if (info.Length() >= 6 && info[5].IsObject())
    {
        Napi::Object colorObj = info[5].As<Napi::Object>();
        if (colorObj.Has("r") && colorObj.Has("g") && colorObj.Has("b"))
        {
            tint.r = colorObj.Get("r").As<Napi::Number>().Uint32Value();
            tint.g = colorObj.Get("g").As<Napi::Number>().Uint32Value();
            tint.b = colorObj.Get("b").As<Napi::Number>().Uint32Value();
            tint.a = colorObj.Has("a") ? colorObj.Get("a").As<Napi::Number>().Uint32Value() : 255;
        }
    }

    // Use DrawTexturePro to draw into a destination rectangle sized as requested.
    Rectangle srcRec = {0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    Rectangle dstRec = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)};
    Vector2 origin = {0.0f, 0.0f};
    float rotation = 0.0f;

    ::DrawTexturePro(texture, srcRec, dstRec, origin, rotation, tint);

    return env.Undefined();
}

Napi::Value RendererWrapper::DrawTexturePro(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    // Expect at least textureId, srcPos, srcSize, destPos, destSize
    if (!renderer_ || info.Length() < 5)
    {
        Napi::TypeError::New(env, "drawTexturePro requires textureId, srcPos, srcSize, destPos and destSize").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // texture id
    Renderer::TextureId textureId = info[0].As<Napi::Number>().Uint32Value();

    Vec2 srcPos = ParseVec2(info[1]);
    Vec2 srcSize = ParseVec2(info[2]);
    Vec2 destPos = ParseVec2(info[3]);
    Vec2 destSize = ParseVec2(info[4]);

    Color4 tint(1, 1, 1, 1);
    if (info.Length() > 5 && !info[5].IsUndefined())
    {
        tint = ParseColor(info[5], tint);
    }

    // call the renderer
    renderer_->DrawTextureRegion(textureId, srcPos, srcSize, destPos, destSize, tint);

    return env.Undefined();
}

// ============================================================================
// Window-related methods
// ============================================================================

Napi::Value RendererWrapper::CloseWindow(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::CloseWindow();
    return env.Undefined();
}

Napi::Value RendererWrapper::IsWindowReady(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowReady());
}

Napi::Value RendererWrapper::IsWindowFullscreen(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowFullscreen());
}

Napi::Value RendererWrapper::IsWindowHidden(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowHidden());
}

Napi::Value RendererWrapper::IsWindowMinimized(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowMinimized());
}

Napi::Value RendererWrapper::IsWindowMaximized(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowMaximized());
}

Napi::Value RendererWrapper::IsWindowFocused(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowFocused());
}

Napi::Value RendererWrapper::IsWindowResized(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsWindowResized());
}

Napi::Value RendererWrapper::IsWindowState(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected flag (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    unsigned int flag = info[0].As<Napi::Number>().Uint32Value();
    return Napi::Boolean::New(env, ::IsWindowState(flag));
}

Napi::Value RendererWrapper::ClearWindowState(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected flags (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    unsigned int flags = info[0].As<Napi::Number>().Uint32Value();
    ::ClearWindowState(flags);
    return env.Undefined();
}

Napi::Value RendererWrapper::ToggleFullscreen(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::ToggleFullscreen();
    return env.Undefined();
}

Napi::Value RendererWrapper::ToggleBorderlessWindowed(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::ToggleBorderlessWindowed();
    return env.Undefined();
}

Napi::Value RendererWrapper::MaximizeWindow(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::MaximizeWindow();
    return env.Undefined();
}

Napi::Value RendererWrapper::MinimizeWindow(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::MinimizeWindow();
    return env.Undefined();
}

Napi::Value RendererWrapper::RestoreWindow(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::RestoreWindow();
    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowIcon(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected imagePath (string) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string imagePath = info[0].As<Napi::String>().Utf8Value();

    // Load image using stb_image
    int width, height, channels;
    unsigned char *data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);

    if (!data)
    {
        Napi::Error::New(env, "Failed to load image: " + imagePath).ThrowAsJavaScriptException();
        return env.Null();
    }

    Image image = {
        .data = data,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    ::SetWindowIcon(image);
    stbi_image_free(data);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowIcons(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsArray())
    {
        Napi::TypeError::New(env, "Expected imagePaths (array of strings) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array imagePaths = info[0].As<Napi::Array>();
    uint32_t count = imagePaths.Length();

    if (count == 0)
    {
        Napi::Error::New(env, "imagePaths array cannot be empty").ThrowAsJavaScriptException();
        return env.Null();
    }

    Image *images = new Image[count];
    unsigned char **dataPointers = new unsigned char *[count];

    for (uint32_t i = 0; i < count; ++i)
    {
        Napi::Value pathVal = imagePaths.Get(i);
        if (!pathVal.IsString())
        {
            delete[] images;
            delete[] dataPointers;
            Napi::TypeError::New(env, "All elements in imagePaths must be strings").ThrowAsJavaScriptException();
            return env.Null();
        }

        std::string imagePath = pathVal.As<Napi::String>().Utf8Value();

        int width, height, channels;
        unsigned char *data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);

        if (!data)
        {
            // Cleanup previously loaded images
            for (uint32_t j = 0; j < i; ++j)
            {
                stbi_image_free(dataPointers[j]);
            }
            delete[] images;
            delete[] dataPointers;

            Napi::Error::New(env, "Failed to load image: " + imagePath).ThrowAsJavaScriptException();
            return env.Null();
        }

        dataPointers[i] = data;
        images[i] = {
            .data = data,
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        };
    }

    ::SetWindowIcons(images, count);

    // Cleanup
    for (uint32_t i = 0; i < count; ++i)
    {
        stbi_image_free(dataPointers[i]);
    }
    delete[] images;
    delete[] dataPointers;

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowTitle(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected title (string) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string title = info[0].As<Napi::String>().Utf8Value();
    ::SetWindowTitle(title.c_str());

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowPosition(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected x, y (numbers) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int x = info[0].As<Napi::Number>().Int32Value();
    int y = info[1].As<Napi::Number>().Int32Value();
    ::SetWindowPosition(x, y);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowMonitor(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    ::SetWindowMonitor(monitor);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowMinSize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected width, height (numbers) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();
    ::SetWindowMinSize(width, height);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowMaxSize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected width, height (numbers) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();
    ::SetWindowMaxSize(width, height);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowSize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected width, height (numbers) arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();
    ::SetWindowSize(width, height);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowOpacity(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected opacity (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    float opacity = info[0].As<Napi::Number>().FloatValue();
    ::SetWindowOpacity(opacity);

    return env.Undefined();
}

Napi::Value RendererWrapper::SetWindowFocused(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::SetWindowFocused();
    return env.Undefined();
}

Napi::Value RendererWrapper::GetWindowHandle(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    void *handle = ::GetWindowHandle();
    // Return as a pointer value (as a number, since JS doesn't have native pointers)
    return Napi::Number::New(env, reinterpret_cast<uintptr_t>(handle));
}

Napi::Value RendererWrapper::GetScreenWidth(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, ::GetScreenWidth());
}

Napi::Value RendererWrapper::GetScreenHeight(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, ::GetScreenHeight());
}

Napi::Value RendererWrapper::GetRenderWidth(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, ::GetRenderWidth());
}

Napi::Value RendererWrapper::GetRenderHeight(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, ::GetRenderHeight());
}

Napi::Value RendererWrapper::GetMonitorCount(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, ::GetMonitorCount());
}

Napi::Value RendererWrapper::GetCurrentMonitor(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, ::GetCurrentMonitor());
}

Napi::Value RendererWrapper::GetMonitorPosition(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    Vector2 pos = ::GetMonitorPosition(monitor);

    Napi::Object result = Napi::Object::New(env);
    result.Set("x", Napi::Number::New(env, pos.x));
    result.Set("y", Napi::Number::New(env, pos.y));

    return result;
}

Napi::Value RendererWrapper::GetMonitorWidth(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    return Napi::Number::New(env, ::GetMonitorWidth(monitor));
}

Napi::Value RendererWrapper::GetMonitorHeight(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    return Napi::Number::New(env, ::GetMonitorHeight(monitor));
}

Napi::Value RendererWrapper::GetMonitorPhysicalWidth(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    return Napi::Number::New(env, ::GetMonitorPhysicalWidth(monitor));
}

Napi::Value RendererWrapper::GetMonitorPhysicalHeight(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    return Napi::Number::New(env, ::GetMonitorPhysicalHeight(monitor));
}

Napi::Value RendererWrapper::GetMonitorRefreshRate(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    return Napi::Number::New(env, ::GetMonitorRefreshRate(monitor));
}

Napi::Value RendererWrapper::GetWindowPosition(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Vector2 pos = ::GetWindowPosition();

    Napi::Object result = Napi::Object::New(env);
    result.Set("x", Napi::Number::New(env, pos.x));
    result.Set("y", Napi::Number::New(env, pos.y));

    return result;
}

Napi::Value RendererWrapper::GetWindowScaleDPI(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Vector2 scale = ::GetWindowScaleDPI();

    Napi::Object result = Napi::Object::New(env);
    result.Set("x", Napi::Number::New(env, scale.x));
    result.Set("y", Napi::Number::New(env, scale.y));

    return result;
}

Napi::Value RendererWrapper::GetMonitorName(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Expected monitor (number) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    int monitor = info[0].As<Napi::Number>().Int32Value();
    const char *name = ::GetMonitorName(monitor);

    return Napi::String::New(env, name);
}

Napi::Value RendererWrapper::SetClipboardText(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected text (string) argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string text = info[0].As<Napi::String>().Utf8Value();
    ::SetClipboardText(text.c_str());

    return env.Undefined();
}

Napi::Value RendererWrapper::GetClipboardText(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    const char *text = ::GetClipboardText();

    return Napi::String::New(env, text ? text : "");
}

Napi::Value RendererWrapper::GetClipboardImage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Image image = ::GetClipboardImage();

    if (image.data == NULL)
    {
        Napi::Error::New(env, "Failed to get clipboard image").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Create result object with image metadata
    Napi::Object result = Napi::Object::New(env);
    result.Set("width", Napi::Number::New(env, image.width));
    result.Set("height", Napi::Number::New(env, image.height));
    result.Set("format", Napi::Number::New(env, image.format));
    result.Set("mipmaps", Napi::Number::New(env, image.mipmaps));

    // Create ArrayBuffer from image data
    size_t dataSize = image.width * image.height * 4; // assuming RGBA
    Napi::ArrayBuffer arrayBuffer = Napi::ArrayBuffer::New(env, dataSize);
    memcpy(arrayBuffer.Data(), image.data, dataSize);

    Napi::Uint8Array uint8Array = Napi::Uint8Array::New(env, dataSize, arrayBuffer, 0);
    result.Set("data", uint8Array);

    // Cleanup
    ::UnloadImage(image);

    return result;
}

Napi::Value RendererWrapper::EnableEventWaiting(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::EnableEventWaiting();
    return env.Undefined();
}

Napi::Value RendererWrapper::DisableEventWaiting(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::DisableEventWaiting();
    return env.Undefined();
}

// ============================================================================
// Cursor-related methods
// ============================================================================

Napi::Value RendererWrapper::ShowCursor(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::ShowCursor();
    return env.Undefined();
}

Napi::Value RendererWrapper::HideCursor(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::HideCursor();
    return env.Undefined();
}

Napi::Value RendererWrapper::IsCursorHidden(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsCursorHidden());
}

Napi::Value RendererWrapper::EnableCursor(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::EnableCursor();
    return env.Undefined();
}

Napi::Value RendererWrapper::DisableCursor(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ::DisableCursor();
    return env.Undefined();
}

Napi::Value RendererWrapper::IsCursorOnScreen(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, ::IsCursorOnScreen());
}
