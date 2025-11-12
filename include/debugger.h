#pragma once

#include <raylib.h>
#include <variant>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include "napi.h"

struct LineShape
{
    Vector2 a, b;
    float thickness = 1.0f;
};
struct RectShape
{
    Rectangle rect;
    bool filled = false;
};
struct CircleShape
{
    Vector2 center;
    float radius;
};
struct RayShape
{
    Vector2 origin;
    Vector2 dir;
    float length = 0.0f;
    float thickness = 1.0f;
}; // dir should be normalized if length>0
struct ArrowShape
{
    Vector2 origin;
    Vector2 vec;
    float thickness = 1.0f;
}; // draw vector arrow
struct PointShape
{
    Vector2 p;
    float size = 2.0f;
};

using DebugShape = std::variant<LineShape, RectShape, CircleShape, RayShape, ArrowShape, PointShape>;

class Debugger
{
public:
    static Debugger &Instance();

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    void LogInfo(const std::string &msg);
    void LogWarn(const std::string &msg);
    void LogError(const std::string &msg);
    void Log(const std::string &level, const std::string &msg);

    // Draw / shapes
    // durationSeconds = 0 => draw for single frame and cleared next DrawDebugShapes() call
    void AddShape(const DebugShape &shape, Color color = GREEN, float durationSeconds = 0.0f);
    void ClearShapes(); // remove all shapes immediately

    void DrawDebugShapes();                                                   // draws shapes and removes expired ones
    void DrawLogOverlay(int maxLines = 12, int startX = 10, int startY = 10); // optional on-screen log

    // settings (public for quick tuning)
    bool showLogOverlay = true;
    bool showShapes = true;

private:
    Debugger();
    ~Debugger() = default;
    Debugger(const Debugger &) = delete;
    Debugger &operator=(const Debugger &) = delete;

    struct ShapeInstance
    {
        DebugShape shape;
        Color color;
        float durationSeconds; // 0 => single-frame
        std::chrono::steady_clock::time_point createdAt;
    };

    std::vector<ShapeInstance> shapes_;
    std::mutex shapesMutex_;

    std::vector<std::string> logLines_;
    std::mutex logMutex_;
    size_t maxStoredLogs_ = 200;

    bool enabled_ = false;
    mutable std::mutex enabledMutex_;

    // helpers
    std::string Timestamp();
    void DrawArrow(const Vector2 &start, const Vector2 &end, float thickness, Color color);
};


Napi::Value SetDebugEnabledWrapped(const Napi::CallbackInfo& info);

Napi::Value IsDebugEnabledWrapped(const Napi::CallbackInfo& info);

Napi::Object DebuggerWrapperInit(Napi::Env env, Napi::Object exports);
