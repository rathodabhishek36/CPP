#pragma once

#include <cstdlib>
#include <memory>

#include "BaseAllocator.hpp"

class Mallocator final : public BaseAllocator {
public:
    Mallocator() : BaseAllocator(0) {
        std::cout << "Constructed Mallocator" << std::endl;
    }

    ~Mallocator() {
        std::cout << "Destructed Mallocator" << std::endl;
    }

    Mallocator(const Mallocator&) = delete;

    Mallocator& operator=(const Mallocator&) = delete;

    void* allocate(const std::size_t size, const std::size_t alignment = 0) override {
        if (size % alignment != 0) {
            throw std::invalid_argument("Size must be a multiple of alignment");
        }
        void* ptr = std::aligned_alloc(alignment, size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    void deallocate(void* ptr) override {
        if (ptr) {
            std::free(ptr);
        }
    }
};