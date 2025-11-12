#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <memory>
#include "vector"
#include <functional>
#include "shared_buffer.h"

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

    int GetCurrentFPS(){
        return ::GetFPS();
    }
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
    Color4 clearColor = Color4(0.2, 0.3, 0.4, 1.0);
};