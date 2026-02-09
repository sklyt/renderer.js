#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

    {
        std::lock_guard<std::mutex> lock(atlas_mutex_);
        for (auto &pair : atlases_)
        {
            delete pair.second;
        }
        atlases_.clear();
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

    {
        std::lock_guard<std::mutex> lock(sprite_mutex_);
        for (auto &pair : sprites_)
        {
            delete pair.second;
        }
        sprites_.clear();
    }

    if (initialized_)
    {
        Shutdown();
    }
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

// sprite

uint32_t Renderer::LoadAtlas(const std::string &path)
{
    int width, height, channels;

    // Force RGBA (4 channels)
    uint8_t *pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (!pixels)
    {
        Debugger::Instance().LogError("Failed to load atlas: " + path);
        return 0; // Invalid ID
    }

    SpriteAtlas *atlas = new SpriteAtlas();
    atlas->width = static_cast<uint32_t>(width);
    atlas->height = static_cast<uint32_t>(height);
    atlas->data = pixels; // Transfer ownership

    std::lock_guard<std::mutex> lock(atlas_mutex_);
    uint32_t id = next_atlas_id_++;
    atlases_[id] = atlas;

    Debugger::Instance().LogInfo("Loaded atlas " + std::to_string(id) +
                                 ": " + std::to_string(width) + "x" + std::to_string(height));

    return id;
}

SpriteAtlas *Renderer::GetAtlas(uint32_t atlasId)
{
    std::lock_guard<std::mutex> lock(atlas_mutex_);
    auto it = atlases_.find(atlasId);
    if (it == atlases_.end())
        return nullptr;
    return it->second;
}

uint32_t Renderer::GetAtlasPixel(SpriteAtlas *atlas, uint32_t x, uint32_t y)
{
    if (!atlas || x >= atlas->width || y >= atlas->height)
        return 0;

    uint32_t idx = (y * atlas->width + x) * 4;
    uint8_t r = atlas->data[idx];
    uint8_t g = atlas->data[idx + 1];
    uint8_t b = atlas->data[idx + 2];
    uint8_t a = atlas->data[idx + 3];

    // Pack into RGBA32
    return (a << 24) | (b << 16) | (g << 8) | r;
}

bool Renderer::IsAtlasOpaque(SpriteAtlas *atlas)
{
    if (!atlas)
        return false;

    size_t pixelCount = atlas->width * atlas->height;
    for (size_t i = 0; i < pixelCount; i++)
    {
        if (atlas->data[i * 4 + 3] < 255)
            return false;
    }
    return true;
}

void Renderer::FreeAtlas(uint32_t atlasId)
{
    std::lock_guard<std::mutex> lock(atlas_mutex_);
    auto it = atlases_.find(atlasId);
    if (it != atlases_.end())
    {
        delete it->second;
        atlases_.erase(it);
    }
}

// renderer.cpp

uint32_t Renderer::CreateSprite(uint32_t atlasId, uint32_t frameWidth, uint32_t frameHeight,
                                uint32_t frameCount, bool opaque)
{
    // Validate atlas exists
    SpriteAtlas *atlas = GetAtlas(atlasId);
    if (!atlas)
    {
        Debugger::Instance().LogError("CreateSprite: invalid atlasId " + std::to_string(atlasId));
        return 0;
    }

    AnimatedSprite *sprite = new AnimatedSprite();
    sprite->atlasId = atlasId;
    sprite->frameWidth = frameWidth;
    sprite->frameHeight = frameHeight;
    sprite->framesPerRow = atlas->width / frameWidth;
    sprite->opaque = opaque ? 1 : 0;

    // Default position (will be updated from JS)
    sprite->x = 0;
    sprite->y = 0;
    sprite->scaleX = 1.0f;
    sprite->scaleY = 1.0f;
    sprite->rotation = 0;
    sprite->currentFrame = 0;

    std::lock_guard<std::mutex> lock(sprite_mutex_);
    uint32_t id = next_sprite_id_++;
    sprites_[id] = sprite;

    Debugger::Instance().LogInfo("Created sprite " + std::to_string(id) +
                                 " from atlas " + std::to_string(atlasId));

    return id;
}

AnimatedSprite *Renderer::GetSprite(uint32_t spriteId)
{
    std::lock_guard<std::mutex> lock(sprite_mutex_);
    auto it = sprites_.find(spriteId);
    if (it == sprites_.end())
        return nullptr;
    return it->second;
}

void Renderer::UpdateSprite(uint32_t spriteId, float x, float y, float rotation,
                            float scaleX, float scaleY, uint32_t frame,
                            uint8_t flipH, uint8_t flipV)
{
    AnimatedSprite *sprite = GetSprite(spriteId);
    if (!sprite)
        return;

    sprite->x = x;
    sprite->y = y;
    sprite->rotation = rotation;
    sprite->scaleX = scaleX;
    sprite->scaleY = scaleY;
    sprite->currentFrame = frame;
    sprite->flipH = flipH;
    sprite->flipV = flipV;
}

void Renderer::DestroySprite(uint32_t spriteId)
{
    std::lock_guard<std::mutex> lock(sprite_mutex_);
    auto it = sprites_.find(spriteId);
    if (it != sprites_.end())
    {
        delete it->second;
        sprites_.erase(it);
    }
}

struct FrameRect
{
    uint32_t x, y, w, h;
};

FrameRect GetFrameRect(AnimatedSprite *sprite, uint32_t frameIndex)
{
    uint32_t col = frameIndex % sprite->framesPerRow;
    uint32_t row = frameIndex / sprite->framesPerRow;

    return {
        col * sprite->frameWidth,
        row * sprite->frameHeight,
        sprite->frameWidth,
        sprite->frameHeight};
}


// Fast path: opaque, no rotation, nearest neighbor
void BlitSpriteNN_Opaque(uint8_t* dstBuffer, uint32_t dstWidth, uint32_t dstHeight,
                         const ScreenRect& dstRect,
                         const SpriteAtlas* atlas, const FrameRect& srcRect,
                         bool flipH, bool flipV)
{
    // Clamp dest rect to buffer bounds
    int32_t dstX = dstRect.x;
    int32_t dstY = dstRect.y;
    int32_t dstW = dstRect.width;
    int32_t dstH = dstRect.height;
    
    if (dstX >= static_cast<int32_t>(dstWidth) || dstY >= static_cast<int32_t>(dstHeight))
        return;
    if (dstX + dstW <= 0 || dstY + dstH <= 0)
        return;
    
   
    int32_t clipLeft = 0, clipTop = 0;
    if (dstX < 0) {
        clipLeft = -dstX;
        dstW += dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        clipTop = -dstY;
        dstH += dstY;
        dstY = 0;
    }
    if (dstX + dstW > static_cast<int32_t>(dstWidth))
        dstW = dstWidth - dstX;
    if (dstY + dstH > static_cast<int32_t>(dstHeight))
        dstH = dstHeight - dstY;
    
    if (dstW <= 0 || dstH <= 0)
        return;
    
    const uint8_t* srcData = atlas->data;
    const uint32_t srcWidth = atlas->width;
    const float scaleX = static_cast<float>(srcRect.w) / dstRect.width;
    const float scaleY = static_cast<float>(srcRect.h) / dstRect.height;
    
    // Row-by-row blit
    for (int32_t row = 0; row < dstH; row++) {
        int32_t screenY = dstY + row;
        int32_t localY = clipTop + row;
        
        // Map to source Y
        float srcYf = (localY + 0.5f) * scaleY;
        uint32_t srcY = static_cast<uint32_t>(srcYf);
        if (flipV)
            srcY = (srcRect.h - 1) - srcY;
        srcY += srcRect.y;
        
        uint32_t dstRowBase = screenY * dstWidth * 4;
        uint32_t srcRowBase = srcY * srcWidth * 4;
        
        for (int32_t col = 0; col < dstW; col++) {
            int32_t screenX = dstX + col;
            int32_t localX = clipLeft + col;
            
            // Map to source X
            float srcXf = (localX + 0.5f) * scaleX;
            uint32_t srcX = static_cast<uint32_t>(srcXf);
            if (flipH)
                srcX = (srcRect.w - 1) - srcX;
            srcX += srcRect.x;
            
            uint32_t dstIdx = dstRowBase + screenX * 4;
            uint32_t srcIdx = srcRowBase + srcX * 4;
            
            // Direct copy (opaque, no blend)
            dstBuffer[dstIdx + 0] = srcData[srcIdx + 0]; // R
            dstBuffer[dstIdx + 1] = srcData[srcIdx + 1]; // G
            dstBuffer[dstIdx + 2] = srcData[srcIdx + 2]; // B
            dstBuffer[dstIdx + 3] = 255;                  // A
        }
    }
}

// With alpha blending
void BlitSpriteNN_Alpha(uint8_t* dstBuffer, uint32_t dstWidth, uint32_t dstHeight,
                        const ScreenRect& dstRect,
                        const SpriteAtlas* atlas, const FrameRect& srcRect,
                        bool flipH, bool flipV)
{

    int32_t dstX = dstRect.x;
    int32_t dstY = dstRect.y;
    int32_t dstW = dstRect.width;
    int32_t dstH = dstRect.height;
    
    if (dstX >= static_cast<int32_t>(dstWidth) || dstY >= static_cast<int32_t>(dstHeight))
        return;
    if (dstX + dstW <= 0 || dstY + dstH <= 0)
        return;
    
    int32_t clipLeft = 0, clipTop = 0;
    if (dstX < 0) {
        clipLeft = -dstX;
        dstW += dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        clipTop = -dstY;
        dstH += dstY;
        dstY = 0;
    }
    if (dstX + dstW > static_cast<int32_t>(dstWidth))
        dstW = dstWidth - dstX;
    if (dstY + dstH > static_cast<int32_t>(dstHeight))
        dstH = dstHeight - dstY;
    
    if (dstW <= 0 || dstH <= 0)
        return;
    
    const uint8_t* srcData = atlas->data;
    const uint32_t srcWidth = atlas->width;
    const float scaleX = static_cast<float>(srcRect.w) / dstRect.width;
    const float scaleY = static_cast<float>(srcRect.h) / dstRect.height;
    
    for (int32_t row = 0; row < dstH; row++) {
        int32_t screenY = dstY + row;
        int32_t localY = clipTop + row;
        
        float srcYf = (localY + 0.5f) * scaleY;
        uint32_t srcY = static_cast<uint32_t>(srcYf);
        if (flipV)
            srcY = (srcRect.h - 1) - srcY;
        srcY += srcRect.y;
        
        uint32_t dstRowBase = screenY * dstWidth * 4;
        uint32_t srcRowBase = srcY * srcWidth * 4;
        
        for (int32_t col = 0; col < dstW; col++) {
            int32_t screenX = dstX + col;
            int32_t localX = clipLeft + col;
            
            float srcXf = (localX + 0.5f) * scaleX;
            uint32_t srcX = static_cast<uint32_t>(srcXf);
            if (flipH)
                srcX = (srcRect.w - 1) - srcX;
            srcX += srcRect.x;
            
            uint32_t dstIdx = dstRowBase + screenX * 4;
            uint32_t srcIdx = srcRowBase + srcX * 4;
            
            uint8_t sR = srcData[srcIdx + 0];
            uint8_t sG = srcData[srcIdx + 1];
            uint8_t sB = srcData[srcIdx + 2];
            uint8_t sA = srcData[srcIdx + 3];
            
            if (sA == 255) {
                // Fully opaque pixel - direct copy
                dstBuffer[dstIdx + 0] = sR;
                dstBuffer[dstIdx + 1] = sG;
                dstBuffer[dstIdx + 2] = sB;
                dstBuffer[dstIdx + 3] = 255;
            } else if (sA > 0) {
                // Alpha blend
                uint8_t dR = dstBuffer[dstIdx + 0];
                uint8_t dG = dstBuffer[dstIdx + 1];
                uint8_t dB = dstBuffer[dstIdx + 2];
                uint8_t dA = dstBuffer[dstIdx + 3];
                
                uint32_t inv = 255 - sA;
                
                uint8_t outR = ((sR * sA + dR * inv) + 127) / 255;
                uint8_t outG = ((sG * sA + dG * inv) + 127) / 255;
                uint8_t outB = ((sB * sA + dB * inv) + 127) / 255;
                uint8_t outA = ((sA * 255 + dA * inv) + 127) / 255;
                
                dstBuffer[dstIdx + 0] = outR;
                dstBuffer[dstIdx + 1] = outG;
                dstBuffer[dstIdx + 2] = outB;
                dstBuffer[dstIdx + 3] = outA;
            }
            // If sA == 0, skip (fully transparent)
        }
    }
}

// cam work


ScreenRect WorldToScreen(const CameraState &cam, float worldX, float worldY,
                         float worldW, float worldH)
{
    // Translate to camera space
    float dx = worldX - cam.worldX;
    float dy = worldY - cam.worldY;

    // Rotate by negative camera rotation (inverse transform)
    float cosA = cosf(-cam.rotation);
    float sinA = sinf(-cam.rotation);

    float camX = dx * cosA - dy * sinA;
    float camY = dx * sinA + dy * cosA;

    // Apply zoom
    camX *= cam.zoom;
    camY *= cam.zoom;

    // Convert to screen coordinates (camera center = screen center)
    float screenX = camX + cam.viewWidth / 2.0f;
    float screenY = camY + cam.viewHeight / 2.0f;

    return {
        static_cast<int32_t>(screenX),
        static_cast<int32_t>(screenY),
        static_cast<uint32_t>(worldW * cam.zoom),
        static_cast<uint32_t>(worldH * cam.zoom)};
}

void Renderer::DrawSprite(uint32_t spriteId, size_t bufRefId)
{
    //  Debugger::Instance().LogInfo("DrawSprite called - spriteId: " + std::to_string(spriteId) + ", bufRefId: " + std::to_string(bufRefId));
     
     // process pending JS writes
     ProcessPendingRegions(bufRefId);
     

     AnimatedSprite* sprite = GetSprite(spriteId);
     if (!sprite) {
         Debugger::Instance().LogWarn("DrawSprite - Early return: sprite not found (spriteId: " + std::to_string(spriteId) + ")");
         return;
     }
     

     SpriteAtlas* atlas = GetAtlas(sprite->atlasId);
     if (!atlas) {
         Debugger::Instance().LogWarn("DrawSprite - Early return: atlas not found (atlasId: " + std::to_string(sprite->atlasId) + ")");
         return;
     }
     
     //  get js camera state
     CameraState cam = GetCameraState(bufRefId);
     
     // calculate world-space sprite bounds
     float worldW = sprite->frameWidth * sprite->scaleX;
     float worldH = sprite->frameHeight * sprite->scaleY;
     
     // frustum cull
     if (!IsInFrustum(cam, sprite->x, sprite->y, worldW, worldH)) {
        //  Debugger::Instance().LogInfo("DrawSprite - Early return: sprite outside frustum (pos: " + std::to_string(sprite->x) + ", " + std::to_string(sprite->y) + ")");
         return; // Off-screen, don't draw
     }
     
     // convert to screen space
     ScreenRect screenRect = WorldToScreen(cam, sprite->x, sprite->y, worldW, worldH);
     

     std::lock_guard<std::mutex> lock(buffers_mutex_);
     if (bufRefId >= shared_buffers_ref.size()) {
         Debugger::Instance().LogWarn("DrawSprite - Early return: bufRefId out of range (bufRefId: " + std::to_string(bufRefId) + ", size: " + std::to_string(shared_buffers_ref.size()) + ")");
         return;
     }
     
     SharedBufferRefs* s = shared_buffers_ref[bufRefId];
     if (!s || !s->control) {
         Debugger::Instance().LogWarn("DrawSprite - Early return: SharedBufferRefs or control is null (bufRefId: " + std::to_string(bufRefId) + ")");
         return;
     }
    
    std::atomic<uint32_t>* ctrl = reinterpret_cast<std::atomic<uint32_t>*>(s->control);
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    uint8_t* dstBuffer = s->pixel_buffers[js_write];
    
    // get frame rect from atlas
    FrameRect srcRect = GetFrameRect(sprite, sprite->currentFrame);
    
    // blit pixels (choose fast path if opaque)
    if (sprite->opaque) {
        BlitSpriteNN_Opaque(dstBuffer, s->width, s->height, screenRect, 
                           atlas, srcRect, sprite->flipH, sprite->flipV);
    } else {
        BlitSpriteNN_Alpha(dstBuffer, s->width, s->height, screenRect,
                          atlas, srcRect, sprite->flipH, sprite->flipV);
    }

    // mark dirty region
    // Add region for the sprite's screen bounds
    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);
    
    if (dirty_count < MAX_DIRTY_REGIONS) {
        uint32_t offset = CTRL_DIRTY_REGIONS + (dirty_count * 4);
        
        // Clamp to buffer bounds
        int32_t rx = screenRect.x < 0 ? 0 : screenRect.x;
        int32_t ry = screenRect.y < 0 ? 0 : screenRect.y;
        uint32_t rw = screenRect.width;
        uint32_t rh = screenRect.height;
        
        if (rx + rw > s->width)
            rw = s->width - rx;
        if (ry + rh > s->height)
            rh = s->height - ry;
        
        ctrl[offset + 0] = rx;
        ctrl[offset + 1] = ry;
        ctrl[offset + 2] = rw;
        ctrl[offset + 3] = rh;
        
        ctrl[CTRL_DIRTY_COUNT].store(dirty_count + 1, std::memory_order_release);
    }

    // Debug output
    // Debugger::Instance().LogInfo("DrawSprite - ID: " + std::to_string(spriteId) + 
    //                               " | Atlas: " + std::to_string(sprite->atlasId) +
    //                               " | Frame: " + std::to_string(sprite->currentFrame));
    
    // Debugger::Instance().LogInfo("DrawSprite - World Pos: (" + std::to_string(sprite->x) + 
    //                               ", " + std::to_string(sprite->y) + 
    //                               ") | World Size: " + std::to_string(worldW) + 
    //                               "x" + std::to_string(worldH));
    
    // Debugger::Instance().LogInfo("DrawSprite - Screen Rect: (" + std::to_string(screenRect.x) + 
    //                               ", " + std::to_string(screenRect.y) + 
    //                               ") | Screen Size: " + std::to_string(screenRect.width) + 
    //                               "x" + std::to_string(screenRect.height));
    
    // Debugger::Instance().LogInfo("DrawSprite - Dirty Region Count: " + std::to_string(dirty_count) + 
    //                               "/" + std::to_string(MAX_DIRTY_REGIONS) +
    //                               " | Buffer ID: " + std::to_string(bufRefId) +
    //                               " | Buffer Size: " + std::to_string(s->width) + 
    //                               "x" + std::to_string(s->height));
    
    // Debugger::Instance().LogInfo("DrawSprite - Scale: (" + std::to_string(sprite->scaleX) + 
    //                               ", " + std::to_string(sprite->scaleY) + 
    //                               ") | Flip: H=" + std::to_string(sprite->flipH) + 
    //                               " V=" + std::to_string(sprite->flipV) +
    //                               " | Opaque: " + std::to_string(sprite->opaque));
    
    // We do NOT swap buffers or set dirty flag here
    // That only happens on canvas.upload() in javascript, on js has the final say
}


// anim 


uint32_t Renderer::CreateSpriteWithAnimations(uint32_t atlasId, uint32_t frameWidth,
                                              uint32_t frameHeight, bool opaque,
                                              const std::vector<AnimationDef>& animations)
{
    // Create base sprite
    AnimatedSprite* sprite = new AnimatedSprite();
    sprite->atlasId = atlasId;
    sprite->frameWidth = frameWidth;
    sprite->frameHeight = frameHeight;
    sprite->framesPerRow = 0; // Will be set from atlas
    sprite->opaque = opaque ? 1 : 0;
    
    // Calculate frames per row from atlas
    SpriteAtlas* atlas = GetAtlas(atlasId);
    if (atlas) {
        sprite->framesPerRow = atlas->width / frameWidth;
    }
    
    // Load animations
    uint32_t nextAnimId = 1;
    for (const auto& animDef : animations) {
        SpriteAnimation* anim = new SpriteAnimation();
        anim->name = animDef.name;
        anim->id = nextAnimId++;
        anim->fps = animDef.fps;
        anim->loop = animDef.loop;
        anim->frameCount = animDef.frames.size();
        
        // Copy frame array
        anim->frames = new uint32_t[anim->frameCount];
        for (size_t i = 0; i < anim->frameCount; i++) {
            anim->frames[i] = animDef.frames[i];
        }
        
        // Store in dictionaries
        sprite->animations[anim->id] = anim;
        sprite->animationNames[anim->name] = anim->id;
    }
    
    // Set initial frame to first frame of first animation (if any)
    if (!animations.empty() && animations[0].frames.size() > 0) {
        sprite->currentFrame = animations[0].frames[0];
    }
    
    // Register sprite
    std::lock_guard<std::mutex> lock(sprite_mutex_);
    uint32_t id = next_sprite_id_++;
    sprites_[id] = sprite;
    
    Debugger::Instance().LogInfo("Created sprite " + std::to_string(id) + 
                                 " with " + std::to_string(animations.size()) + " animations");
    
    return id;
}

void Renderer::PlayAnimation(uint32_t spriteId, const std::string& animName)
{
    AnimatedSprite* sprite = GetSprite(spriteId);
    if (!sprite)
        return;
    
    // Lookup animation ID by name
    auto it = sprite->animationNames.find(animName);
    if (it == sprite->animationNames.end()) {
        Debugger::Instance().LogWarn("Animation '" + animName + "' not found on sprite " + 
                                        std::to_string(spriteId));
        return;
    }
    
    PlayAnimationById(spriteId, it->second);
}

void Renderer::PlayAnimationById(uint32_t spriteId, uint32_t animId)
{
    AnimatedSprite* sprite = GetSprite(spriteId);
    if (!sprite)
        return;
    
    // Get animation
    auto it = sprite->animations.find(animId);
    if (it == sprite->animations.end())
        return;
    
    SpriteAnimation* anim = it->second;
    
    // Switch to this animation
    sprite->currentAnimationId = animId;
    sprite->playing = 1;
    sprite->frameTimer = 0.0f;
    
    // Set first frame
    if (anim->frameCount > 0) {
        sprite->currentFrame = anim->frames[0];
    }
}


void Renderer::UpdateSpriteAnimations(float deltaTime)
{
    std::lock_guard<std::mutex> lock(sprite_mutex_);
    
    for (auto& pair : sprites_) {
        AnimatedSprite* sprite = pair.second;
        
        if (!sprite->playing || sprite->currentAnimationId == 0)
            continue;
        
        // Get current animation
        auto it = sprite->animations.find(sprite->currentAnimationId);
        if (it == sprite->animations.end())
            continue;
        
        SpriteAnimation* anim = it->second;
        
        float frameDuration = 1.0f / anim->fps;
        sprite->frameTimer += deltaTime;
        
        // Advance frames
        while (sprite->frameTimer >= frameDuration) {
            sprite->frameTimer -= frameDuration;
            
            // Find current frame index
            uint32_t currentIdx = 0;
            for (uint32_t i = 0; i < anim->frameCount; i++) {
                if (anim->frames[i] == sprite->currentFrame) {
                    currentIdx = i;
                    break;
                }
            }
            
            // Advance
            currentIdx++;
            
            // Check end
            if (currentIdx >= anim->frameCount) {
                if (anim->loop) {
                    currentIdx = 0;
                } else {
                    currentIdx = anim->frameCount - 1;
                    sprite->playing = 0;
                    break;
                }
            }
            
            sprite->currentFrame = anim->frames[currentIdx];
        }
    }
}

uint32_t Renderer::CreateAnimator(const std::vector<std::string>& animNames,
                                  const std::vector<std::vector<std::string>>& framePaths,
                                  const std::vector<float>& fpsList,
                                  const std::vector<bool>& loopList)
{
    Animator* animator = new Animator();
    
    uint32_t nextAnimId = 1;
    for (size_t i = 0; i < animNames.size(); i++) {
        MultiAtlasAnimation* anim = new MultiAtlasAnimation();
        anim->name = animNames[i];
        anim->id = nextAnimId++;
        anim->fps = fpsList[i];
        anim->loop = loopList[i];
        
        // Load each frame's atlas
        for (const std::string& path : framePaths[i]) {
            uint32_t atlasId = LoadAtlas(path);
            
            if (atlasId == 0) {
                Debugger::Instance().LogError("Failed to load animator frame: " + path);
                continue;
            }
            
            SpriteAtlas* atlas = GetAtlas(atlasId);
            AnimatorFrame frame;
            frame.atlasId = atlasId;
            frame.width = atlas->width;
            frame.height = atlas->height;
            
            anim->frames.push_back(frame);
        }
        
        animator->animations[anim->id] = anim;
        animator->animationNames[anim->name] = anim->id;
        
        Debugger::Instance().LogInfo("Loaded animation '" + anim->name + 
                                     "' with " + std::to_string(anim->frames.size()) + " frames");
    }
    
    std::lock_guard<std::mutex> lock(animator_mutex_);
    uint32_t id = next_animator_id_++;
    animators_[id] = animator;
    
    return id;
}

void Renderer::UpdateAnimator(uint32_t animatorId, float x, float y, float rotation,
                              float scaleX, float scaleY, bool flipH, bool flipV)
{
    std::lock_guard<std::mutex> lock(animator_mutex_);
    auto it = animators_.find(animatorId);
    if (it == animators_.end())
        return;
    
    Animator* anim = it->second;
    anim->x = x;
    anim->y = y;
    anim->rotation = rotation;
    anim->scaleX = scaleX;
    anim->scaleY = scaleY;
    anim->flipH = flipH ? 1 : 0;
    anim->flipV = flipV ? 1 : 0;
}

void Renderer::PlayAnimatorAnimation(uint32_t animatorId, const std::string& animName)
{
    std::lock_guard<std::mutex> lock(animator_mutex_);
    auto it = animators_.find(animatorId);
    if (it == animators_.end())
        return;
    
    Animator* animator = it->second;
    auto nameIt = animator->animationNames.find(animName);
    if (nameIt == animator->animationNames.end())
        return;
    
    animator->currentAnimationId = nameIt->second;
    animator->currentFrameIndex = 0;
    animator->frameTimer = 0.0f;
    animator->playing = 1;
}

void Renderer::DrawAnimator(uint32_t animatorId, size_t bufRefId)
{
    std::lock_guard<std::mutex> lock(animator_mutex_);
    auto it = animators_.find(animatorId);
    if (it == animators_.end())
        return;
    
    Animator* animator = it->second;
    
    if (animator->currentAnimationId == 0)
        return;
    
    auto animIt = animator->animations.find(animator->currentAnimationId);
    if (animIt == animator->animations.end())
        return;
    
    MultiAtlasAnimation* anim = animIt->second;
    
    if (animator->currentFrameIndex >= anim->frames.size())
        return;
    
    // Get current frame
    AnimatorFrame& frame = anim->frames[animator->currentFrameIndex];
    SpriteAtlas* atlas = GetAtlas(frame.atlasId);
    if (!atlas)
        return;
    
    // Get camera
    CameraState cam = GetCameraState(bufRefId);
    
    // Calculate world bounds
    float worldW = frame.width * animator->scaleX;
    float worldH = frame.height * animator->scaleY;
    
    // Frustum cull
    if (!IsInFrustum(cam, animator->x, animator->y, worldW, worldH))
        return;
    
    // Convert to screen
    ScreenRect screenRect = WorldToScreen(cam, animator->x, animator->y, worldW, worldH);
    
    // Get write buffer
    std::lock_guard<std::mutex> bufLock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return;
    
    SharedBufferRefs* s = shared_buffers_ref[bufRefId];
    if (!s || !s->control)
        return;
    
    std::atomic<uint32_t>* ctrl = reinterpret_cast<std::atomic<uint32_t>*>(s->control);
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    uint8_t* dstBuffer = s->pixel_buffers[js_write];
    
    // Source rect is entire atlas (loose sprite)
    FrameRect srcRect = { 0, 0, frame.width, frame.height };
    
    // Blit (assume non-opaque for loose sprites)
    BlitSpriteNN_Alpha(dstBuffer, s->width, s->height, screenRect,
                      atlas, srcRect, animator->flipH, animator->flipV);
    
    // Mark dirty
    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);
    if (dirty_count < MAX_DIRTY_REGIONS) {
        uint32_t offset = CTRL_DIRTY_REGIONS + (dirty_count * 4);
        ctrl[offset + 0] = screenRect.x;
        ctrl[offset + 1] = screenRect.y;
        ctrl[offset + 2] = screenRect.width;
        ctrl[offset + 3] = screenRect.height;
        ctrl[CTRL_DIRTY_COUNT].store(dirty_count + 1, std::memory_order_release);
    }
}

void Renderer::UpdateAnimators(float deltaTime)
{
    std::lock_guard<std::mutex> lock(animator_mutex_);
    
    for (auto& pair : animators_) {
        Animator* animator = pair.second;
        
        if (!animator->playing || animator->currentAnimationId == 0)
            continue;
        
        auto it = animator->animations.find(animator->currentAnimationId);
        if (it == animator->animations.end())
            continue;
        
        MultiAtlasAnimation* anim = it->second;
        
        float frameDuration = 1.0f / anim->fps;
        animator->frameTimer += deltaTime;
        
        while (animator->frameTimer >= frameDuration) {
            animator->frameTimer -= frameDuration;
            animator->currentFrameIndex++;
            
            if (animator->currentFrameIndex >= anim->frames.size()) {
                if (anim->loop) {
                    animator->currentFrameIndex = 0;
                } else {
                    animator->currentFrameIndex = anim->frames.size() - 1;
                    animator->playing = 0;
                    break;
                }
            }
        }
    }
}

void Renderer::DestroyAnimator(uint32_t animatorId)
{
    std::lock_guard<std::mutex> lock(animator_mutex_);
    auto it = animators_.find(animatorId);
    if (it != animators_.end()) {
        delete it->second;
        animators_.erase(it);
    }
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

CameraState Renderer::GetCameraState(size_t bufRefId)
{
    CameraState cam = {0};

    std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return cam;

    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s || !s->control)
        return cam;

    // Reinterpret control buffer as float*
    float *ctrl_f32 = reinterpret_cast<float *>(s->control);

    cam.worldX = ctrl_f32[CTRL_CAM_WORLD_X];
    cam.worldY = ctrl_f32[CTRL_CAM_WORLD_Y];
    cam.zoom = ctrl_f32[CTRL_CAM_ZOOM];
    cam.viewWidth = ctrl_f32[CTRL_CAM_VIEW_WIDTH];
    cam.viewHeight = ctrl_f32[CTRL_CAM_VIEW_HEIGHT];
    cam.rotation = ctrl_f32[CTRL_CAM_ROTATION];
    cam.frustumLeft = ctrl_f32[CTRL_CAM_FRUSTUM_LEFT];
    cam.frustumRight = ctrl_f32[CTRL_CAM_FRUSTUM_RIGHT];
    cam.frustumTop = ctrl_f32[CTRL_CAM_FRUSTUM_TOP];
    cam.frustumBottom = ctrl_f32[CTRL_CAM_FRUSTUM_BOTTOM];

    return cam;
}

bool Renderer::IsInFrustum(const CameraState &cam, float worldX, float worldY, float width, float height)
{
    float left = worldX - width / 2.0f;
    float right = worldX + width / 2.0f;
    float top = worldY - height / 2.0f;
    float bottom = worldY + height / 2.0f;

    // AABB intersection test
    if (right < cam.frustumLeft || left > cam.frustumRight ||
        bottom < cam.frustumTop || top > cam.frustumBottom)
    {
        return false; // Outside
    }

    return true; // at least partially visible
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

void Renderer::ProcessPendingRegions(size_t bufRefId)
{
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return;

    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s || !s->control)
        return;

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);

    // Get the CURRENT write buffer that JS is using
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);

    if (dirty_count == 0)
        return; // Nothing to do

    uint8_t *pixel_data = s->pixel_buffers[js_write];

    auto it = textures_.find(s->texture_id);
    if (it == textures_.end())
        return;

    Texture2D &texture = it->second;

    // Process all dirty regions
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

    // Clear dirty count - JS can start fresh
    ctrl[CTRL_DIRTY_COUNT].store(0u, std::memory_order_release);
}

void Renderer::PartialTextureUpdate(size_t bufRefId, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return;

    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s)
        return;

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    uint8_t *pixel_data = s->pixel_buffers[js_write];

    UploadRegionToGPU(s->texture_id, pixel_data, x, y, w, h, s->width, s->height);
}

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