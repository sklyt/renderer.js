#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "renderer.h"
#include <iostream>
#include <filesystem>
#include "input_manager.h"
#include "audio_manager.h"
#include <debugger.h>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstring>
#include <iterator>



ScreenRect WorldToScreenWithCameraTransform(const CameraState &cam, float worldX,
                                            float worldY, float worldW,
                                            float worldH, float cosA, float sinA)
{
    // Translate to camera space
    float dx = worldX - cam.worldX;
    float dy = worldY - cam.worldY;

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

ScreenRect WorldToScreen(const CameraState &cam, float worldX, float worldY,
                         float worldW, float worldH)
{
    return WorldToScreenWithCameraTransform(
        cam, worldX, worldY, worldW, worldH,
        cosf(-cam.rotation), sinf(-cam.rotation));
}


struct NearestNeighborAxis
{
    uint64_t source;
    uint64_t remainder;
    uint64_t denominator;
    uint64_t step;
    uint64_t stepRemainder;

    void Advance()
    {
        source += step;
        remainder += stepRemainder;
        if (remainder >= denominator)
        {
            remainder -= denominator;
            ++source;
        }
    }
};

static NearestNeighborAxis MakeNearestNeighborAxis(uint32_t sourceExtent,
                                                   uint32_t destinationExtent,
                                                   uint32_t localStart)
{
    const uint64_t denominator = static_cast<uint64_t>(destinationExtent) * 2;
    const uint64_t numerator =
        (static_cast<uint64_t>(localStart) * 2 + 1) * sourceExtent;
    const uint64_t stepNumerator = static_cast<uint64_t>(sourceExtent) * 2;
    return {
        numerator / denominator,
        numerator % denominator,
        denominator,
        stepNumerator / denominator,
        stepNumerator % denominator};
}


// Fast path: opaque, no rotation, nearest neighbor
void BlitSpriteNN_Opaque(uint8_t *dstBuffer, uint32_t dstWidth, uint32_t dstHeight,
                         const ScreenRect &dstRect,
                         const SpriteAtlas *atlas, const FrameRect &srcRect,
                         bool flipH, bool flipV)
{
    if (dstRect.width == 0 || dstRect.height == 0 ||
        srcRect.w == 0 || srcRect.h == 0)
        return;

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
    if (dstX < 0)
    {
        clipLeft = -dstX;
        dstW += dstX;
        dstX = 0;
    }
    if (dstY < 0)
    {
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

    const uint8_t *srcData = atlas->data;
    const uint32_t srcWidth = atlas->width;
    if (srcRect.w == dstRect.width && srcRect.h == dstRect.height &&
        !flipH && !flipV)
    {
        for (int32_t row = 0; row < dstH; ++row)
        {
            const uint32_t srcY = srcRect.y + clipTop + row;
            const uint32_t screenY = dstY + row;
            const size_t srcOffset =
                (static_cast<size_t>(srcY) * srcWidth + srcRect.x + clipLeft) * 4;
            const size_t dstOffset =
                (static_cast<size_t>(screenY) * dstWidth + dstX) * 4;
            std::memcpy(dstBuffer + dstOffset, srcData + srcOffset,
                        static_cast<size_t>(dstW) * 4);
        }
        return;
    }

    NearestNeighborAxis sourceY =
        MakeNearestNeighborAxis(srcRect.h, dstRect.height, clipTop);

    // Row-by-row blit
    for (int32_t row = 0; row < dstH; row++)
    {
        int32_t screenY = dstY + row;
        uint32_t srcY = static_cast<uint32_t>(sourceY.source);
        if (flipV)
            srcY = (srcRect.h - 1) - srcY;
        srcY += srcRect.y;

        const size_t dstRowBase = static_cast<size_t>(screenY) * dstWidth * 4;
        const size_t srcRowBase = static_cast<size_t>(srcY) * srcWidth * 4;
        NearestNeighborAxis sourceX =
            MakeNearestNeighborAxis(srcRect.w, dstRect.width, clipLeft);

        for (int32_t col = 0; col < dstW; col++)
        {
            int32_t screenX = dstX + col;
            uint32_t srcX = static_cast<uint32_t>(sourceX.source);
            if (flipH)
                srcX = (srcRect.w - 1) - srcX;
            srcX += srcRect.x;

            const size_t dstIdx = dstRowBase + static_cast<size_t>(screenX) * 4;
            const size_t srcIdx = srcRowBase + static_cast<size_t>(srcX) * 4;

            // Direct copy (opaque, no blend)
            dstBuffer[dstIdx + 0] = srcData[srcIdx + 0]; // R
            dstBuffer[dstIdx + 1] = srcData[srcIdx + 1]; // G
            dstBuffer[dstIdx + 2] = srcData[srcIdx + 2]; // B
            dstBuffer[dstIdx + 3] = 255;                 // A
            sourceX.Advance();
        }
        sourceY.Advance();
    }
}

// With alpha blending
void BlitSpriteNN_Alpha(uint8_t *dstBuffer, uint32_t dstWidth, uint32_t dstHeight,
                        const ScreenRect &dstRect,
                        const SpriteAtlas *atlas, const FrameRect &srcRect,
                        bool flipH, bool flipV)
{
    if (dstRect.width == 0 || dstRect.height == 0 ||
        srcRect.w == 0 || srcRect.h == 0)
        return;

    int32_t dstX = dstRect.x;
    int32_t dstY = dstRect.y;
    int32_t dstW = dstRect.width;
    int32_t dstH = dstRect.height;

    if (dstX >= static_cast<int32_t>(dstWidth) || dstY >= static_cast<int32_t>(dstHeight))
        return;
    if (dstX + dstW <= 0 || dstY + dstH <= 0)
        return;

    int32_t clipLeft = 0, clipTop = 0;
    if (dstX < 0)
    {
        clipLeft = -dstX;
        dstW += dstX;
        dstX = 0;
    }
    if (dstY < 0)
    {
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

    const uint8_t *srcData = atlas->data;
    const uint32_t srcWidth = atlas->width;
    NearestNeighborAxis sourceY =
        MakeNearestNeighborAxis(srcRect.h, dstRect.height, clipTop);

    for (int32_t row = 0; row < dstH; row++)
    {
        int32_t screenY = dstY + row;
        uint32_t srcY = static_cast<uint32_t>(sourceY.source);
        if (flipV)
            srcY = (srcRect.h - 1) - srcY;
        srcY += srcRect.y;

        const size_t dstRowBase = static_cast<size_t>(screenY) * dstWidth * 4;
        const size_t srcRowBase = static_cast<size_t>(srcY) * srcWidth * 4;
        NearestNeighborAxis sourceX =
            MakeNearestNeighborAxis(srcRect.w, dstRect.width, clipLeft);

        for (int32_t col = 0; col < dstW; col++)
        {
            int32_t screenX = dstX + col;
            uint32_t srcX = static_cast<uint32_t>(sourceX.source);
            if (flipH)
                srcX = (srcRect.w - 1) - srcX;
            srcX += srcRect.x;

            const size_t dstIdx = dstRowBase + static_cast<size_t>(screenX) * 4;
            const size_t srcIdx = srcRowBase + static_cast<size_t>(srcX) * 4;
            sourceX.Advance();

            uint8_t sR = srcData[srcIdx + 0];
            uint8_t sG = srcData[srcIdx + 1];
            uint8_t sB = srcData[srcIdx + 2];
            uint8_t sA = srcData[srcIdx + 3];

            if (sA == 255)
            {
                // Fully opaque pixel - direct copy
                dstBuffer[dstIdx + 0] = sR;
                dstBuffer[dstIdx + 1] = sG;
                dstBuffer[dstIdx + 2] = sB;
                dstBuffer[dstIdx + 3] = 255;
            }
            else if (sA > 0)
            {
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
        sourceY.Advance();
    }
}


void BlitSpriteRotated(uint8_t *dstBuffer, uint32_t dstWidth, uint32_t dstHeight,
                       float centerX, float centerY, float width, float height,
                       float rotation, const SpriteAtlas *atlas, const FrameRect &srcRect,
                       bool flipH, bool flipV, bool opaque)
{
    const float cosRotation = std::cos(rotation);
    const float sinRotation = std::sin(rotation);
    const float halfWidth = width / 2.0f;
    const float halfHeight = height / 2.0f;
    const float extentX = std::abs(halfWidth * cosRotation) +
                          std::abs(halfHeight * sinRotation);
    const float extentY = std::abs(halfWidth * sinRotation) +
                          std::abs(halfHeight * cosRotation);
    const int32_t left = std::max(0, static_cast<int32_t>(std::floor(centerX - extentX)));
    const int32_t right = std::min(static_cast<int32_t>(dstWidth),
                                   static_cast<int32_t>(std::ceil(centerX + extentX)));
    const int32_t top = std::max(0, static_cast<int32_t>(std::floor(centerY - extentY)));
    const int32_t bottom = std::min(static_cast<int32_t>(dstHeight),
                                    static_cast<int32_t>(std::ceil(centerY + extentY)));

    for (int32_t y = top; y < bottom; ++y)
    {
        for (int32_t x = left; x < right; ++x)
        {
            const float dx = x + 0.5f - centerX;
            const float dy = y + 0.5f - centerY;
            const float localX = dx * cosRotation + dy * sinRotation;
            const float localY = -dx * sinRotation + dy * cosRotation;
            if (localX < -halfWidth || localX >= halfWidth ||
                localY < -halfHeight || localY >= halfHeight)
                continue;

            uint32_t sourceX = static_cast<uint32_t>(
                (localX + halfWidth) * srcRect.w / width);
            uint32_t sourceY = static_cast<uint32_t>(
                (localY + halfHeight) * srcRect.h / height);
            sourceX = std::min(sourceX, srcRect.w - 1);
            sourceY = std::min(sourceY, srcRect.h - 1);
            if (flipH)
                sourceX = srcRect.w - 1 - sourceX;
            if (flipV)
                sourceY = srcRect.h - 1 - sourceY;

            const uint32_t sourceIndex =
                ((srcRect.y + sourceY) * atlas->width + srcRect.x + sourceX) * 4;
            const uint32_t destinationIndex = (y * dstWidth + x) * 4;
            const uint8_t alpha = opaque ? 255 : atlas->data[sourceIndex + 3];
            if (alpha == 0)
                continue;

            if (alpha == 255)
            {
                dstBuffer[destinationIndex] = atlas->data[sourceIndex];
                dstBuffer[destinationIndex + 1] = atlas->data[sourceIndex + 1];
                dstBuffer[destinationIndex + 2] = atlas->data[sourceIndex + 2];
                dstBuffer[destinationIndex + 3] = 255;
                continue;
            }

            const uint32_t inverseAlpha = 255 - alpha;
            for (uint32_t channel = 0; channel < 3; ++channel)
            {
                dstBuffer[destinationIndex + channel] = static_cast<uint8_t>(
                    (atlas->data[sourceIndex + channel] * alpha +
                     dstBuffer[destinationIndex + channel] * inverseAlpha + 127) /
                    255);
            }
            dstBuffer[destinationIndex + 3] = static_cast<uint8_t>(
                (alpha * 255 + dstBuffer[destinationIndex + 3] * inverseAlpha + 127) /
                255);
        }
    }
}

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

    {
        std::lock_guard<std::mutex> lock(tiled_mutex_);
        for (auto &pair : tiledMaps_)
        {
            delete pair.second;
        }
        tiledMaps_.clear();
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

// tiled

uint32_t Renderer::LoadTiledMap(const std::string &path)
{
    TiledMap *map = ParseTiledMap(path);

    if (!map)
    {
        Debugger::Instance().LogError("Failed to parse Tiled map: " + path);
        return 0;
    }

    size_t s = map->tilesets.size();
    Debugger::Instance().LogInfo("path: " + map->basePath + " size " + std::to_string(s));

    for (const auto &tileset : map->tilesets)
    {
        // Skip tilesets with no image to load - either an empty tileset
        // (tileCount == 0, e.g. a stray/duplicate entry) or a collection
        // tileset whose fallback tile (local id 0) had no image.
        if (tileset->imagePath.empty())
        {
            Debugger::Instance().LogInfo("Skipping tileset '" + tileset->name +
                                         "' (firstgid " + std::to_string(tileset->first_gid) +
                                         "): no image path");
            continue;
        }

        std::string atlasPath = map->basePath + tileset->imagePath;
        Debugger::Instance().LogInfo("Loading tileset '" + tileset->name +
                                     "' path " + map->basePath + " " + tileset->imagePath);

        uint32_t atlasId = LoadAtlas(atlasPath);
        if (atlasId == 0)
        {
            Debugger::Instance().LogError("Failed to load tileset atlas: " + atlasPath);
            delete map;
            return 0;
        }

        tileset->atlasId = atlasId;
        Debugger::Instance().LogInfo("Loaded tileset '" + tileset->name +
                                     "' as atlas " + std::to_string(atlasId));
    }

    InitializeTileRenderData(map);

       for (auto *layer : map->layers)
       {
           if (layer->type == TiledLayerType::OBJECT && layer->objectLayer)
           {
               InitializeObjectLayer(layer->objectLayer, map);
           }

           if (layer->type == TiledLayerType::COLLISION &&
               !layer->CollisionShapes.empty())
           {
               const size_t beforeCount = layer->CollisionShapes.size();
               MergeCollisionBoxes(layer->CollisionShapes);
               Debugger::Instance().LogInfo(
                   "Reduced collision shapes in layer '" + layer->name + "' from " +
                   std::to_string(beforeCount) + " to " +
                   std::to_string(layer->CollisionShapes.size()));
           }
       }

    std::lock_guard<std::mutex> lock(tiled_mutex_);
    uint32_t id = next_tiled_map_id_++;
    tiledMaps_[id] = map;

    Debugger::Instance().LogInfo("Loaded Tiled map " + std::to_string(id) +
                                 " from " + path);

    return id;
}

TiledMap *Renderer::GetTiledMap(uint32_t mapId)
{
    std::lock_guard<std::mutex> lock(tiled_mutex_);
    auto it = tiledMaps_.find(mapId);
    if (it == tiledMaps_.end())
        return nullptr;
    return it->second;
}

void Renderer::MergeCollisionBoxes(std::vector<CollisionShape> &shapes)
{
    std::vector<CollisionShape> boxes;
    std::vector<CollisionShape> nonBoxes;
    boxes.reserve(shapes.size());
    nonBoxes.reserve(shapes.size());

    for (const CollisionShape &shape : shapes)
    {
        if (shape.type == CollisionShapeType::BOX)
            boxes.push_back(shape);
        else
            nonBoxes.push_back(shape);
    }

    if (boxes.empty())
        return;

    std::sort(boxes.begin(), boxes.end(),
              [](const CollisionShape &a, const CollisionShape &b)
              {
                  if (a.y != b.y)
                      return a.y < b.y;
                  return a.x < b.x;
              });

    bool merged = true;
    while (merged)
    {
        merged = false;
        for (size_t i = 0; i < boxes.size() && !merged; ++i)
        {
            for (size_t j = i + 1; j < boxes.size(); ++j)
            {
                const CollisionShape &a = boxes[i];
                const CollisionShape &b = boxes[j];
                CollisionShape mergedBox = a;
                bool canMerge = false;

                if (a.y == b.y && a.height == b.height)
                {
                    if (a.x + a.width == b.x)
                    {
                        mergedBox.width = a.width + b.width;
                        mergedBox.name = a.name + "_" + b.name;
                        canMerge = true;
                    }
                    else if (b.x + b.width == a.x)
                    {
                        mergedBox.x = b.x;
                        mergedBox.width = a.width + b.width;
                        mergedBox.name = b.name + "_" + a.name;
                        canMerge = true;
                    }
                }

                if (!canMerge && a.x == b.x && a.width == b.width)
                {
                    if (a.y + a.height == b.y)
                    {
                        mergedBox.height = a.height + b.height;
                        mergedBox.name = a.name + "_" + b.name;
                        canMerge = true;
                    }
                    else if (b.y + b.height == a.y)
                    {
                        mergedBox.y = b.y;
                        mergedBox.height = a.height + b.height;
                        mergedBox.name = b.name + "_" + a.name;
                        canMerge = true;
                    }
                }

                if (canMerge)
                {
                    mergedBox.type = CollisionShapeType::BOX;
                    boxes[i] = std::move(mergedBox);
                    boxes.erase(boxes.begin() + j);
                    merged = true;
                    break;
                }
            }
        }
    }

    shapes.clear();
    shapes.reserve(boxes.size() + nonBoxes.size());
    shapes.insert(shapes.end(), boxes.begin(), boxes.end());
    shapes.insert(shapes.end(), nonBoxes.begin(), nonBoxes.end());

    Debugger::Instance().LogInfo(
        "Merged collision boxes: " + std::to_string(boxes.size()) +
        " boxes, " + std::to_string(nonBoxes.size()) + " other shapes");
}

std::vector<float> Renderer::GetTiledCollisions(uint32_t mapId)
{
    std::vector<float> data;
    TiledMap *map = GetTiledMap(mapId);
    if (!map)
        return data;

    for (const TiledLayer *layer : map->layers)
    {
        if (layer->type != TiledLayerType::COLLISION)
            continue;

        for (const CollisionShape &shape : layer->CollisionShapes)
        {
            data.push_back(static_cast<float>(shape.type));
            data.push_back(shape.x);
            data.push_back(shape.y);

            switch (shape.type)
            {
            case CollisionShapeType::BOX:
                data.push_back(shape.width);
                data.push_back(shape.height);
                break;

            case CollisionShapeType::CIRCLE:
                data.push_back(shape.radius);
                break;

            case CollisionShapeType::POLYGON:
                data.push_back(static_cast<float>(shape.points.size() / 2));
                data.insert(data.end(), shape.points.begin(), shape.points.end());
                break;

            case CollisionShapeType::POINT:
                break;
            }
        }
    }

    Debugger::Instance().LogInfo(
        "Exported " + std::to_string(data.size()) +
        " collision floats for map " + std::to_string(mapId));
    return data;
}

void Renderer::DestroyTiledMap(uint32_t mapId)
{
    std::lock_guard<std::mutex> lock(tiled_mutex_);
    auto it = tiledMaps_.find(mapId);
    if (it != tiledMaps_.end())
    {
        for (TiledLayer *layer : it->second->layers)
        {
            if (layer->type != TiledLayerType::OBJECT || !layer->objectLayer)
                continue;

            for (const TiledObjectInstance &instance : layer->objectLayer->spriteInstances)
                DestroySprite(instance.spriteId);
        }

        delete it->second;
        tiledMaps_.erase(it);
    }
}

TilesetLookup Renderer::FindTileset(TiledMap *map, uint32_t tileId)
{
    TilesetLookup result = {nullptr, 0};

    if (tileId == 0)
        return result; // Tile ID 0 = empty

    const auto tileset = std::upper_bound(
        map->tilesets.begin(), map->tilesets.end(), tileId,
        [](uint32_t gid, const TiledTileset *candidate)
        {
            return gid < candidate->first_gid;
        });
    if (tileset == map->tilesets.begin())
        return result;

    result.tileset = *std::prev(tileset);
    result.localid = tileId - result.tileset->first_gid;

    return result;
}

void Renderer::InitializeTileRenderData(TiledMap *map)
{
    for (TiledTileset *tileset : map->tilesets)
    {
        SpriteAtlas *atlas = tileset->atlasId == 0 ? nullptr : GetAtlas(tileset->atlasId);
        if (!atlas || tileset->tileWidth == 0 || tileset->tileHeight == 0)
            continue;

        if (tileset->columns == 0)
            tileset->columns = atlas->width / tileset->tileWidth;
        if (tileset->columns == 0)
            continue;

        tileset->frames.resize(tileset->tileCount);
        for (uint32_t localId = 0; localId < tileset->tileCount; ++localId)
        {
            TiledTileFrame &frame = tileset->frames[localId];
            const uint32_t column = localId % tileset->columns;
            const uint32_t row = localId / tileset->columns;
            frame.x = column * tileset->tileWidth;
            frame.y = row * tileset->tileHeight;
            frame.width = tileset->tileWidth;
            frame.height = tileset->tileHeight;

            if (static_cast<uint64_t>(frame.x) + frame.width > atlas->width ||
                static_cast<uint64_t>(frame.y) + frame.height > atlas->height)
                continue;

            frame.opaque = true;
            for (uint32_t y = 0; y < frame.height && frame.opaque; ++y)
            {
                const uint8_t *pixel = atlas->data +
                    ((frame.y + y) * atlas->width + frame.x) * 4 + 3;
                for (uint32_t x = 0; x < frame.width; ++x, pixel += 4)
                {
                    if (*pixel != 255)
                    {
                        frame.opaque = false;
                        break;
                    }
                }
            }
        }
    }

    for (TiledLayer *layer : map->layers)
    {
        if (layer->type != TiledLayerType::TILE || !layer->tileLayer)
            continue;

        TiledTileLayer *tileLayer = layer->tileLayer;
        const size_t cellCount =
            static_cast<size_t>(tileLayer->width) * tileLayer->height;
        tileLayer->renderInfo.resize(std::min(tileLayer->dataCount, cellCount));

        for (size_t index = 0; index < tileLayer->renderInfo.size(); ++index)
        {
            const uint32_t encodedGid = tileLayer->data[index];
            const uint32_t tileId = encodedGid & 0x0FFFFFFFu;
            if (tileId == 0)
                continue;

            const TilesetLookup lookup = FindTileset(map, tileId);
            if (!lookup.tileset || lookup.tileset->atlasId == 0 ||
                lookup.localid >= lookup.tileset->frames.size())
                continue;

            const TiledTileFrame &frame = lookup.tileset->frames[lookup.localid];
            SpriteAtlas *atlas = GetAtlas(lookup.tileset->atlasId);
            if (!atlas || frame.width == 0 || frame.height == 0 ||
                static_cast<uint64_t>(frame.x) + frame.width > atlas->width ||
                static_cast<uint64_t>(frame.y) + frame.height > atlas->height)
                continue;

            TiledTileRenderInfo &info = tileLayer->renderInfo[index];
            info.atlasId = lookup.tileset->atlasId;
            info.frame = frame;
            info.flipH = (encodedGid & 0x80000000u) != 0;
            info.flipV = (encodedGid & 0x40000000u) != 0;
        }
    }
}


// // renderer.cpp - alternative implementation

// void Renderer::RenderTileLayer(TiledTileLayer* layer, TiledMap* map, 
//                                size_t bufRefId, const CameraState& cam)
// {
//     // ... same setup as before ...
    
//     const uint32_t CHUNK_SIZE = 16;  // 16x16 tiles per chunk
    
//     // Calculate visible chunk range
//     int32_t startChunkX = startTileX / CHUNK_SIZE;
//     int32_t endChunkX = (endTileX + CHUNK_SIZE - 1) / CHUNK_SIZE;
//     int32_t startChunkY = startTileY / CHUNK_SIZE;
//     int32_t endChunkY = (endTileY + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
//     // Render visible chunks
//     for (int32_t cy = startChunkY; cy < endChunkY; cy++) {
//         for (int32_t cx = startChunkX; cx < endChunkX; cx++) {
//             // Calculate tile range for this chunk
//             int32_t chunkStartX = cx * CHUNK_SIZE;
//             int32_t chunkEndX = std::min(chunkStartX + CHUNK_SIZE, 
//                                          static_cast<int32_t>(layer->width));
//             int32_t chunkStartY = cy * CHUNK_SIZE;
//             int32_t chunkEndY = std::min(chunkStartY + CHUNK_SIZE, 
//                                          static_cast<int32_t>(layer->height));
            
//             // Clamp to visible range
//             if (chunkStartX < startTileX) chunkStartX = startTileX;
//             if (chunkEndX > endTileX) chunkEndX = endTileX;
//             if (chunkStartY < startTileY) chunkStartY = startTileY;
//             if (chunkEndY > endTileY) chunkEndY = endTileY;
            
//             // Render tiles in this chunk
//             for (int32_t ty = chunkStartY; ty < chunkEndY; ty++) {
//                 for (int32_t tx = chunkStartX; tx < chunkEndX; tx++) {
//                     // ... same tile rendering as before ...
//                 }
//             }
//         }
//     }
// }

TileRenderMetrics Renderer::RenderTileLayer(TiledTileLayer *layer, TiledMap *map,
                                            size_t bufRefId, const CameraState &cam,
                                            float inverseCameraCos,
                                            float inverseCameraSin,
                                            bool chunked, bool measure)
{
    TileRenderMetrics metrics;
    if (!layer || !map || !layer->data || map->tileWidth == 0 ||
        map->tileHeight == 0 || layer->width == 0 || layer->height == 0)
        return metrics;

    const auto layerStart = measure ? std::chrono::steady_clock::now()
                                    : std::chrono::steady_clock::time_point{};
    const auto elapsedNanoseconds = [](const auto &start)
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - start)
                .count());
    };

    // Get current write buffer
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
        return metrics;

    SharedBufferRefs *s = shared_buffers_ref[bufRefId]; 
    if (!s || !s->control)
        return metrics;

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    if (js_write >= 3 || !s->pixel_buffers[js_write])
        return metrics;
    uint8_t *dstBuffer = s->pixel_buffers[js_write];
    uint32_t cachedAtlasId = 0;
    SpriteAtlas *cachedAtlas = nullptr;

    // Layer offset in world space
    const auto rangeStart = measure ? std::chrono::steady_clock::now()
                                    : std::chrono::steady_clock::time_point{};
    float layerWorldX = static_cast<float>(layer->offsetX);
    float layerWorldY = static_cast<float>(layer->offsetY);

    // Calculate visible tile range
    float tileW = static_cast<float>(map->tileWidth);
    float tileH = static_cast<float>(map->tileHeight);

    // Camera frustum in world space
    float visibleLeft = cam.frustumLeft - layerWorldX;
    float visibleRight = cam.frustumRight - layerWorldX;
    float visibleTop = cam.frustumTop - layerWorldY;
    float visibleBottom = cam.frustumBottom - layerWorldY;

    // Convert to tile coordinates
    int32_t startTileX = static_cast<int32_t>(std::floor(visibleLeft / tileW));
    int32_t endTileX = static_cast<int32_t>(std::ceil(visibleRight / tileW));
    int32_t startTileY = static_cast<int32_t>(std::floor(visibleTop / tileH));
    int32_t endTileY = static_cast<int32_t>(std::ceil(visibleBottom / tileH));

    // Clamp to layer bounds
    if (startTileX < 0)
        startTileX = 0;
    if (startTileY < 0)
        startTileY = 0;
    if (endTileX > static_cast<int32_t>(layer->width))
        endTileX = layer->width;
    if (endTileY > static_cast<int32_t>(layer->height))
        endTileY = layer->height;

    if (measure)
        metrics.rangeNanoseconds = elapsedNanoseconds(rangeStart);

    const auto renderTile = [&](uint32_t tx, uint32_t ty)
    {
        ++metrics.scannedCells;
        const size_t tileIndex = static_cast<size_t>(ty) * layer->width + tx;
        if (tileIndex >= layer->renderInfo.size())
            return;

        ++metrics.nonEmptyTiles;
        const auto tilesetLookupStart = measure ? std::chrono::steady_clock::now()
                                                : std::chrono::steady_clock::time_point{};
        const TiledTileRenderInfo &info = layer->renderInfo[tileIndex];
        if (measure)
            metrics.tilesetLookupNanoseconds += elapsedNanoseconds(tilesetLookupStart);
        if (info.atlasId == 0)
            return;

        if (info.atlasId != cachedAtlasId)
        {
            const auto atlasLookupStart = measure ? std::chrono::steady_clock::now()
                                                 : std::chrono::steady_clock::time_point{};
            cachedAtlasId = info.atlasId;
            cachedAtlas = GetAtlas(info.atlasId);
            if (measure)
                metrics.atlasLookupNanoseconds += elapsedNanoseconds(atlasLookupStart);
        }
        if (!cachedAtlas)
            return;

        const FrameRect srcRect = {
            info.frame.x, info.frame.y, info.frame.width, info.frame.height};
        const float worldX = layerWorldX + tx * tileW;
        const float worldY = layerWorldY + ty * tileH;
        const ScreenRect screenRect = WorldToScreenWithCameraTransform(
            cam, worldX, worldY, tileW, tileH,
            inverseCameraCos, inverseCameraSin);

        const auto blitStart = measure ? std::chrono::steady_clock::now()
                                       : std::chrono::steady_clock::time_point{};
        if (info.frame.opaque)
        {
            BlitSpriteNN_Opaque(dstBuffer, s->width, s->height, screenRect,
                                cachedAtlas, srcRect, info.flipH, info.flipV);
        }
        else
        {
            BlitSpriteNN_Alpha(dstBuffer, s->width, s->height, screenRect,
                               cachedAtlas, srcRect, info.flipH, info.flipV);
        }
        if (measure)
            metrics.blitNanoseconds += elapsedNanoseconds(blitStart);
        ++metrics.renderedTiles;
    };
    const auto renderRowRange = [&](uint32_t y, int32_t startX, int32_t endX)
    {
        if (startX >= endX || y >= layer->nonEmptyTilesByRow.size())
            return;

        const std::vector<uint32_t> &nonEmptyX = layer->nonEmptyTilesByRow[y];
        auto tile = std::lower_bound(
            nonEmptyX.begin(), nonEmptyX.end(), static_cast<uint32_t>(startX));
        for (; tile != nonEmptyX.end() && *tile < static_cast<uint32_t>(endX); ++tile)
            renderTile(*tile, y);
    };

    if (chunked)
    {
        const int32_t startChunkX = startTileX / TILED_RENDER_CHUNK_SIZE;
        const int32_t endChunkX =
            (endTileX + TILED_RENDER_CHUNK_SIZE - 1) / TILED_RENDER_CHUNK_SIZE;
        const int32_t startChunkY = startTileY / TILED_RENDER_CHUNK_SIZE;
        const int32_t endChunkY =
            (endTileY + TILED_RENDER_CHUNK_SIZE - 1) / TILED_RENDER_CHUNK_SIZE;

        for (int32_t cy = startChunkY; cy < endChunkY; ++cy)
        {
            for (int32_t cx = startChunkX; cx < endChunkX; ++cx)
            {
                const size_t chunkIndex =
                    static_cast<size_t>(cy) * layer->chunkColumns + cx;
                if (chunkIndex >= layer->nonEmptyChunks.size() ||
                    layer->nonEmptyChunks[chunkIndex] == 0)
                    continue;

                const int32_t chunkStartX = std::max(
                    startTileX, cx * static_cast<int32_t>(TILED_RENDER_CHUNK_SIZE));
                const int32_t chunkEndX = std::min(
                    endTileX, (cx + 1) * static_cast<int32_t>(TILED_RENDER_CHUNK_SIZE));
                const int32_t chunkStartY = std::max(
                    startTileY, cy * static_cast<int32_t>(TILED_RENDER_CHUNK_SIZE));
                const int32_t chunkEndY = std::min(
                    endTileY, (cy + 1) * static_cast<int32_t>(TILED_RENDER_CHUNK_SIZE));

                for (int32_t y = chunkStartY; y < chunkEndY; ++y)
                    renderRowRange(static_cast<uint32_t>(y), chunkStartX, chunkEndX);
            }
        }
    }
    else
    {
        for (int32_t y = startTileY; y < endTileY; ++y)
            renderRowRange(static_cast<uint32_t>(y), startTileX, endTileX);
    }

    const auto dirtyRegionStart = measure ? std::chrono::steady_clock::now()
                                          : std::chrono::steady_clock::time_point{};
    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);
    
    if (dirty_count < MAX_DIRTY_REGIONS)
    {
        ScreenRect visibleScreen = WorldToScreenWithCameraTransform(
            cam,
            layerWorldX + startTileX * tileW,
            layerWorldY + startTileY * tileH,
            (endTileX - startTileX) * tileW,
            (endTileY - startTileY) * tileH,
            inverseCameraCos, inverseCameraSin);
        uint32_t offset = CTRL_DIRTY_REGIONS + (dirty_count * 4);
        
        int32_t rx = visibleScreen.x < 0 ? 0 : visibleScreen.x;
        int32_t ry = visibleScreen.y < 0 ? 0 : visibleScreen.y;
        uint32_t rw = visibleScreen.width;
        uint32_t rh = visibleScreen.height;
        
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

    if (measure)
    {
        metrics.dirtyRegionNanoseconds = elapsedNanoseconds(dirtyRegionStart);
        metrics.tileLayerNanoseconds = elapsedNanoseconds(layerStart);
    }

    return metrics;
}

 

void Renderer::DrawTiledMap(uint32_t mapId, size_t bufRefId, bool chunked)
{
    const bool measureTiles = tileRenderInstrumentation_.IsEnabled();
    const auto mapDrawStart = measureTiles ? std::chrono::steady_clock::now()
                                           : std::chrono::steady_clock::time_point{};
    TiledMap *map = GetTiledMap(mapId);
    if (!map)
        return;

    ProcessPendingRegions(bufRefId);

    const CameraState cam = GetCameraState(bufRefId);
    const float inverseCameraCos = cosf(-cam.rotation);
    const float inverseCameraSin = sinf(-cam.rotation);
    TileRenderMetrics frameMetrics;
    for (TiledLayer *layer : map->layers)
    {
        if (!layer->visible)
            continue;

        switch (layer->type)
        {
            case TiledLayerType::TILE:
                frameMetrics += RenderTileLayer(
                    layer->tileLayer, map, bufRefId, cam,
                    inverseCameraCos, inverseCameraSin, chunked, measureTiles);
                break;

            case TiledLayerType::OBJECT:
                RenderObjectLayer(layer->objectLayer, bufRefId, cam);
                break;

            case TiledLayerType::COLLISION:
                break;
        }
    }

    if (measureTiles)
    {
        frameMetrics.mapDrawNanoseconds = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - mapDrawStart)
                .count());
        tileRenderInstrumentation_.RecordFrame(frameMetrics);
    }
}

void Renderer::InitializeTileInstrumentation(uint32_t reportIntervalFrames)
{
    tileRenderInstrumentation_.Initialize(reportIntervalFrames);
}

void Renderer::InitializeObjectLayer(TiledObjectLayer *objLayer, TiledMap *map)
{
    if (!objLayer || !map)
        return;

    objLayer->spriteInstances.reserve(objLayer->objects.size());
    for (const TiledObject &obj : objLayer->objects)
    {
        const uint32_t tileId = obj.gid & 0x0FFFFFFFu;
        if (tileId == 0)
            continue;

        TilesetLookup lookup = FindTileset(map, tileId);
        if (!lookup.tileset || lookup.tileset->atlasId == 0 ||
            lookup.tileset->tileWidth == 0 || lookup.tileset->tileHeight == 0 ||
            lookup.localid >= lookup.tileset->tileCount)
        {
            Debugger::Instance().LogWarn("Object references invalid gid: " +
                                         std::to_string(tileId));
            continue;
        }

        const uint32_t spriteId = CreateSprite(
            lookup.tileset->atlasId,
            lookup.tileset->tileWidth,
            lookup.tileset->tileHeight,
            lookup.tileset->tileCount,
            false);

        if (spriteId == 0) {
            Debugger::Instance().LogError("Failed to create sprite for object");
            continue;
        }

        AnimatedSprite *sprite = GetSprite(spriteId);
        if (sprite) {
            sprite->currentFrame = lookup.localid;
            if (lookup.tileset->columns != 0)
                sprite->framesPerRow = lookup.tileset->columns;
            if (lookup.localid < lookup.tileset->frames.size())
                sprite->opaque = lookup.tileset->frames[lookup.localid].opaque;
        }

        TiledObjectInstance instance;
        instance.spriteId = spriteId;
        instance.originalX = obj.x + objLayer->offsetX;
        instance.originalY = obj.y + objLayer->offsetY;
        instance.rotation = obj.rotation * (3.14159265358979323846f / 180.0f);
        instance.scaleX = obj.width > 0.0f
                              ? obj.width / lookup.tileset->tileWidth
                              : 1.0f;
        instance.scaleY = obj.height > 0.0f
                              ? obj.height / lookup.tileset->tileHeight
                              : 1.0f;
        instance.flipH = (obj.gid & 0x80000000u) != 0;
        instance.flipV = (obj.gid & 0x40000000u) != 0;
        instance.visible = obj.visible;

        objLayer->spriteInstances.push_back(instance);
    }

    Debugger::Instance().LogInfo("Initialized " +
                                 std::to_string(objLayer->spriteInstances.size()) +
                                 " objects in layer '" + objLayer->name + "'");
}

void Renderer::RenderObjectLayer(TiledObjectLayer *objLayer, size_t bufRefId,
                                 const CameraState &cam)
{
    if (!objLayer)
        return;

    for (const TiledObjectInstance &instance : objLayer->spriteInstances)
    {
        if (!instance.visible)
            continue;

        AnimatedSprite *sprite = GetSprite(instance.spriteId);
        if (!sprite)
            continue;

        sprite->x = instance.originalX;
        sprite->y = instance.originalY;
        sprite->rotation = instance.rotation;
        sprite->scaleX = instance.scaleX;
        sprite->scaleY = instance.scaleY;
        sprite->flipH = instance.flipH;
        sprite->flipV = instance.flipV;
        DrawSprite_Internal(instance.spriteId, bufRefId, cam);
    }
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


// cam work



void Renderer::DrawSprite(uint32_t spriteId, size_t bufRefId)
{
    ProcessPendingRegions(bufRefId);
    DrawSprite_Internal(spriteId, bufRefId, GetCameraState(bufRefId));
}

void Renderer::DrawSprite_Internal(uint32_t spriteId, size_t bufRefId,
                                   const CameraState &cam)
{
    AnimatedSprite *sprite = GetSprite(spriteId);
    if (!sprite)
    {
        Debugger::Instance().LogWarn("DrawSprite - sprite not found: " +
                                     std::to_string(spriteId));
        return;
    }

    SpriteAtlas *atlas = GetAtlas(sprite->atlasId);
    if (!atlas)
    {
        Debugger::Instance().LogWarn("DrawSprite - atlas not found: " +
                                     std::to_string(sprite->atlasId));
        return;
    }

    if (sprite->frameWidth == 0 || sprite->frameHeight == 0 ||
        sprite->framesPerRow == 0 || sprite->scaleX <= 0.0f ||
        sprite->scaleY <= 0.0f || cam.zoom <= 0.0f)
        return;

    float worldW = sprite->frameWidth * sprite->scaleX;
    float worldH = sprite->frameHeight * sprite->scaleY;
    const float cosRotation = std::cos(sprite->rotation);
    const float sinRotation = std::sin(sprite->rotation);
    const float boundsW = std::abs(worldW * cosRotation) +
                          std::abs(worldH * sinRotation);
    const float boundsH = std::abs(worldW * sinRotation) +
                          std::abs(worldH * cosRotation);
    const float centerWorldX = sprite->x + worldW / 2.0f;
    const float centerWorldY = sprite->y + worldH / 2.0f;
    if (!IsInFrustum(cam, centerWorldX, centerWorldY, boundsW, boundsH))
    {
        return;
    }

    const float screenRotation = sprite->rotation - cam.rotation;
    const float screenW = worldW * cam.zoom;
    const float screenH = worldH * cam.zoom;
    ScreenRect screenRect;
    const bool transformed = std::abs(screenRotation) > 0.000001f ||
                             std::abs(cam.rotation) > 0.000001f ||
                             std::abs(sprite->rotation) > 0.000001f;
    float centerScreenX = 0.0f;
    float centerScreenY = 0.0f;
    if (transformed)
    {
        const float dx = centerWorldX - cam.worldX;
        const float dy = centerWorldY - cam.worldY;
        const float cosCamera = std::cos(cam.rotation);
        const float sinCamera = std::sin(cam.rotation);
        centerScreenX = (dx * cosCamera + dy * sinCamera) * cam.zoom +
                        cam.viewWidth / 2.0f;
        centerScreenY = (-dx * sinCamera + dy * cosCamera) * cam.zoom +
                        cam.viewHeight / 2.0f;

        const float extentX = (std::abs(screenW * std::cos(screenRotation)) +
                               std::abs(screenH * std::sin(screenRotation))) /
                              2.0f;
        const float extentY = (std::abs(screenW * std::sin(screenRotation)) +
                               std::abs(screenH * std::cos(screenRotation))) /
                              2.0f;
        const int32_t left = static_cast<int32_t>(std::floor(centerScreenX - extentX));
        const int32_t top = static_cast<int32_t>(std::floor(centerScreenY - extentY));
        screenRect = {
            left,
            top,
            static_cast<uint32_t>(std::ceil(centerScreenX + extentX) - left),
            static_cast<uint32_t>(std::ceil(centerScreenY + extentY) - top)};
    }
    else
    {
        screenRect = WorldToScreen(cam, sprite->x, sprite->y, worldW, worldH);
    }

    const FrameRect srcRect = GetFrameRect(sprite, sprite->currentFrame);
    if (srcRect.w == 0 || srcRect.h == 0 ||
        static_cast<uint64_t>(srcRect.x) + srcRect.w > atlas->width ||
        static_cast<uint64_t>(srcRect.y) + srcRect.h > atlas->height)
        return;

    std::lock_guard<std::mutex> lock(buffers_mutex_);
    if (bufRefId >= shared_buffers_ref.size())
    {
        Debugger::Instance().LogWarn("DrawSprite - buffer reference out of range: " +
                                     std::to_string(bufRefId));
        return;
    }

    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s || !s->control)
    {
        Debugger::Instance().LogWarn("DrawSprite - buffer reference is incomplete");
        return;
    }

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    if (js_write >= 3 || !s->pixel_buffers[js_write])
    {
        Debugger::Instance().LogWarn("DrawSprite - write buffer is unavailable");
        return;
    }

    uint8_t *dstBuffer = s->pixel_buffers[js_write];

    if (transformed)
    {
        BlitSpriteRotated(dstBuffer, s->width, s->height, centerScreenX,
                          centerScreenY, screenW, screenH, screenRotation,
                          atlas, srcRect, sprite->flipH, sprite->flipV,
                          sprite->opaque);
    }
    else
    {
        if (sprite->opaque)
        {
            BlitSpriteNN_Opaque(dstBuffer, s->width, s->height, screenRect,
                                atlas, srcRect, sprite->flipH, sprite->flipV);
        }
        else
        {
            BlitSpriteNN_Alpha(dstBuffer, s->width, s->height, screenRect,
                               atlas, srcRect, sprite->flipH, sprite->flipV);
        }
    }

    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);
    if (dirty_count < MAX_DIRTY_REGIONS)
    {
        const int32_t left = std::max<int32_t>(0, screenRect.x);
        const int32_t top = std::max<int32_t>(0, screenRect.y);
        const int32_t right = std::min<int32_t>(
            s->width, screenRect.x + static_cast<int32_t>(screenRect.width));
        const int32_t bottom = std::min<int32_t>(
            s->height, screenRect.y + static_cast<int32_t>(screenRect.height));
        if (right > left && bottom > top)
        {
            const uint32_t offset = CTRL_DIRTY_REGIONS + dirty_count * 4;
            ctrl[offset] = left;
            ctrl[offset + 1] = top;
            ctrl[offset + 2] = right - left;
            ctrl[offset + 3] = bottom - top;
            ctrl[CTRL_DIRTY_COUNT].store(dirty_count + 1, std::memory_order_release);
        }
    }
}

// anim

uint32_t Renderer::CreateSpriteWithAnimations(uint32_t atlasId, uint32_t frameWidth,
                                              uint32_t frameHeight, bool opaque,
                                              const std::vector<AnimationDef> &animations)
{
    // Create base sprite
    AnimatedSprite *sprite = new AnimatedSprite();
    sprite->atlasId = atlasId;
    sprite->frameWidth = frameWidth;
    sprite->frameHeight = frameHeight;
    sprite->framesPerRow = 0; // Will be set from atlas
    sprite->opaque = opaque ? 1 : 0;

    // Calculate frames per row from atlas
    SpriteAtlas *atlas = GetAtlas(atlasId);
    if (atlas)
    {
        sprite->framesPerRow = atlas->width / frameWidth;
    }

    // Load animations
    uint32_t nextAnimId = 1;
    for (const auto &animDef : animations)
    {
        SpriteAnimation *anim = new SpriteAnimation();
        anim->name = animDef.name;
        anim->id = nextAnimId++;
        anim->fps = animDef.fps;
        anim->loop = animDef.loop;
        anim->frameCount = animDef.frames.size();

        // Copy frame array
        anim->frames = new uint32_t[anim->frameCount];
        for (size_t i = 0; i < anim->frameCount; i++)
        {
            anim->frames[i] = animDef.frames[i];
        }

        // Store in dictionaries
        sprite->animations[anim->id] = anim;
        sprite->animationNames[anim->name] = anim->id;
    }

    // Set initial frame to first frame of first animation (if any)
    if (!animations.empty() && animations[0].frames.size() > 0)
    {
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

void Renderer::PlayAnimation(uint32_t spriteId, const std::string &animName)
{
    AnimatedSprite *sprite = GetSprite(spriteId);
    if (!sprite)
        return;

    // Lookup animation ID by name
    auto it = sprite->animationNames.find(animName);
    if (it == sprite->animationNames.end())
    {
        Debugger::Instance().LogWarn("Animation '" + animName + "' not found on sprite " +
                                     std::to_string(spriteId));
        return;
    }

    PlayAnimationById(spriteId, it->second);
}

void Renderer::PlayAnimationById(uint32_t spriteId, uint32_t animId)
{
    AnimatedSprite *sprite = GetSprite(spriteId);
    if (!sprite)
        return;

    // Get animation
    auto it = sprite->animations.find(animId);
    if (it == sprite->animations.end())
        return;

    SpriteAnimation *anim = it->second;

    // Switch to this animation
    sprite->currentAnimationId = animId;
    sprite->playing = 1;
    sprite->frameTimer = 0.0f;

    // Set first frame
    if (anim->frameCount > 0)
    {
        sprite->currentFrame = anim->frames[0];
    }
}

void Renderer::UpdateSpriteAnimations(float deltaTime)
{
    std::lock_guard<std::mutex> lock(sprite_mutex_);

    for (auto &pair : sprites_)
    {
        AnimatedSprite *sprite = pair.second;

        if (!sprite->playing || sprite->currentAnimationId == 0)
            continue;

        // Get current animation
        auto it = sprite->animations.find(sprite->currentAnimationId);
        if (it == sprite->animations.end())
            continue;

        SpriteAnimation *anim = it->second;

        float frameDuration = 1.0f / anim->fps;
        sprite->frameTimer += deltaTime;

        // Advance frames
        while (sprite->frameTimer >= frameDuration)
        {
            sprite->frameTimer -= frameDuration;

            // Find current frame index
            uint32_t currentIdx = 0;
            for (uint32_t i = 0; i < anim->frameCount; i++)
            {
                if (anim->frames[i] == sprite->currentFrame)
                {
                    currentIdx = i;
                    break;
                }
            }

            // Advance
            currentIdx++;

            // Check end
            if (currentIdx >= anim->frameCount)
            {
                if (anim->loop)
                {
                    currentIdx = 0;
                }
                else
                {
                    currentIdx = anim->frameCount - 1;
                    sprite->playing = 0;
                    break;
                }
            }

            sprite->currentFrame = anim->frames[currentIdx];
        }
    }
}

uint32_t Renderer::CreateAnimator(const std::vector<std::string> &animNames,
                                  const std::vector<std::vector<std::string>> &framePaths,
                                  const std::vector<float> &fpsList,
                                  const std::vector<bool> &loopList)
{
    Animator *animator = new Animator();

    uint32_t nextAnimId = 1;
    for (size_t i = 0; i < animNames.size(); i++)
    {
        MultiAtlasAnimation *anim = new MultiAtlasAnimation();
        anim->name = animNames[i];
        anim->id = nextAnimId++;
        anim->fps = fpsList[i];
        anim->loop = loopList[i];

        // Load each frame's atlas
        for (const std::string &path : framePaths[i])
        {
            uint32_t atlasId = LoadAtlas(path);

            if (atlasId == 0)
            {
                Debugger::Instance().LogError("Failed to load animator frame: " + path);
                continue;
            }

            SpriteAtlas *atlas = GetAtlas(atlasId);
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

    Animator *anim = it->second;
    anim->x = x;
    anim->y = y;
    anim->rotation = rotation;
    anim->scaleX = scaleX;
    anim->scaleY = scaleY;
    anim->flipH = flipH ? 1 : 0;
    anim->flipV = flipV ? 1 : 0;
}

void Renderer::PlayAnimatorAnimation(uint32_t animatorId, const std::string &animName)
{
    std::lock_guard<std::mutex> lock(animator_mutex_);
    auto it = animators_.find(animatorId);
    if (it == animators_.end())
        return;

    Animator *animator = it->second;
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

    Animator *animator = it->second;

    if (animator->currentAnimationId == 0)
        return;

    auto animIt = animator->animations.find(animator->currentAnimationId);
    if (animIt == animator->animations.end())
        return;

    MultiAtlasAnimation *anim = animIt->second;

    if (animator->currentFrameIndex >= anim->frames.size())
        return;

    // Get current frame
    AnimatorFrame &frame = anim->frames[animator->currentFrameIndex];
    SpriteAtlas *atlas = GetAtlas(frame.atlasId);
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

    SharedBufferRefs *s = shared_buffers_ref[bufRefId];
    if (!s || !s->control)
        return;

    std::atomic<uint32_t> *ctrl = reinterpret_cast<std::atomic<uint32_t> *>(s->control);
    uint32_t js_write = ctrl[CTRL_JS_WRITE_IDX].load(std::memory_order_acquire);
    uint8_t *dstBuffer = s->pixel_buffers[js_write];

    // Source rect is entire atlas (loose sprite)
    FrameRect srcRect = {0, 0, frame.width, frame.height};

    // Blit (assume non-opaque for loose sprites)
    BlitSpriteNN_Alpha(dstBuffer, s->width, s->height, screenRect,
                       atlas, srcRect, animator->flipH, animator->flipV);

    // Mark dirty
    uint32_t dirty_count = ctrl[CTRL_DIRTY_COUNT].load(std::memory_order_acquire);
    if (dirty_count < MAX_DIRTY_REGIONS)
    {
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

    for (auto &pair : animators_)
    {
        Animator *animator = pair.second;

        if (!animator->playing || animator->currentAnimationId == 0)
            continue;

        auto it = animator->animations.find(animator->currentAnimationId);
        if (it == animator->animations.end())
            continue;

        MultiAtlasAnimation *anim = it->second;

        float frameDuration = 1.0f / anim->fps;
        animator->frameTimer += deltaTime;

        while (animator->frameTimer >= frameDuration)
        {
            animator->frameTimer -= frameDuration;
            animator->currentFrameIndex++;

            if (animator->currentFrameIndex >= anim->frames.size())
            {
                if (anim->loop)
                {
                    animator->currentFrameIndex = 0;
                }
                else
                {
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
    if (it != animators_.end())
    {
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