#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include "unordered_map"
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <chrono>
#include <algorithm>

struct DirtyRect
{
    int x, y, width, height;

    DirtyRect() : x(0), y(0), width(0), height(0) {}
    DirtyRect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}

    bool Overlaps(const DirtyRect &other) const
    {
        return !(x + width <= other.x ||
                 other.x + other.width <= x ||
                 y + height <= other.y ||
                 other.y + other.height <= y);
    }

    bool IsAdjacent(const DirtyRect &other) const
    {

        bool horizontal = (x + width == other.x || other.x + other.width == x) &&
                          !(y + height <= other.y || other.y + other.height <= y);
        bool vertical = (y + height == other.y || other.y + other.height == y) &&
                        !(x + width <= other.x || other.x + other.width <= x);
        return horizontal || vertical;
    }

    /**
     * merge overlaps and adjancents
     */
    DirtyRect Merge(const DirtyRect &other) const
    {
        int x1 = std::min(x, other.x);
        int y1 = std::min(y, other.y);
        int x2 = std::max(x + width, other.x + other.width);
        int y2 = std::max(y + height, other.y + other.height);
        return DirtyRect(x1, y1, x2 - x1, y2 - y1);
    }

    int Area() const
    {
        return width * height;
    }

    bool IsValid() const
    {
        return width > 0 && height > 0;
    }
};

class SharedBuffer
{
public:
    SharedBuffer(size_t size, int width, int height);
    ~SharedBuffer();

    void *GetData();
    size_t GetSize() const;
    bool IsDirty() const;
    void MarkDirty();
    void MarkClean();
    void Resize(size_t new_size);
    bool IsLocked() const { return locked_.load(); }
    void Lock() { locked_.store(true); }
    void Unlock() { locked_.store(false); }

    void *GetReadData();  // For render thread - always safe to read
    void *GetWriteData(); // For JavaScript thread - gets current write buffer

    // double buffering control
    bool SwapBuffers();
    bool TryLockForWrite(int timeout_ms = 0);
    void UnlockWrite();

    size_t GetSwapCount() const { return swap_count_.load(); }
    bool IsWriteLocked() const { return write_locked_.load(); }

    std::vector<DirtyRect> GetDirtyRegions() const;
    void ClearDirtyRegions();
    void MarkRegionDirty(int x, int y, int width, int height);
    void MarkFullyDirty();
    void SetDimensions(int width, int height)
    {
        buffer_width_ = width;
        buffer_height_ = height;
    }
    int GetWidth() {return buffer_width_;};
    int GetHeight() {return buffer_height_;};

private:
    std::vector<unsigned char> data_;
    std::mutex mutex_;
    std::atomic<bool> locked_;
    bool use_external_;
    // std::chrono::time_point<double> last_access_;
    std::vector<uint8_t> buffers_[2];
    std::atomic<int> read_index_{0};        // current read buffer index
    std::atomic<int> write_index_{1};       // current write buffer index
    std::atomic<bool> dirty_{false};        // write buffer has new data
    std::atomic<bool> write_locked_{false}; // write buffer is being written to

    std::mutex swap_mutex_;
    std::condition_variable swap_cv_;

    std::atomic<size_t> swap_count_{0};

    // buffer dimensions (for dirty rect validation)
    int buffer_width_;
    int buffer_height_;

    mutable std::mutex dirty_mutex_;
    std::vector<DirtyRect> dirty_regions_;
    bool fully_dirty_;

    SharedBuffer(const SharedBuffer &) = delete;
    SharedBuffer &operator=(const SharedBuffer &) = delete;

    void OptimizeDirtyRegions();
    float CalculateDirtyCoverage() const;
};
