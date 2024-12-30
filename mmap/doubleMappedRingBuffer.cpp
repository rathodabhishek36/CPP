#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <string>
#include <optional>

// Note : A good solution to the problem wherein there is an API which asks for a 
// continuous buffer space to populate some data. For example, boost socket async_read.

/*
Some helpful man links :
    https://man7.org/linux/man-pages/man3/shm_open.3.html
    https://man7.org/linux/man-pages/man3/shm_unlink.3p.html
    https://man7.org/linux/man-pages/man3/ftruncate.3p.html
    https://man7.org/linux/man-pages/man2/mmap.2.html
    https://www.man7.org/linux/man-pages/man3/munmap.3p.html
    https://man7.org/linux/man-pages/man2/close.2.html
*/

template<size_t PAGESIZE>
class DoublyMappedRingBuffer {
    char* buffer_;
    size_t readPos_;
    size_t writePos_;

    template<typename T1, typename T2>
    static T1 throw_if_equal(T1 t1, T2 t2) {
        if (t1 == t2) {
            throw std::system_error(errno, std::system_category());
        }
        return t1;
    }

    static void* doubleMmap(int fd) {
        int prot = PROT_READ | PROT_WRITE;
        int flag = MAP_SHARED;
        char* addr1 = throw_if_equal((char*)::mmap64(NULL, 2*PAGESIZE, prot, flag, fd, 0), MAP_FAILED);
        throw_if_equal(::munmap(addr1 + PAGESIZE, PAGESIZE), -1);
        char* addr2 = throw_if_equal((char*)::mmap64(addr1+PAGESIZE, PAGESIZE, prot, flag, fd, 0), MAP_FAILED);
        assert(addr2 == (addr1 + PAGESIZE));
        return addr1;
    }

    void allocate() {
        char shm_name[64];
        snprintf(shm_name, sizeof(shm_name), "/DoublyMappedRingBuffer.%d", getpid());
        
        int flags = O_CREAT | O_RDWR | O_TRUNC;
        int mode = S_IRWXU | S_IRGRP;
        int fd = throw_if_equal(::shm_open(shm_name, flags, mode), -1);
        throw_if_equal(::shm_unlink(shm_name), -1);
        throw_if_equal(::ftruncate64(fd, PAGESIZE), -1);
        
        buffer_ = reinterpret_cast<char*>(doubleMmap(fd));
        
        ::close(fd);

        memset(buffer_, 0, PAGESIZE);
        std::cout << "Allocated " << PAGESIZE << " bytes at " << (void*)buffer_ << std::endl;
    }
    
    void reset() {
        readPos_ = 0;
        writePos_ = 0;
    }

    static size_t mask(size_t value) {
        constexpr static auto maskVal = PAGESIZE-1;
        return (value & maskVal);
    }

    static void assert_with_message(bool cond, std::string message) {
        if (!cond) {
            std::cerr << "Assertion failed : " << message << std::endl;
            assert(false);
        }
    }

public:
    DoublyMappedRingBuffer() : buffer_(nullptr), readPos_(0), writePos_(0) {
        static_assert((PAGESIZE & (PAGESIZE-1)) == 0, "PAGESIZE is not a power of 2");
        assert_with_message(PAGESIZE % sysconf(_SC_PAGE_SIZE) == 0,"PAGESIZE should be a multiple of OS supported page size.");

        try {
            allocate();
        } catch (const std::system_error& err) {
            std::cerr << "Caught error with code: " << err.code() << ", what: " << err.what() << std::endl;
        }
    }

    DoublyMappedRingBuffer(const DoublyMappedRingBuffer& other) = delete;

    DoublyMappedRingBuffer& operator=(const DoublyMappedRingBuffer& other) = delete;

    ~DoublyMappedRingBuffer() {
        if (buffer_) {
            std::cout << "Un-Mapping memory at : " << (void*)buffer_ << std::endl;
            ::munmap(buffer_, 2*PAGESIZE);
            buffer_ = nullptr;
        }
        reset();
    }

    bool produce(const char* data, size_t length) {
        if (full() || (free() < length)) {
            std::cout << "Not enough space to write, Available space : " << free() << std::endl;
            return false;
        }
        memcpy(wPtr(), data, length);
        std::cout << "Written " << length << " bytes." << std::endl;
        writePos_+=length;
        return true;
    }

    char* wPtr() {
        return buffer_ + mask(writePos_);
    }

    const char* rPtr() const {
        return buffer_ + mask(readPos_);
    }

    const size_t wPos() const {
        return writePos_;
    }

    const size_t rPos() const {
        return readPos_;
    }

    size_t free() {
        return PAGESIZE - used();
    }

    size_t used() {
        return (writePos_ - readPos_);
    }

    std::optional<std::string> consume(size_t length=PAGESIZE) {
        if (empty()) {
            return std::nullopt;
        }
        std::string ret;
        auto availableLen = std::min(length, used());
        std::cout << "Reading " << availableLen << " bytes" << std::endl;
        ret.assign(rPtr(), availableLen);
        readPos_ += availableLen;
        return ret;
    }

    bool empty() {
        return (readPos_ == writePos_);
    }

    bool full() {
        return (PAGESIZE == (writePos_-readPos_));
    }

};

int main() {
    
    DoublyMappedRingBuffer<4096> ringBuffer;
    // TODO - write tests.
    
    return 0;
}