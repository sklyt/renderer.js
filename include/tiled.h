#pragma once

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include <string>
#include <vector>
#include <cstdio>

#include <cstdint>
#include <unordered_map>

// images and their range(gid- first gid to next gid, if a data point is in this range in a layer we read from this image, then use the gid to find the exact spot in that image)
struct TiledTileset
{
    uint32_t first_gid;
    uint32_t tileCount; // number of divisions in the image(tile)
    uint32_t tileWidth;
    uint32_t tileHeight;
    uint32_t atlasId; // internal sprite references
    uint32_t columns; // Tiles per row in atlas
    std::string name;
    std::string imagePath; // Relative path from map file
};

enum class TiledLayerType
{
    TILE,
    OBJECT,
    COLLISION,
};

// actual map data
struct TiledTileLayer
{
    std::string name;
    uint32_t width;
    uint32_t height;
    uint32_t *data; // array of tile IDs (width * height) point ot tileset

    int32_t offsetX;
    int32_t offsetY;
    bool visible;

    TiledTileLayer() : data(nullptr), width(0), height(0), offsetX(0), offsetY(0), visible(true) {}

    ~TiledTileLayer()
    {
        if (data)
        {
            delete[] data;
            data = nullptr;
        }
    }
};

// objecta

struct TiledObject
{
    uint32_t id;
    uint32_t gid; // Tile gid (for sprite objects)
    float x, y;
    float width, height;
    float rotation;
    std::string name;
    std::string type;
    bool visible;
};

// Object layer
struct TiledObjectLayer
{
    std::string name;
    std::vector<TiledObject> objects;
    int32_t offsetX;
    int32_t offsetY;
    bool visible;
};

enum class CollisionShapeType
{
    BOX = 0,
    CIRCLE = 1,
    POLYGON = 2,
    POINT = 3
};

// Collision shape
struct CollisionShape
{
    CollisionShapeType type;
    float x, y;
    float width, height;       // For box
    float radius;              // For circle
    std::vector<float> points; // For polygon (x,y,x,y,...)
    std::string name;
};

// wrapper for TiledTileLayer

struct TiledLayer
{
    TiledLayerType type;
    std::string name;
    bool visible;

    // only one is populated based on type
    TiledTileLayer *tileLayer;
    TiledObjectLayer *objectLayer;
    std::vector<CollisionShape> CollisionShapes;

    TiledLayer() : tileLayer(nullptr), objectLayer(nullptr) {}

    ~TiledLayer()
    {
        if (tileLayer)
        {
            delete tileLayer;
            tileLayer = nullptr;
        }
        if (objectLayer)
        {
            delete objectLayer;
            objectLayer = nullptr;
        }
    }
};



struct TiledMap{
    std::string basePath;
    uint32_t width;
    uint32_t height;
    uint32_t tileWidth;
    uint32_t tileHeight;

    std::vector<TiledTileset*> tilesets; // images and gids
    std::vector<TiledLayer*> layers; // actual map and shapes

     ~TiledMap() {
        for (auto* layer : layers) {
            delete layer; // trigger clean up(destructor)
        }
        layers.clear();
    }
};

TiledMap *ParseTiledMap(const std::string &path);