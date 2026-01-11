#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <memory>
#include "vector"
#include <functional>
#include "shared_buffer.h"
#include <thread>
#include "napi.h"
#include <atomic>
#include <cstdint>

// control buffer offsets (in u32 units)
#define CTRL_JS_WRITE_IDX 0
#define CTRL_CPP_READ_IDX 1
#define CTRL_GPU_RENDER_IDX 2
#define CTRL_DIRTY_FLAG 3
#define CTRL_DIRTY_COUNT 4
#define CTRL_DIRTY_REGIONS 5 // start of [x,y,w,h] array

#define MAX_DIRTY_REGIONS 256

struct SharedBufferRefs
{
    uint8_t* pixel_buffers[3]; // pointers to JS ArrayBuffers
    uint32_t *control;      // pointer to control buffer
    Napi::Reference<Napi::ArrayBuffer>  refs[4];       // Keep buffers alive
    size_t buffer_size;     // size of each pixel buffer
    uint32_t width;
    uint32_t height;
    unsigned int texture_id;

};

using onReziseCallback = std::function<void(int width, int height)>;

class Color4
{
public:
    float r, g, b, a;

    Color4(float r = 0, float g = 0, float b = 0, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}

    Color ToRaylib() const
    {
        return {(unsigned char)(r * 255), (unsigned char)(g * 255),
                (unsigned char)(b * 255), (unsigned char)(a * 255)};
    }
};

class Vec2
{
public:
    float x, y;

    Vec2(float x = 0, float y = 0) : x(x), y(y) {}

    Vector2 ToRaylib() const { return {x, y}; }
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    struct ImageData
    {
        std::vector<uint8_t> data;
        int width;
        int height;
        int format; // Raylib PixelFormat
        bool success;
    };

    ImageData LoadImageFromFile(const std::string &path);
    void UnloadImageData(ImageData &imageData);

    // Core lifecycle
    bool Initialize(int width, int height, const char *title);
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    // Window management
    void UpdateSizeIfNeeded();
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    void SetTargetFPS(int fps) { ::SetTargetFPS(fps); }
    bool IsWindowClosed() { return WindowShouldClose(); }

    // Basic drawing
    void Clear(const Color4 &color);
    void DrawRectangle(const Vec2 &pos, const Vec2 &size, const Color4 &color);
    void DrawCircle(const Vec2 &center, float radius, const Color4 &color);
    void DrawLine(const Vec2 &start, const Vec2 &end, const Color4 &color, float thickness = 1.0f);
    void DrawText(const std::string &text, const Vec2 &pos, int fontSize, const Color4 &color);

    // Texture management
    typedef unsigned int TextureId;

    TextureId LoadTexture(const std::string &path);
    void UnloadTexture(TextureId textureId);
    void DrawTexture(TextureId textureId, const Vec2 &pos, const Color4 &tint = Color4(1, 1, 1, 1));
    void DrawTexture(TextureId textureId, const Vec2 &pos, const Vec2 &size, const Color4 &tint = Color4(1, 1, 1, 1));
    void DrawTextureRegion(TextureId textureId, const Vec2 &srcPos, const Vec2 &srcSize,
                           const Vec2 &destPos, const Vec2 &destSize, const Color4 &tint = Color4(1, 1, 1, 1));

    // Render targets
    TextureId CreateRenderTexture(int width, int height);
    void DestroyRenderTexture(TextureId id);
    void SetRenderTarget(TextureId id);
    Vec2 GetRenderTextureSize(TextureId id) const;

    // utility
    void RegisterRenderCallback(std::function<void()> callback);
    bool Step();
    std::vector<SharedBuffer *> shared_buffers_;
    std::mutex buffers_mutex_;
    TextureId GenerateTextureId();
    std::unordered_map<TextureId, Texture2D> textures_;

    int GetCurrentFPS()
    {
        return ::GetFPS();
    }

    // buffers
    void SwapAllBuffers();
    void ProcessBufferUpdates();
    void SwapBuffers(size_t bufRefId);
    void ProcessDirtyRegions(size_t bufRefId, uint32_t buffer_idx) ;
    void UploadEntireBuffer(size_t bufRefId, uint32_t buffer_idx);
    void UploadRegionToGPU(TextureId texId, uint8_t* pixel_data,
                                 uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                 uint32_t fullWidth, uint32_t fullHeight);

    void StartAsyncBufferProcessing();
    void StopAsyncBufferProcessing();

    onReziseCallback onResize_;
    std::vector<SharedBufferRefs *>  shared_buffers_ref;
    
    bool buffers_initialized_ = false;
  Color4 clearColor = Color4(0.2, 0.3, 0.4, 1.0);
private:
    int width_;
    int height_;
    bool initialized_;

    // Internal texture management
    TextureId nextTextureId_;
    TextureId nextRenderTextureId_;

    std::unordered_map<TextureId, RenderTexture2D> internalRenderTextures_;

    TextureId currentRenderTargetId_;
    bool inTextureMode_;

    std::vector<std::function<void()>> renderCallbacks_;
  

    std::atomic<bool> async_processing_{false};
    std::thread buffer_update_thread_;

    void BufferUpdateThread();
};