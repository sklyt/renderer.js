#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include "unordered_map"


class SharedBuffer
{
public:
    SharedBuffer(size_t size);
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
    // void Resize(size_t new_size)
    // {
    //     std::lock_guard<std::mutex> lock(mutex_);
    //     data_.resize(new_size);
    //     dirty_.store(true);
    // }

    // auto GetLastAccess() const { return last_access_; }
    bool ShouldCleanup() const
    {
        // auto now = std::chrono::steady_clock::now();
        // auto diff = std::chrono::duration_cast<std::chrono::minutes>(now - last_access_);
        // return diff.count() > 5; // Cleanup after 5 minutes of inactivity
    }

private:
    std::vector<unsigned char> data_;
    std::atomic<bool> dirty_;
    std::mutex mutex_;
    std::atomic<bool> locked_;
    bool use_external_;
    // std::chrono::time_point<double> last_access_;
};
