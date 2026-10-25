#include "tiled.h"
#include "debugger.h"
#include <fstream>
#include <sstream>

static std::string ReadFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        Debugger::Instance().LogError("Failed to open file: " + path);
        return "";
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    file.read(&buffer[0], size);

    return buffer;
}

// Helper: extract directory from file path
static std::string GetDirectory(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
        return "./";
    return path.substr(0, pos + 1);
}

// the array of images

static void ParseTilesets(const rapidjson::Value &tilesets, TiledMap *map)
{
    if (!tilesets.IsArray())
        return;

    for (rapidjson::SizeType i = 0; i < tilesets.Size(); i++)
    {
        const rapidjson::Value &ts = tilesets[i];

        TiledTileset *tileset = new TiledTileset();
        tileset->first_gid = ts["firstgid"].GetUint();
        tileset->tileCount = ts["tilecount"].GetUint();
        tileset->tileWidth = ts["tilewidth"].GetUint();
        tileset->tileHeight = ts["tileheight"].GetUint();
        tileset->columns = ts.HasMember("columns") ? ts["columns"].GetUint() : 0;
        tileset->name = ts["name"].GetString();


        // TODO:  parse multiple images in tiles
        if (ts.HasMember("image"))
        {
            tileset->imagePath = ts["image"].GetString();
        }
        else if (ts.HasMember("tiles") && ts["tiles"].IsArray() && ts["tiles"].Size() > 0)
        {
            const rapidjson::Value &firstTile = ts["tiles"][0];
            if (firstTile.HasMember("image"))
                tileset->imagePath = firstTile["image"].GetString();
        }

        tileset->atlasId = 0; // will be returned by sprite

        map->tilesets.push_back(tileset);
    }
}

// actual data

static TiledTileLayer *ParseTileLayer(const rapidjson::Value &layer)
{
    TiledTileLayer *tileLayer = new TiledTileLayer();

    tileLayer->name = layer["name"].GetString();
    tileLayer->width = layer["width"].GetUint();
    tileLayer->height = layer["height"].GetUint();
    tileLayer->visible = layer.HasMember("visible") ? layer["visible"].GetBool() : true;
    tileLayer->offsetX = layer.HasMember("offsetx") ? layer["offsetx"].GetInt() : 0;
    tileLayer->offsetY = layer.HasMember("offsety") ? layer["offsety"].GetInt() : 0;

    if (layer.HasMember("data") && layer["data"].IsArray())
    {
        const rapidjson::Value &data = layer["data"];
        size_t count = data.Size();

        tileLayer->data = new uint32_t[count];

        for (rapidjson::SizeType i = 0; i < count; i++)
        {
            tileLayer->data[i] = data[i].GetUint();
        }
    }

    return tileLayer;
}

static TiledObjectLayer *ParseObjectLayer(const rapidjson::Value &layer)
{
    TiledObjectLayer *objLayer = new TiledObjectLayer();

    objLayer->name = layer["name"].GetString();
    objLayer->visible = layer.HasMember("visible") ? layer["visible"].GetBool() : true;
    objLayer->offsetX = layer.HasMember("offsetx") ? layer["offsetx"].GetInt() : 0;
    objLayer->offsetY = layer.HasMember("offsety") ? layer["offsety"].GetInt() : 0;

    // Parse objects array
    if (layer.HasMember("objects") && layer["objects"].IsArray())
    {
        const rapidjson::Value &objects = layer["objects"];

        for (rapidjson::SizeType i = 0; i < objects.Size(); i++)
        {
            const rapidjson::Value &obj = objects[i];

            TiledObject tiledObj;
            tiledObj.id = obj["id"].GetUint();
            tiledObj.x = obj["x"].GetFloat();
            tiledObj.y = obj["y"].GetFloat();
            tiledObj.width = obj.HasMember("width") ? obj["width"].GetFloat() : 0.0f;
            tiledObj.height = obj.HasMember("height") ? obj["height"].GetFloat() : 0.0f;
            tiledObj.rotation = obj.HasMember("rotation") ? obj["rotation"].GetFloat() : 0.0f;
            tiledObj.visible = obj.HasMember("visible") ? obj["visible"].GetBool() : true;
            tiledObj.gid = obj.HasMember("gid") ? obj["gid"].GetUint() : 0;

            if (obj.HasMember("name"))
                tiledObj.name = obj["name"].GetString();
            if (obj.HasMember("type"))
                tiledObj.type = obj["type"].GetString();

            objLayer->objects.push_back(tiledObj);
        }
    }

    return objLayer;
}

static void ParseCollisionShapes(const rapidjson::Value &layer, std::vector<CollisionShape> &shapes)
{
    if (!layer.HasMember("objects") || !layer["objects"].IsArray())
        return;

    const rapidjson::Value &objects = layer["objects"];

    for (rapidjson::SizeType i = 0; i < objects.Size(); i++)
    {
        const rapidjson::Value &obj = objects[i];

        CollisionShape shape;
        shape.x = obj["x"].GetFloat();
        shape.y = obj["y"].GetFloat();

        if (obj.HasMember("name"))
            shape.name = obj["name"].GetString();

        // Determine shape type
        if (obj.HasMember("ellipse") && obj["ellipse"].GetBool())
        {
            // Circle
            shape.type = CollisionShapeType::CIRCLE;
            shape.radius = obj["width"].GetFloat() / 2.0f;
            shape.x += shape.radius; // Center point
            shape.y += shape.radius;
        }
        else if (obj.HasMember("polygon") && obj["polygon"].IsArray())
        {
            // Polygon
            shape.type = CollisionShapeType::POLYGON;
            const rapidjson::Value &points = obj["polygon"];

            for (rapidjson::SizeType p = 0; p < points.Size(); p++)
            {
                shape.points.push_back(points[p]["x"].GetFloat());
                shape.points.push_back(points[p]["y"].GetFloat());
            }
        }
        else if (obj.HasMember("point") && obj["point"].GetBool())
        {
            // Point
            shape.type = CollisionShapeType::POINT;
        }
        else
        {
            // Box (default)
            shape.type = CollisionShapeType::BOX;
            shape.width = obj.HasMember("width") ? obj["width"].GetFloat() : 0.0f;
            shape.height = obj.HasMember("height") ? obj["height"].GetFloat() : 0.0f;
        }

        shapes.push_back(shape);
    }
}

static void ParseLayers(const rapidjson::Value &layers, TiledMap *map)
{

    if (!layers.IsArray())
        return;

    for (rapidjson::SizeType i = 0; i < layers.Size(); i++)
    {
        const rapidjson::Value &layerJson = layers[i];

        std::string type = layerJson["type"].GetString();
        bool visible = layerJson.HasMember("visible") ? layerJson["visible"].GetBool() : true;

        if (!visible)
            continue; // Skip invisible layers

        TiledLayer *layer = new TiledLayer();
        layer->name = layerJson["name"].GetString();
        layer->visible = visible;

        if (type == "tilelayer")
        {
            layer->type = TiledLayerType::TILE;
            layer->tileLayer = ParseTileLayer(layerJson);
        }
        else if (type == "objectgroup")
        {
            // Determine if it's collision or regular objects
            // Check if first object has gid (sprite objects) or shapes (collision)
            bool hasGid = false;
            if (layerJson.HasMember("objects") && layerJson["objects"].Size() > 0)
            {
                hasGid = layerJson["objects"][0].HasMember("gid");
            }

            if (hasGid)
            {
                layer->type = TiledLayerType::OBJECT;
                layer->objectLayer = ParseObjectLayer(layerJson);
            }
            else
            {
                layer->type = TiledLayerType::COLLISION;
                ParseCollisionShapes(layerJson, layer->CollisionShapes);
            }
        }

        map->layers.push_back(layer);
    }
}

TiledMap *ParseTiledMap(const std::string &path)
{
    Debugger::Instance().LogInfo("Parsing Tiled map: " + path);

    std::string jsonContent = ReadFile(path);

    if (jsonContent.empty())
    {
        Debugger::Instance().LogError("Failed to read Tiled map file");
        return nullptr;
    }

    rapidjson::Document doc;
    doc.Parse(jsonContent.c_str());

    if (doc.HasParseError())
    {
        Debugger::Instance().LogError("JSON parse error at offset " +
                                      std::to_string(doc.GetErrorOffset()));
        return nullptr;
    }

    TiledMap *map = new TiledMap();
    map->basePath = GetDirectory(path);
    map->width = doc["width"].GetUint();
    map->height = doc["height"].GetUint();
    map->tileWidth = doc["tilewidth"].GetUint();
    map->tileHeight = doc["tileheight"].GetUint();

    // Parse sections
    if (doc.HasMember("tilesets"))
        ParseTilesets(doc["tilesets"], map);

    if (doc.HasMember("layers"))
        ParseLayers(doc["layers"], map);

    Debugger::Instance().LogInfo("Parsed map: " + std::to_string(map->width) + "x" +
                                 std::to_string(map->height) + ", " +
                                 std::to_string(map->layers.size()) + " layers");

    return map;
}