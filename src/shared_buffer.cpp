#include "shared_buffer.h"
#include <cstring>
#include <thread>
#include "debugger.h"

constexpr float FULL_DIRTY_THRESHOLD = 0.75f;
constexpr size_t MAX_DIRTY_REGIONS = 64;

SharedBuffer::SharedBuffer(size_t size, int width, int height)
    : data_(size), buffer_height_(height), buffer_width_(width), dirty_(false), locked_(false), buffers_{std::vector<uint8_t>(size), std::vector<uint8_t>(size)}, write_locked_(false)
//   last_access_(std::chrono::steady_clock::now())
{
    if (size > 0)
    {
        memset(buffers_[0].data(), 0, size);
        memset(buffers_[1].data(), 0, size);
    }
}

SharedBuffer::~SharedBuffer()
{
    std::lock_guard<std::mutex> lock(swap_mutex_); // <- make sure no one is writing
}
/**
 * @deprecated in favor of double buffering
 */
void *SharedBuffer::GetData()
{
    // last_access_ = std::chrono::steady_clock::now();
    return data_.data();
}

void *SharedBuffer::GetReadData()
{
    // safe to call without lock - atomic read
    return buffers_[read_index_.load()].data();
}

void *SharedBuffer::GetWriteData()
{

    return buffers_[write_index_.load()].data();
}

size_t SharedBuffer::GetSize() const
{
    return buffers_[0].size();
}

bool SharedBuffer::IsDirty() const
{
    return dirty_.load();
}

void SharedBuffer::MarkDirty()
{
    dirty_.store(true);
}

void SharedBuffer::MarkClean()
{
    dirty_.store(false);
}

std::vector<DirtyRect> SharedBuffer::GetDirtyRegions() const
{
    std::lock_guard<std::mutex> lock(dirty_mutex_);

    if (fully_dirty_)
    {
        if (buffer_width_ > 0 && buffer_height_ > 0)
        {
            return {DirtyRect(0, 0, buffer_width_, buffer_height_)};
        }
        // no dimensions set, can't create rect
        return {};
    }

    // copy of dirty regions
    return dirty_regions_;
}

void SharedBuffer::ClearDirtyRegions()
{
    std::lock_guard<std::mutex> lock(dirty_mutex_);
    dirty_regions_.clear();
    fully_dirty_ = false;
}

void SharedBuffer::MarkRegionDirty(int x, int y, int width, int height)
{

    if (fully_dirty_)
    {
        return;
    }

    if (buffer_width_ <= 0 || buffer_height_ <= 0)
    {
        MarkFullyDirty();
        return;
    }

    // clamp region to buffer bounds
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > buffer_width_)
    {
        width = buffer_width_ - x;
    }
    if (y + height > buffer_height_)
    {
        height = buffer_height_ - y;
    }

    // validate result
    if (width <= 0 || height <= 0)
    {
        return; // Invalid region, ignore
    }

    DirtyRect new_rect(x, y, width, height);

    std::lock_guard<std::mutex> lock(dirty_mutex_);

    dirty_regions_.push_back(new_rect);

    // optimize if we have too many regions
    if (dirty_regions_.size() > MAX_DIRTY_REGIONS)
    {
        OptimizeDirtyRegions();
    }

    float coverage = CalculateDirtyCoverage();
    if (coverage > FULL_DIRTY_THRESHOLD)
    {
        dirty_regions_.clear();
        fully_dirty_ = true;
    }
}

void SharedBuffer::MarkFullyDirty()
{
    std::lock_guard<std::mutex> lock(dirty_mutex_);
    dirty_regions_.clear();
    fully_dirty_ = true;
}

void SharedBuffer::OptimizeDirtyRegions()
{
    if (dirty_regions_.empty())
    {
        return;
    }

    std::vector<DirtyRect> optimized;
    optimized.reserve(dirty_regions_.size());

    optimized.push_back(dirty_regions_[0]);

    // try to merge each subsequent region
    for (size_t i = 1; i < dirty_regions_.size(); ++i)
    {
        const DirtyRect &current = dirty_regions_[i];
        bool merged = false;

        for (size_t j = 0; j < optimized.size(); ++j)
        {
            DirtyRect &existing = optimized[j];

            if (current.Overlaps(existing) || current.IsAdjacent(existing))
            {
                DirtyRect merged_rect = existing.Merge(current);

                // Only merge if area increase is reasonable
                // (avoid creating huge rects from small overlaps)
                int old_area = existing.Area() + current.Area();
                int new_area = merged_rect.Area();

                // Allow up to 50% area increase from merge
                if (new_area <= old_area * 1.5f)
                {
                    existing = merged_rect;
                    merged = true;
                    break;
                }
            }
        }

        if (!merged)
        {
            optimized.push_back(current);
        }
    }

    dirty_regions_ = std::move(optimized);
}

float SharedBuffer::CalculateDirtyCoverage() const
{

    if (fully_dirty_ || buffer_width_ <= 0 || buffer_height_ <= 0)
    {
        return 1.0f;
    }

    if (dirty_regions_.empty())
    {
        return 0.0f;
    }

    int total_area = buffer_width_ * buffer_height_;

    // Simple approach: sum all dirty areas
    // (slight overestimation due to overlaps, but fast)
    int dirty_area = 0;
    for (const auto &rect : dirty_regions_)
    {
        dirty_area += rect.Area();
    }

    // cap at 100% to handle overlap overestimation
    return std::min(1.0f, static_cast<float>(dirty_area) / total_area);
}

/**
 * keep checking if we can write with 100ms sleep in between if timeout > 0, basically trying to acquire a lock
 */
bool SharedBuffer::TryLockForWrite(int timeout_ms = 0)
{
    bool expected = false;

    if (timeout_ms == 0)
    {
        // non-blocking attempt
        return write_locked_.compare_exchange_strong(expected, true);
    }
    else
    {
        // Blocking with timeout
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start <
               std::chrono::milliseconds(timeout_ms))
        {
            if (write_locked_.compare_exchange_weak(expected, true))
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            expected = false; // Reset for next attempt
        }
        return false;
    }
}

void SharedBuffer::UnlockWrite()
{
    write_locked_.store(false);
}

bool SharedBuffer::SwapBuffers()
{
    std::lock_guard<std::mutex> lock(swap_mutex_);

    // nly swap if write buffer is dirty and not currently being written to
    if (!dirty_.load() || write_locked_.load())
    {
        return false;
    }

    // atomic swap of indices
    int current_read = read_index_.load();
    int current_write = write_index_.load();

    // Copy dirty regions from write to read buffer BEFORE swap
    // so the new write buffer has the latest data
    // TODO: consider tripple buffering
    std::vector<DirtyRect> regions_to_copy = GetDirtyRegions();

    if (!regions_to_copy.empty() && buffer_width_ > 0 && buffer_height_ > 0)
    {
        uint8_t *src = buffers_[current_write].data();
        uint8_t *dst = buffers_[current_read].data();
        int bytes_per_pixel = 4; // RGBA

        for (const auto &region : regions_to_copy)
        {
            for (int row = 0; row < region.height; ++row)
            {
                int offset = ((region.y + row) * buffer_width_ + region.x) * bytes_per_pixel;
                int row_bytes = region.width * bytes_per_pixel;
                memcpy(dst + offset, src + offset, row_bytes);
            }
        }
    }
    read_index_.store(current_write);
    write_index_.store(current_read);

    dirty_.store(false);
    swap_count_.fetch_add(1);
    ClearDirtyRegions();
    return true;
}

void SharedBuffer::Resize(size_t new_size)
{
    std::lock_guard<std::mutex> lock(swap_mutex_);

    // wait for write to complete if ongoing
    while (write_locked_.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    buffers_[0].resize(new_size);
    buffers_[1].resize(new_size);

    dirty_.store(true);
}