#pragma once

#include <cstdlib>
#include <memory>

#include "BaseAllocator.hpp"

/*
* Idea is to manage the memory as a Stack. So, as LinerAllocator, we keep a pointer to the current memory address
* and we move it forward for every allocation. However, we also can move it backwards when a free operation is done. 
* Similar to LinearAllocator, we keep the spatial locality principle and the fragmentation is still very low.
* Time Complexity: O(1) for allocation and deallocation
* Constraints: Can only deallocate the last allocated element.
*/

template<std::size_t CAPACITY>
class StackAllocator : public BaseAllocator {

    struct Header {
        std::size_t prevPadding_;

        explicit Header(std::size_t padding) : prevPadding_(padding) {}
    };

    std::byte buffer_[CAPACITY];
    std::size_t offset_;

    std::size_t getPadding(void* start, std::size_t alignment) {
        auto prefixSpace = sizeof(Header);
        auto nextAlignedAddr = getAlignedAddress(start, alignment);
        auto padding = std::uintptr_t(nextAlignedAddr) - std::uintptr_t(start);

        if (prefixSpace > padding) {
            prefixSpace -= padding;
            if ((prefixSpace % alignment) > 0) {
                padding += prefixSpace + alignment - (prefixSpace % alignment);
            } else {
                padding += prefixSpace;
            }
        }
        return padding;
    }
    
public:
    StackAllocator() : BaseAllocator(CAPACITY), offset_(0) {
        std::memset(buffer_, 0, CAPACITY);
        std::cout << "Constructed StackAllocator with Capacity: " << CAPACITY << std::endl;
        std::cout << "Per Allocation Header Size: " << sizeof(Header) << std::endl;
    }

    ~StackAllocator() override {
        std::cout << "Destruced StackAllocator" << std::endl;
    }

    StackAllocator(const StackAllocator&) = delete;

    StackAllocator& operator=(const StackAllocator&) = delete;

    void* allocate(const std::size_t size, const std::size_t alignment = 0) override {
        auto padding = getPadding(buffer_ + offset_, alignment);
        if (offset_ + padding + size > CAPACITY) {
            throw std::bad_alloc();
        }
        void* newAddress = (buffer_ + offset_ + padding);
        auto headerAddr = (char*)newAddress - sizeof(Header);
        new (headerAddr) Header(padding);
        offset_ += padding + size;
        std::cout << "Allocated " << size << " bytes at address: " << buffer_+padding << " with alignment: " << alignment << std::endl;
        std::cout << "Available memory: " << getAvailable() << " bytes" << std::endl;
        return newAddress;
    }

    void deallocate(void* ptr) override {
        auto headerAddr = static_cast<unsigned char*>(ptr) - sizeof(Header);
        auto header = reinterpret_cast<Header*>(headerAddr);
        auto padding = header->prevPadding_;
        offset_ = ((unsigned char*)ptr - (unsigned char*)buffer_) - padding;
        header->~Header();
        std::cout << "Deallocated memory, Available memory: " << getAvailable() << " bytes" << std::endl;
    }

    std::size_t getAvailable() const {
        return capacity_ - offset_;
    }

    void printMemory() {
        hexdump(buffer_);
    }
};