#include "debugger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

Debugger &Debugger::Instance()
{
    static Debugger inst;
    return inst;
}

Debugger::Debugger()
{
    // optionally reserve
    logLines_.reserve(maxStoredLogs_);
    shapes_.reserve(128);
}

std::string Debugger::Timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}



void Debugger::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lk(enabledMutex_);
    enabled_ = enabled;
}

bool Debugger::IsEnabled() const {
    std::lock_guard<std::mutex> lk(enabledMutex_);
    return enabled_;
}


static inline bool Debugger_IsEnabledFast(const Debugger& d) {
    return d.IsEnabled(); // cheap wrapper (mutex inside)
}


void Debugger::Log(const std::string &level, const std::string &msg)
{
    std::string full = "[" + level + "] " + Timestamp() + " - " + msg;

    // console
    {
        std::lock_guard<std::mutex> lk(logMutex_);
        // keep a ring / cap
        if (logLines_.size() >= maxStoredLogs_)
        {
            // drop oldest
            logLines_.erase(logLines_.begin());
        }
        logLines_.push_back(full);
    }

    // also print to stdout immediately
    std::cout << full << std::endl;
}

void Debugger::LogInfo(const std::string &msg) { Log("INFO", msg); }
void Debugger::LogWarn(const std::string &msg) { Log("WARN", msg); }
void Debugger::LogError(const std::string &msg) { Log("ERROR", msg); }

void Debugger::AddShape(const DebugShape &shape, Color color, float durationSeconds)
{
    ShapeInstance inst;
    inst.shape = shape;
    inst.color = color;
    inst.durationSeconds = durationSeconds;
    inst.createdAt = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lk(shapesMutex_);
    shapes_.push_back(std::move(inst));
}

void Debugger::ClearShapes()
{
    std::lock_guard<std::mutex> lk(shapesMutex_);
    shapes_.clear();
}

void Debugger::DrawArrow(const Vector2 &start, const Vector2 &end, float thickness, Color color)
{
    // Draw main line
    DrawLineEx(start, end, thickness, color);

    // Arrowhead
    const float arrowSize = 8.0f * (thickness); // tweak scale
    Vector2 dir = {end.x - start.x, end.y - start.y};
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len <= 0.0001f)
        return;
    dir.x /= len;
    dir.y /= len;

    // perpendicular
    Vector2 perp = {-dir.y, dir.x};

    Vector2 p1 = {end.x - dir.x * arrowSize + perp.x * (arrowSize * 0.5f),
                  end.y - dir.y * arrowSize + perp.y * (arrowSize * 0.5f)};
    Vector2 p2 = {end.x - dir.x * arrowSize - perp.x * (arrowSize * 0.5f),
                  end.y - dir.y * arrowSize - perp.y * (arrowSize * 0.5f)};

    DrawTriangle((Vector2){end.x, end.y}, p1, p2, color);
}

void Debugger::DrawDebugShapes()
{
    if (!showShapes)
        return;

    std::lock_guard<std::mutex> lk(shapesMutex_);
    using clock = std::chrono::steady_clock;
    auto now = clock::now();

    // draw copy to avoid holding lock for long? we draw while locked here for simplicity
    // we will remove expired ones
    auto it = shapes_.begin();
    while (it != shapes_.end())
    {
        bool expired = false;
        if (it->durationSeconds > 0.0f)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(now - it->createdAt).count();
            if (elapsed >= it->durationSeconds)
                expired = true;
        }

        if (expired)
        {
            it = shapes_.erase(it);
            continue;
        }

        // Draw the variant
        std::visit([&](auto &&s)
                   {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, LineShape>) {
                DrawLineEx(s.a, s.b, s.thickness, it->color);
            } else if constexpr (std::is_same_v<T, RectShape>) {
                if (s.filled) {
                    DrawRectangleRec(s.rect, it->color);
                } else {
                    DrawRectangleLinesEx(s.rect, 1, it->color);
                }
            } else if constexpr (std::is_same_v<T, CircleShape>) {
                DrawCircleV(s.center, s.radius, it->color);
            } else if constexpr (std::is_same_v<T, RayShape>) {
                Vector2 end;
                if (s.length > 0.0f) {
                    end = { s.origin.x + s.dir.x * s.length, s.origin.y + s.dir.y * s.length };
                } else {
                    // if no length, draw a short line in dir
                    end = { s.origin.x + s.dir.x * 32.0f, s.origin.y + s.dir.y * 32.0f };
                }
                DrawLineEx(s.origin, end, s.thickness, it->color);
                DrawArrow(s.origin, end, s.thickness, it->color);
            } else if constexpr (std::is_same_v<T, ArrowShape>) {
                Vector2 end = { s.origin.x + s.vec.x, s.origin.y + s.vec.y };
                DrawArrow(s.origin, end, s.thickness, it->color);
            } else if constexpr (std::is_same_v<T, PointShape>) {
                DrawCircleV(s.p, s.size, it->color);
            } }, it->shape);

        ++it;
    }
}

void Debugger::DrawLogOverlay(int maxLines, int startX, int startY)
{
    if (!showLogOverlay)
        return;

    // grab a reversed slice of logLines_ safely
    std::vector<std::string> copy;
    {
        std::lock_guard<std::mutex> lk(logMutex_);
        size_t lines = std::min<size_t>(maxLines, logLines_.size());
        copy.reserve(lines);
        for (size_t i = 0; i < lines; ++i)
        {
            // get last lines
            copy.push_back(logLines_[logLines_.size() - lines + i]);
        }
    }

    // Draw background box
    const int padding = 6;
    const int lineHeight = 18;
    int boxW = 700;
    int boxH = (int)copy.size() * lineHeight + padding * 2;
    DrawRectangle(startX - 4, startY - 4, boxW, boxH, (Color){0, 0, 0, 120});

    // draw each
    int y = startY + padding;
    for (const auto &ln : copy)
    {
        DrawText(ln.c_str(), startX, y, 14, WHITE);
        y += lineHeight;
    }
}
