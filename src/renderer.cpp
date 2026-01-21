#include "renderer.h"
#include <iostream>
#include <filesystem>
#include "input_manager.h"
#include "audio_manager.h"
#include <debugger.h>

Renderer::Renderer() : width_(0), height_(0), initialized_(false),
                       nextTextureId_(1), nextRenderTextureId_(1000000),
                       currentRenderTargetId_(0), inTextureMode_(false) {}

Renderer::~Renderer()
{
    if (initialized_)
    {
        Shutdown();
    }

    // Cleanup textures
    for (auto &kv : textures_)
    {
        UnloadTexture(kv.second.id);
    }
    textures_.clear();

    // Cleanup render textures
    for (auto &kv : internalRenderTextures_)
    {
        UnloadRenderTexture(kv.second);
    }
    internalRenderTextures_.clear();

    std::lock_guard<std::mutex> lock(buffers_mutex_);
    for (SharedBufferRefs *s : shared_buffers_ref)
    {
        if (!s)
            continue;
        for (int i = 0; i < 4; ++i)
        {
            if (!s->refs[i].IsEmpty())
                s->refs[i].Reset();
        }
        // if raylib texture exists, unload it:
        auto it = textures_.find(s->texture_id);
        if (it != textures_.end())
        {
            ::UnloadTexture(it->second);
            textures_.erase(it);
        }
        delete s;
    }
    shared_buffers_ref.clear();
}

void Renderer::UpdateSizeIfNeeded()
{
    if (IsWindowResized())
    {
        width_ = GetScreenWidth();
        height_ = GetScreenHeight();

        if (onResize_)
        {
            onResize_(width_, height_);
        }
    }
}

bool Renderer::Initialize(int width, int height, const char *title)
{
    width_ = width;
    height_ = height;

    InitWindow(width, height, title);

    if (!IsWindowReady())
    {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    if (!AudioManager::Instance().Initialize())
    {
        Debugger::Instance().LogError("Failed to initialize audio manager");
        return false;
    }

    initialized_ = true;
    Debugger::Instance().LogInfo("Renderer initialized:" + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void Renderer::Shutdown()
{
    if (!initialized_)
        return;

    CloseWindow();
    initialized_ = false;
    renderCallbacks_.clear();
    std::cout << "Renderer shutdown" << std::endl;
}

void Renderer::BeginFrame()
{
    BeginDrawing();
}

void Renderer::EndFrame()
{
    EndDrawing();
}

void Renderer::Clear(const Color4 &color)
{
    ClearBackground(color.ToRaylib());
}

void Renderer::DrawRectangle(const Vec2 &pos, const Vec2 &size, const Color4 &color)
{
    ::DrawRectangle((int)pos.x, (int)pos.y, (int)size.x, (int)size.y, color.ToRaylib());
}

void Renderer::DrawCircle(const Vec2 &center, float radius, const Color4 &color)
{
    ::DrawCircle((int)center.x, (int)center.y, radius, color.ToRaylib());
}

void Renderer::DrawLine(const Vec2 &start, const Vec2 &end, const Color4 &color, float thickness)
{
    ::DrawLineEx(start.ToRaylib(), end.ToRaylib(), thickness, color.ToRaylib());
}

void Renderer::DrawText(const std::string &text, const Vec2 &pos, int fontSize, const Color4 &color)
{
    ::DrawText(text.c_str(), (int)pos.x, (int)pos.y, fontSize, color.ToRaylib());
}

// img

Renderer::ImageData Renderer::LoadImageFromFile(const std::string &path)
{
    ImageData result;
    result.success = false;

    Image image = ::LoadImage(path.c_str());

    if (image.data == NULL)
    {
        std::cerr << "Failed to load image: " << path << std::endl;
        return result;
    }

    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    result.width = image.width;
    result.height = image.height;
    result.format = image.format;

    int dataSize = image.width * image.height * 4; // 4 bytes per pixel (RGBA)
    result.data.resize(dataSize);

    memcpy(result.data.data(), image.data, dataSize);

    ::UnloadImage(image);

    result.success = true;
    return result;
}

void Renderer::UnloadImageData(ImageData &imageData)
{
    imageData.data.clear();
    imageData.width = 0;
    imageData.height = 0;
    imageData.format = 0;
    imageData.success = false;
}

// Texture management
Renderer::TextureId Renderer::GenerateTextureId()
{
    return nextTextureId_++;
}

Renderer::TextureId Renderer::LoadTexture(const std::string &path)
{
    if (!std::filesystem::exists(path))
    {
        std::cerr << "Texture file not found: " << path << std::endl;
        return 0;
    }

    Texture2D texture = ::LoadTexture(path.c_str());
    if (texture.id == 0)
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    TextureId id = GenerateTextureId();
    textures_.emplace(id, texture);
    std::cout << "Texture loaded: " << path << " (ID: " << id << ")" << std::endl;

    return id;
}

void Renderer::UnloadTexture(TextureId textureId)
{
    auto it = textures_.find(textureId);
    if (it != textures_.end())
    {
        UnloadTexture(it->second.id);
        textures_.erase(it);
        std::cout << "Texture unloaded: " << textureId << std::endl;
    }
}

void Renderer::DrawTexture(TextureId textureId, const Vec2 &pos, const Color4 &tint)
{
    // Check internal render textures first
    auto renderIt = internalRenderTextures_.find(textureId);
    if (renderIt != internalRenderTextures_.end())
    {
        ::DrawTextureV(renderIt->second.texture, pos.ToRaylib(), tint.ToRaylib());
        return;
    }

    // Check regular textures
    auto textureIt = textures_.find(textureId);
    if (textureIt != textures_.end())
    {
        ::DrawTextureV(textureIt->second, pos.ToRaylib(), tint.ToRaylib());
    }
}

void Renderer::DrawTexture(TextureId textureId, const Vec2 &pos, const Vec2 &size, const Color4 &tint)
{
    auto renderIt = internalRenderTextures_.find(textureId);
    if (renderIt != internalRenderTextures_.end())
    {
        Rectangle dest = {pos.x, pos.y, size.x, size.y};
        Rectangle src = {0, 0, (float)renderIt->second.texture.width, (float)renderIt->second.texture.height};
        ::DrawTexturePro(renderIt->second.texture, src, dest, Vector2{0, 0}, 0.0f, tint.ToRaylib());
        return;
    }

    auto textureIt = textures_.find(textureId);
    if (textureIt != textures_.end())
    {
        Rectangle dest = {pos.x, pos.y, size.x, size.y};
        Rectangle src = {0, 0, (float)textureIt->second.width, (float)textureIt->second.height};
        ::DrawTexturePro(textureIt->second, src, dest, Vector2{0, 0}, 0.0f, tint.ToRaylib());
    }
}

void Renderer::DrawTextureRegion(TextureId textureId, const Vec2 &srcPos, const Vec2 &srcSize,
                                 const Vec2 &destPos, const Vec2 &destSize, const Color4 &tint)
{
    auto renderIt = internalRenderTextures_.find(textureId);
    if (renderIt != internalRenderTextures_.end())
    {
        Rectangle src = {srcPos.x, srcPos.y, srcSize.x, srcSize.y};
        Rectangle dest = {destPos.x, destPos.y, destSize.x, destSize.y};
        ::DrawTexturePro(renderIt->second.texture, src, dest, Vector2{0, 0}, 0.0f, tint.ToRaylib());
        return;
    }

    auto textureIt = textures_.find(textureId);
    if (textureIt != textures_.end())
    {
        Rectangle src = {srcPos.x, srcPos.y, srcSize.x, srcSize.y};
        Rectangle dest = {destPos.x, destPos.y, destSize.x, destSize.y};
        ::DrawTexturePro(textureIt->second, src, dest, Vector2{0, 0}, 0.0f, tint.ToRaylib());
    }
}

// Render targets
Renderer::TextureId Renderer::CreateRenderTexture(int width, int height)
{
    RenderTexture2D rt = LoadRenderTexture(width, height);
    TextureId id = nextRenderTextureId_++;
    internalRenderTextures_.emplace(id, rt);
    std::cout << "Created render texture: " << id << " (" << width << "x" << height << ")" << std::endl;
    return id;
}

void Renderer::DestroyRenderTexture(TextureId id)
{
    auto it = internalRenderTextures_.find(id);
    if (it != internalRenderTextures_.end())
    {
        UnloadRenderTexture(it->second);
        internalRenderTextures_.erase(it);
        std::cout << "Destroyed render texture: " << id << std::endl;
    }
}

void Renderer::SetRenderTarget(TextureId id)
{
    if (id == 0)
    {
        if (inTextureMode_)
        {
            EndTextureMode();
            inTextureMode_ = false;
            currentRenderTargetId_ = 0;
        }
        return;
    }

    auto it = internalRenderTextures_.find(id);
    if (it == internalRenderTextures_.end())
    {
        std::cerr << "Unknown render target: " << id << std::endl;
        return;
    }

    if (inTextureMode_)
    {
        EndTextureMode();
    }

    BeginTextureMode(it->second);
    inTextureMode_ = true;
    currentRenderTargetId_ = id;
}

Vec2 Renderer::GetRenderTextureSize(TextureId id) const
{
    auto it = internalRenderTextures_.find(id);
    if (it != internalRenderTextures_.end())
    {
        return Vec2((float)it->second.texture.width, (float)it->second.texture.height);
    }

    auto textureIt = textures_.find(id);
    if (textureIt != textures_.end())
    {
        return Vec2((float)textureIt->second.width, (float)textureIt->second.height);
    }

    return Vec2(0, 0);
}

// Utility for steping thru the renderer(usually for games) it's a single call vs multiple from

void Renderer::RegisterRenderCallback(std::function<void()> callback)
{
    renderCallbacks_.push_back(callback);
}

bool Renderer::Step()
{
    UpdateSizeIfNeeded();
    SwapAllBuffers();
    BeginFrame();
    Clear(clearColor);
    for (auto &callback : renderCallbacks_)
    {
        callback();
    }
    
    // Keep music streams fed (required for raylib streaming music playback)
    AudioManager::Instance().Update();
    
    EndFrame();
    return !IsWindowClosed();
}

// Buffer

void Renderer::SwapBuffers(size_t bufRefId)
{
    // Debugger::Instance().LogInfo("swapping buffer " + std::to_string(bufRefId));
    // Debugger::Instance().LogInfo("buffer size " + std::to_string(shared_buffers_ref.size()));
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return;
    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s)
        return;
    // Debugger::Instance().LogInfo("buffer textureId " + std::to_string(s->texture_id));

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);
    uint32_t dirty = ctrl[CTRL_DIRTY_FLAG].load(std::memory_order_acquire);
    if (dirty == 0)
        return;

    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_relaxed);
    uint32_t cpp_read = ctrl[CTRL_CPP_READ_IDX].load(std::memory_order_relaxed);
    uint32_t gpu_render = ctrl[CTRL_GPU_RENDER_IDX].load(std::memory_order_relaxed);

    uint32_t new_gpu = cpp_read;
    uint32_t new_ready = js_write;
    uint32_t new_write = gpu_render;

    // process dirty regions for the buffer that JS wrote into (js_write)
    ProcessDirtyRegions(bufRefId, js_write);

    // rotate indices
    ctrl[CTRL_GPU_RENDER_IDX].store(new_gpu, std::memory_order_relaxed);
    ctrl[CTRL_CPP_READ_IDX].store(new_ready, std::memory_order_relaxed);
    ctrl[CTRL_JS_WRITE_IDX].store(new_write, std::memory_order_release);

    // Clear dirty flag AFTER swapping
    ctrl[CTRL_DIRTY_FLAG].store(0u, std::memory_order_release);
}

// renderer.cpp
void Renderer::ProcessDirtyRegions(size_t bufRefId, uint32_t buffer_idx)
{
    // std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return;
    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s || !s->control)
        return;

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);
    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);
    uint8_t *pixel_data = s->pixel_buffers[buffer_idx];

    // retrieve texture by id
    auto it = textures_.find(s->texture_id);
    if (it == textures_.end())
    {
        // no texture, nothing to upload
        return;
    }

    Texture2D &texture = it->second;

    if (dirty_count == 0)
    {
        // no regions => upload entire buffer
        UploadEntireBuffer(bufRefId, buffer_idx);
        return;
    }

    if (dirty_count > MAX_DIRTY_REGIONS)
        dirty_count = MAX_DIRTY_REGIONS;

    for (uint32_t i = 0; i < dirty_count; ++i)
    {
        uint32_t offset = CTRL_DIRTY_REGIONS + (i * 4);
        uint32_t x = ctrl[offset + 0].load(std::memory_order_relaxed);
        uint32_t y = ctrl[offset + 1].load(std::memory_order_relaxed);
        uint32_t w = ctrl[offset + 2].load(std::memory_order_relaxed);
        uint32_t h = ctrl[offset + 3].load(std::memory_order_relaxed);

        if (w == 0 || h == 0)
            continue;
        UploadRegionToGPU(s->texture_id, pixel_data, x, y, w, h, s->width, s->height);
    }

    // Clear dirty count after processing
    ctrl[CTRL_DIRTY_COUNT].store(0u, std::memory_order_release);
}

// Upload a rectangle region for given texture id.
// pixel_data points to the full RGBA8 pixel buffer (width*height*4).
void Renderer::UploadRegionToGPU(TextureId texId, uint8_t *pixel_data,
                                 uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                 uint32_t fullWidth, uint32_t fullHeight)
{
    if (w == 0 || h == 0)
        return;

    // std::lock_guard<std::mutex> lock(buffers_mutex_);
    auto it = textures_.find(texId);
    if (it == textures_.end())
        return;
    Texture2D &texture = it->second;

    // clamp rect inside texture bounds
    if (x >= fullWidth || y >= fullHeight)
        return;
    if (x + w > fullWidth)
        w = fullWidth - x;
    if (y + h > fullHeight)
        h = fullHeight - y;

    size_t regionSize = static_cast<size_t>(w) * h * 4u;
    std::vector<uint8_t> region_data;
    region_data.resize(regionSize);

    // copy scanlines
    for (uint32_t row = 0; row < h; ++row)
    {
        const uint8_t *src = pixel_data + ((static_cast<size_t>(y + row) * fullWidth + x) * 4u);
        uint8_t *dst = region_data.data() + (static_cast<size_t>(row) * w * 4u);
        memcpy(dst, src, static_cast<size_t>(w) * 4u);
    }

    Rectangle rect = {static_cast<float>(x),
                      static_cast<float>(y),
                      static_cast<float>(w),
                      static_cast<float>(h)};

    // Update only this rect on GPU
    ::UpdateTextureRec(texture, rect, region_data.data());
}

// Upload entire buffer
void Renderer::UploadEntireBuffer(size_t bufRefId, uint32_t buffer_idx)
{
    // std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return;
    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s)
        return;

    uint8_t *pixel_data = s->pixel_buffers[buffer_idx];
    auto it = textures_.find(s->texture_id);
    if (it == textures_.end())
        return;
    Texture2D &texture = it->second;

    // full upload: UpdateTexture expects pointer to full RGBA buffer
    ::UpdateTexture(texture, pixel_data);
}

void Renderer::SwapAllBuffers()
{

    // for (auto &buffer : shared_buffers_)
    // {
    //     if (buffer)
    //     {
    //         buffer->SwapBuffers();
    //     }
    // }
    size_t n;
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        n = shared_buffers_ref.size();
    }
    for (size_t i = 0; i < n; i++)
    {
        SwapBuffers(i);
    };
}

void Renderer::StartAsyncBufferProcessing()
{
    if (async_processing_.load())
        return;

    async_processing_.store(true);
    buffer_update_thread_ = std::thread(&Renderer::BufferUpdateThread, this);
}

void Renderer::StopAsyncBufferProcessing()
{
    async_processing_.store(false);
    if (buffer_update_thread_.joinable())
    {
        buffer_update_thread_.join();
    }
}

void Renderer::BufferUpdateThread()
{
    while (async_processing_.load())
    {
        // process buffer swaps at controlled rate (e.g., 60Hz)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        SwapAllBuffers();
    }
}

void Renderer::ProcessBufferUpdates()
{
    // manual buffer processing
    SwapAllBuffers();
}