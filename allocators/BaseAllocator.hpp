#pragma once

#include <iostream>
#include <iomanip>
#include <cstdint>

class BaseAllocator {
protected:
    std::size_t capacity_;

public:
    BaseAllocator(std::size_t capacity = 0) : capacity_(capacity) {}
    
    virtual ~BaseAllocator() = default;
    
    BaseAllocator(const BaseAllocator&) = delete;

    BaseAllocator& operator=(const BaseAllocator&) = delete;

    virtual void* allocate(const std::size_t size, const std::size_t alignment = 0) = 0;

    virtual void deallocate(void* ptr) = 0;

    virtual void construct(void* ptr) = 0;

    virtual void destruct(void* ptr) = 0;

    void* getAlignedAddress(void* start, std::size_t alignment) {
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(start);
        return (void*)((addr + alignment - 1) & ~(alignment - 1));
    }

    inline void hexdump(const void* ptr) {
        const auto data = static_cast<const unsigned char*>(ptr);
        
        for (size_t i=0; i<capacity_; i+=16) {
            // Print address
            std::cout << std::hex << std::setw(16) << std::setfill('0') << reinterpret_cast<uintptr_t>(data+i) << "  ";
            
            // Print hex bytes
            for (size_t j=0; j<16; ++j) {
                if (i+j < capacity_) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i+j] << " ";
                } else {
                    std::cout << "   ";  // Padding for alignment
                }
            }
            std::cout << " ";
            // Print ASCII characters
            for (size_t j=0; j<16; ++j) {
                if (i+j < capacity_) {
                    char c = data[i+j];
                    std::cout << (std::isprint(c) ? c : '.');
                }
            }
            std::cout << std::endl;
        }
    }
};