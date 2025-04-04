#pragma once

#include <cstdlib>
#include <memory>

#include "BaseAllocator.hpp"

/*
* Idea is to keep a pointer at the first memory address of your memory chunk and move it every time an allocation is done. 
* In this allocator, the internal fragmentation is kept to a minimum because all elements are sequentially (spatial locality) inserted
* and the only fragmentation between them is the alignment.
* Time Complexity: O(1) for allocation and deallocation
* Constraints: Cannot deallocte signle element, only the whole memory chunk can be deallocated
*/

template<std::size_t CAPACITY>
class LinearAllocator final : public BaseAllocator {
private:
    char buffer_[CAPACITY];
    std::size_t offset_ = 0;

public:
    LinearAllocator() : BaseAllocator(CAPACITY), offset_(0) {
        std::memset(buffer_, 0, CAPACITY);
        std::cout << "Contructed LinearAllocator with capacity: " << CAPACITY << std::endl;
    }

    ~LinearAllocator() {
        std::cout << "Destructed LinearAllocator" << std::endl;
    }

    LinearAllocator(const LinearAllocator&) = delete;

    LinearAllocator& operator=(const LinearAllocator&) = delete;

    void* allocate(const std::size_t size, const std::size_t alignment = 0) override {
        auto current = (std::uintptr_t)buffer_ + offset_;
        auto alignedAddress = getAlignedAddress((void*)current, alignment);
        auto padding = std::uintptr_t(alignedAddress) - std::uintptr_t(current);
        if (padding + size > CAPACITY) {
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

    void reset() {
        offset_ = 0;
    }

    std::size_t getAvailable() const {
        return CAPACITY - offset_;
    }

    void printMemory() {
        hexdump(buffer_);
    }

};