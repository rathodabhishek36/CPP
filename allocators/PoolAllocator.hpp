#pragma once

#include <cstdlib>
#include <memory>

#undef NDEBUG   // To enable assert, if NDEBUG is defined, it disables assert
#include <cassert>

#include "BaseAllocator.hpp"

/*
* Idea is to split a big memory chunk in smaller chunks of the same size and keeps track of which of them are free. 
* When an allocation is requested it returns the free chunk size. When a chunk is freed, it just stores it to be used in the next allocation.
* This way, allocations work super fast and the fragmentation is still very low.
* Time Complexity: O(1) for allocation and deallocation
* Constraints: Can return only chunks of the same size.
*/

class PoolAllocator final : public BaseAllocator {
    struct List {
        struct Node {
            Node* next_;
        };
        Node* head_;

        List() : head_(nullptr) {}

        void push(Node* address) {
            address->next_ = head_;
            head_ = address;
        }

        Node* pop() {
            if (!head_) {
                return nullptr;
            }
            Node* node = head_;
            head_ = head_->next_;
            return node;
        }
    };

    void* buffer_;
    std::size_t blockSize_;
    std::size_t allocatedBlocks_;
    List freeList_;

    inline void initFreeList() {
        auto blockCount = capacity_ / blockSize_;
        for (std::size_t i=0; i<blockCount; ++i) {
            freeList_.push(reinterpret_cast<List::Node*>((char*)buffer_ + i*blockSize_));
        }
    }

public:
    PoolAllocator(const std::size_t capacity, const std::size_t blockSize) 
        : BaseAllocator(capacity), blockSize_(blockSize), allocatedBlocks_(0)
    {
        assert((blockSize_ >= 8) && "Block size must be at least 8 bytes");
        assert((capacity % blockSize == 0) && "Capacity must be a multiple of block size");
        
        buffer_ = std::malloc(capacity_);
        if (!buffer_) {
            throw std::bad_alloc();
        }
        std::memset(buffer_, 0, capacity_);
        initFreeList();
        std::cout << "Constructed PoolAllocator with Capacity: " << capacity << " and Block Size: " << blockSize << std::endl;
    }

    ~PoolAllocator() override {
        std::free(buffer_);
        std::cout << "Destructed PoolAllocator" << std::endl;
    }

    PoolAllocator(const PoolAllocator&) = delete;

    PoolAllocator& operator=(const PoolAllocator&) = delete;

    void* allocate(const std::size_t size = 0, const std::size_t alignment = 0) override {
        assert((size <= blockSize_) && "Requested size exceeds block size");
        auto mem = freeList_.pop();
        if (!mem) {
            throw std::bad_alloc();
        }
        ++allocatedBlocks_;
        std::cout << "Allocated " << blockSize_ << " bytes at address: " << mem << std::endl;
        std::cout << "Available memory: " << getAvailable() << " bytes" << std::endl;
        return mem;
    }

    void deallocate(void* ptr) override {
        freeList_.push(static_cast<List::Node*>(ptr));
        --allocatedBlocks_;
        std::cout << "Deallocated memory at: " << ptr << std::endl;
        std::cout << "Available memory: " << getAvailable() << " bytes" << std::endl;
    }

    std::size_t getAvailable() const {
        return capacity_ - (allocatedBlocks_*blockSize_);
    }

    void printMemory() {
        hexdump(buffer_);
    }

};