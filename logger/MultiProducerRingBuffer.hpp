#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>


template<typename T>
class MultiProducerSingleConsumerRingBuffer {

    #ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
    #else
    static constexpr size_t CACHE_LINE_SIZE = 64;
    #endif
    
    std::vector<T> buffer_;
    std::mutex writerLock_;
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> head_;
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail_;

public:
    MultiProducerSingleConsumerRingBuffer(std::size_t size) : buffer_(size), head_(0), tail_(0) {
        static_assert(CACHE_LINE_SIZE == 64, "L1 Cache line size is not 64 bytes.");
    }

    bool try_push(const T& data) {
        std::scoped_lock guard (writerLock_);

        auto current_head = head_.load(std::memory_order_relaxed);
        auto next_head = (current_head + 1) % buffer_.size();
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer is full
        }
        buffer_[current_head] = data;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        auto current_tail = tail_.load(std::memory_order_relaxed);
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt; // Buffer is empty
        }
        auto data = buffer_[current_tail];
        auto next_tail = (current_tail + 1) % buffer_.size();
        tail_.store(next_tail, std::memory_order_release);
        return data;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    std::size_t size() const {
        auto current_head = head_.load(std::memory_order_acquire);
        auto current_tail = tail_.load(std::memory_order_acquire);
        if (current_head >= current_tail) {
            return current_head - current_tail;
        }
        return buffer_.size() - current_tail + current_head;
    }

};