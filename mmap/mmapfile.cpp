#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cerrno>
#include <system_error>
#include <fstream>
#include <string.h>

/*
Some helpful man links : 
    https://man7.org/linux/man-pages/man2/open.2.html
    https://man7.org/linux/man-pages/man3/ftruncate.3p.html
    https://man7.org/linux/man-pages/man2/access.2.html
    https://man7.org/linux/man-pages/man2/mmap.2.html
    https://www.man7.org/linux/man-pages/man3/munmap.3p.html
    https://man7.org/linux/man-pages/man2/close.2.html
    https://man7.org/linux/man-pages/man2/msync.2.html
*/

template<typename T>
class mmapfile {
    using value_type = std::remove_extent_t<T>;
    using pointer_type = value_type*;
    using const_pointer_type = const pointer_type;
    using reference_type = value_type&;
    using const_reference_type = const reference_type;

    int fd_;
    size_t length_;
    size_t offset_;
    pointer_type base_;

    static bool fileExists(const char* file) {
        return (::access(file, F_OK) == 0);
    }

    template<typename T1, typename T2>
    T1 throw_if_equal(T1 t1, T2 t2) {
        if (t1 == t2) {
            throw std::system_error(errno, std::system_category());
        }
        return t1;
    }

    template<typename T1, typename T2>
    T1 throw_if_not_equal(T1 t1, T2 t2) {
        if (t1 != t2) {
            throw std::system_error(errno, std::system_category());
        }
        return t1;
    }

public:
    mmapfile() : fd_(-1), length_(0), offset_(0), base_(nullptr) {}

    mmapfile(const std::string& name, size_t length=sizeof(T), size_t offset=0)
        : fd_(-1), length_(length), offset_(offset), base_(nullptr)
    {
        const char* filename = name.c_str();

        int flags = O_CREAT | O_RDWR;
        if (fileExists(filename)) {
            std::cout << "File with name : " << filename << " already exists" << std::endl;
            flags = O_RDWR;
        }

        int mode = S_IRWXU;
        fd_ = throw_if_equal(::open(filename, flags, mode), -1);

        throw_if_not_equal(::ftruncate(fd_, sizeof(T)), 0);
        
        int prot = PROT_READ | PROT_WRITE;
        flags = MAP_SHARED;
        base_ = reinterpret_cast<pointer_type>(
            throw_if_equal(::mmap(NULL, length_, prot, flags, fd_, offset_), MAP_FAILED)
        );
        std::cout << "Successfully m-mapped file with name : " << filename
                  << " and size : " << length_ << " bytes" 
                  << ", with base virtual address : " << base_
                  << std::endl;
    }

    mmapfile(const mmapfile& file) = delete;

    mmapfile(mmapfile&& other) : mmapfile() {
        std::swap(*this, other);
    }

    ~mmapfile() noexcept {
        if (base_) {
            std::cout << "m-unmapping file with base virtual address : " << base_ << std::endl;
            ::munmap(base_, sizeof(T));
        }
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    reference_type operator*() {
        return *base_;
    }

    const_reference_type operator*() const {
        return *base_;
    }

    pointer_type operator->() {
        return base_;
    }

    const_pointer_type operator->() const {
        return base_;
    }

    reference_type operator[](size_t index) {
        return base_[index];
    }

    const_reference_type operator[](size_t index) const {
        return base_[index];
    }

    int fd() const {
        return fd_;
    }

    bool isValid() const {
        return (fd_ != -1);
    }

    void sync(size_t length=0, size_t offset=0) {
        if (length == 0) {
            length = sizeof(T) - offset;
        }
        ::msync(reinterpret_cast<unsigned char*>(base_)+offset, length, MS_SYNC);
    }
};

void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream out{filename, std::ios::binary};
    out.write(content.c_str(), content.size());
    out.close();
}

struct test {
    int a;
    char buff[1024];
};

int main() {

    mmapfile<test> testmmap_ {"testfile"};
    
    std::cout << "Initial state : " << std::endl;
    std::cout << "a : " << testmmap_->a << ",  buffer : " << testmmap_->buff << std::endl;
    
    testmmap_->a = 36;
    strcpy(testmmap_->buff, "My name is Abhishek Rathod");
    std::cout << "Modified values to : " << std::endl;
    std::cout << "a : " << testmmap_->a << ",  buffer : " << testmmap_->buff << std::endl;
    
    auto data = (::mmap(NULL, sizeof(test), PROT_READ | PROT_WRITE, MAP_SHARED, testmmap_.fd(), 0));
    std::cout << "Virtual address of new mmap : " << data << std::endl;

    auto disk_test = reinterpret_cast<test*>(data);
    std::cout << "After reading the values from the file again : " << std::endl;
    std::cout << "a : " << disk_test->a << ", buffer : " << disk_test->buff << std::endl;
    
    ::munmap(data, sizeof(test));
    
    return 0;
}