#pragma once

#include <cstdlib>
#include <memory>

#include "BaseAllocator.hpp"

class LinearAllocator final : public BaseAllocator {
private:
    void* start_ = nullptr;
    std::size_t offset_ = 0;

public:
    LinearAllocator(const std::size_t capacity) : BaseAllocator(capacity), start_(malloc(capacity)), offset_(0) {}

    ~LinearAllocator() {
        if (start_) {
            free(start_);
            start_ = nullptr;
        }
        offset_ = 0;
    }

    LinearAllocator(const LinearAllocator&) = delete;

    LinearAllocator& operator=(const LinearAllocator&) = delete;

    void* allocate(const std::size_t size, const std::size_t alignment = 0) override {
        auto current = (std::uintptr_t)start_ + offset_;
        auto alignedAddress = getAlignedAddress((void*)current, alignment);
        auto padding = std::uintptr_t(alignedAddress) - std::uintptr_t(current);
        if (padding + size > capacity_) {
            throw std::bad_alloc();
        }
        offset_ += padding + size;
        std::cout << "Allocated " << size << " bytes at address: " << alignedAddress << std::endl;
        std::cout << "Available memory: " << getAvailable() << " bytes" << std::endl;
        return alignedAddress;
    }

    void deallocate(void* ptr) override {
        // no-op
    }

    void construct(void* ptr) override {
        // no-op
    }

    void destruct(void* ptr) override {
        // no-op
    }

    void reset() {
        offset_ = 0;
    }

    std::size_t getAvailable() const {
        return capacity_ - offset_;
    }

    void printMemory() {
        hexdump(start_);
    }

};