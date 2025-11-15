#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include "unordered_map"
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <chrono>

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

    void *GetReadData();  // For render thread - always safe to read
    void *GetWriteData(); // For JavaScript thread - gets current write buffer

    // double buffering control
    bool SwapBuffers();
    bool TryLockForWrite(int timeout_ms = 0);
    void UnlockWrite();

    size_t GetSwapCount() const { return swap_count_.load(); }
    bool IsWriteLocked() const { return write_locked_.load(); }

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


    SharedBuffer(const SharedBuffer &) = delete;
    SharedBuffer &operator=(const SharedBuffer &) = delete;
};
