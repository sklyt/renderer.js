#include "renderer.h"
#include <iostream>
#include <filesystem>
#include "input_manager.h"

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
}

void Renderer::UpdateSizeIfNeeded()
{
    if (IsWindowResized())
    {
        width_ = GetScreenWidth();
        height_ = GetScreenHeight();

        if(onResize_){
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

    initialized_ = true;
    std::cout << "Renderer initialized: " << width << "x" << height << std::endl;
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


Renderer::ImageData Renderer::LoadImageFromFile(const std::string& path) {
    ImageData result;
    result.success = false;
    
  
    Image image = ::LoadImage(path.c_str());
    
    if (image.data == NULL) {
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

void Renderer::UnloadImageData(ImageData& imageData) {
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
    ProcessBufferUpdates();
    BeginFrame();
    // Clear(clearColor);
    for (auto &callback : renderCallbacks_)
    {
        callback();
    }
    EndFrame();
    return !IsWindowClosed();
}

// Buffer 


void Renderer::SwapAllBuffers()
{
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    
    for (auto& buffer : shared_buffers_) {
        if (buffer) {
            buffer->SwapBuffers();
        }
    }
}



void Renderer::StartAsyncBufferProcessing()
{
    if (async_processing_.load()) return;
    
    async_processing_.store(true);
    buffer_update_thread_ = std::thread(&Renderer::BufferUpdateThread, this);
}


void Renderer::StopAsyncBufferProcessing()
{
    async_processing_.store(false);
    if (buffer_update_thread_.joinable()) {
        buffer_update_thread_.join();
    }
}


void Renderer::BufferUpdateThread()
{
    while (async_processing_.load()) {
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