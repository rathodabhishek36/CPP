#include <iostream>
#include <atomic>
#include <unistd.h>
#include <cassert>

template<typename T, typename Allocator=std::allocator<T>>
class SPSCQueue {
    using pointer = T*;
    using AllocTraits = std::allocator_traits<Allocator>;

    #ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
    #else
    static constexpr size_t CACHE_LINE_SIZE = 64;
    #endif

    // Padding to apply before and after data_ to avoid false sharing.
    static constexpr size_t CACHE_LINE_PADDING = (CACHE_LINE_SIZE-1) / sizeof(T) +1;
    
    std::size_t capacity_;
    pointer data_;
    Allocator allocator_;

    alignas(CACHE_LINE_SIZE) std::atomic<size_t> readPos_ {0};
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> writePos_ {0};

    static void assert_with_message(bool cond, std::string message) {
        if (!cond) {
            std::cerr << "Assertion failed : " << message << std::endl;
            assert(false);
        }
    }

    void sanity_check() {
        static_assert(alignof(SPSCQueue<T>) == CACHE_LINE_SIZE, "Alignment of SPSCQueue should be equal to cache line size.");
        assert_with_message(CACHE_LINE_SIZE == sysconf(_SC_LEVEL1_DCACHE_LINESIZE), "L1 Cache line size is not 64 bytes.");
        assert_with_message(capacity_ > 0, "Capacity should be greater than 0.");
        assert_with_message(reinterpret_cast<char*>(&readPos_) - reinterpret_cast<char*>(&writePos_) >= CACHE_LINE_SIZE
                            , "readPos_ and writePos_ should be on different cache lines.");
    }

public:
    SPSCQueue(const std::size_t capacity=100'000, const Allocator& allocator = Allocator())
        : allocator_(allocator)
        , capacity_(capacity)
    {
        sanity_check();
        // One extra element to differentiate between full and empty. 
        // If readPos_ == writePos_ then queue is empty.
        // If readPos_ == writePos_+1 then queue is full.
        ++capacity_;
        
        if (capacity_ > SIZE_MAX - 2*CACHE_LINE_PADDING) {
            capacity_ = SIZE_MAX - 2*CACHE_LINE_PADDING;
        }

        data_ = AllocTraits::allocate(allocator_, CACHE_LINE_PADDING + capacity_ + CACHE_LINE_PADDING);
    }

    ~SPSCQueue() {
        while (front()) {
            pop();
        }
        AllocTraits::deallocate(allocator_, data_, CACHE_LINE_PADDING + capacity_ + CACHE_LINE_PADDING);
    }

    // Non-Copyable and Non-Movable.
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    bool empty() const {
        return readPos_.load(std::memory_order_acquire) == writePos_.load(std::memory_order_acquire);
    }

    // Blocking call.
    template<typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
        static_assert(std::is_constructible_v<T, Args...>, "T should be constructible with Args...");
        const auto currentWritePos = writePos_.load(std::memory_order_relaxed);
        auto nextWritePos = currentWritePos + 1;
        if (nextWritePos == capacity_) {
            nextWritePos = 0;
        }
        while (nextWritePos == readPos_.load(std::memory_order_acquire)) {
            // keep on trying until there is space in the queue to write.
        }
        new (&data_[CACHE_LINE_PADDING + currentWritePos]) T(std::forward<Args>(args)...);
        writePos_.store(nextWritePos, std::memory_order_release);
    }

    // Non-Blocking call.
    template<typename... Args>
    [[ nodiscard ]] bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
        static_assert(std::is_constructible_v<T, Args...>, "T should be constructible with Args...");
        const auto currentWritePos = writePos_.load(std::memory_order_relaxed);
        auto nextWritePos = currentWritePos + 1;
        if (nextWritePos == capacity_) {
            nextWritePos = 0;
        }
        if (nextWritePos == readPos_.load(std::memory_order_acquire)) {
            return false;
        }
        new (&data_[CACHE_LINE_PADDING + currentWritePos]) T(std::forward<Args>(args)...);
        writePos_.store(nextWritePos, std::memory_order_release);
        return true;
    }

    void push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        static_assert(std::is_copy_constructible_v<T>, "T should be copy constructible.");
        emplace(value);
    }

    template<typename U>
    std::enable_if_t<std::is_constructible_v<T, U>, void>
    push(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>) {
        emplace(std::forward<U>(value));
    }

    [[ nodiscard ]] bool try_push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        static_assert(std::is_copy_constructible_v<T>, "T should be copy constructible.");
        return try_emplace(value);
    }

    template<typename U>
    std::enable_if_t<std::is_constructible_v<T, U>, bool>
    try_push(T&& value) noexcept(std::is_nothrow_constructible_v<T, U>) {
        return try_emplace(std::forward<U>(value));
    }

    void pop() noexcept {
        static_assert(std::is_nothrow_destructible_v<T>, "T should be nothrow destructible.");
        if (!empty()) {
            const auto currrentReadPos = readPos_.load(std::memory_order_relaxed);
            data_[CACHE_LINE_PADDING + currrentReadPos].~T();
            auto nextReadPos = currrentReadPos + 1;
            if (nextReadPos == capacity_) {
                nextReadPos = 0;
            }
            readPos_.store(nextReadPos, std::memory_order_release);
        }
    }

    [[ nodiscard ]] pointer front() noexcept {
        const auto currentReadPos = readPos_.load(std::memory_order_relaxed);
        if (currentReadPos == writePos_.load(std::memory_order_acquire)) {
            return nullptr;
        }
        return &data_[CACHE_LINE_PADDING + currentReadPos];
    }

    [[ nodiscard ]] size_t capacity() const noexcept {
        return capacity_-1;
    }

    [[ nodiscard ]] size_t size() const noexcept {
        std::ptrdiff_t size = writePos_.load(std::memory_order_acquire) 
                              - readPos_.load(std::memory_order_acquire);
        if (size < 0) {
            size += capacity_;
        }
        return static_cast<size_t>(size);
    }

};

int main() {
    {
        SPSCQueue<int> queue(10);
        std::cout << alignof(SPSCQueue<float>) << std::endl;
    }
    
    return 0;
}
