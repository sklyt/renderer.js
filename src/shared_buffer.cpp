#include "shared_buffer.h"

SharedBuffer::SharedBuffer(size_t size)
    : data_(size), dirty_(false), locked_(false)
    //   last_access_(std::chrono::steady_clock::now())
{
}

SharedBuffer::~SharedBuffer()
{
    
}

void *SharedBuffer::GetData()
{
    // last_access_ = std::chrono::steady_clock::now();
    return data_.data();
}

size_t SharedBuffer::GetSize() const
{
    return data_.size();
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

void SharedBuffer::Resize(size_t new_size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.resize(new_size);
    dirty_.store(true);
}