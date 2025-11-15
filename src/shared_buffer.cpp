#include "shared_buffer.h"
#include <cstring>
#include <thread>

SharedBuffer::SharedBuffer(size_t size)
    : data_(size), dirty_(false), locked_(false), buffers_{std::vector<uint8_t>(size), std::vector<uint8_t>(size)}, write_locked_(false)
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

void* SharedBuffer::GetReadData()
{
    // safe to call without lock - atomic read
    return buffers_[read_index_.load()].data();
}

void* SharedBuffer::GetWriteData()
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


/**
 * keep checking if we can write with 100ms sleep in between if timeout > 0, basically trying to acquire a lock
 */
bool SharedBuffer::TryLockForWrite(int timeout_ms)
{
    bool expected = false;
    
    if (timeout_ms == 0) {
        // non-blocking attempt
        return write_locked_.compare_exchange_strong(expected, true);
    } else {
        // Blocking with timeout
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < 
               std::chrono::milliseconds(timeout_ms)) {
            if (write_locked_.compare_exchange_weak(expected, true)) {
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
    if (!dirty_.load() || write_locked_.load()) {
        return false;
    }
    
    // atomic swap of indices
    int current_read = read_index_.load();
    int current_write = write_index_.load();
    
    read_index_.store(current_write);
    write_index_.store(current_read);
    
    dirty_.store(false);
    swap_count_.fetch_add(1);
    
    return true;
}


void SharedBuffer::Resize(size_t new_size)
{
    std::lock_guard<std::mutex> lock(swap_mutex_);
    
    // wait for write to complete if ongoing
    while (write_locked_.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }


    buffers_[0].resize(new_size);
    buffers_[1].resize(new_size);

    dirty_.store(true);
}