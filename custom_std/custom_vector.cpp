#include <iostream>
#include <algorithm>
#include <utility>
#include <initializer_list>

namespace ajr {

    template <typename T, typename Allocator=std::allocator<T>>
    class vector {

        constexpr static int GROWTH_FACTOR = 2.0;

        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using allocator_traits = std::allocator_traits<Allocator>;

      public:
        class RandomAccessIterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

        private:
            pointer ptr;

        public:
            explicit RandomAccessIterator(pointer p = nullptr) : ptr(p) {}

            // Dereferencing
            reference operator*() const {
                return *ptr;
            }

            // Pointer access
            pointer operator->() const {
                return ptr;
            }

            // Pre-increment
            RandomAccessIterator& operator++() {
                ++ptr;
                return *this;
            }

            // Post-increment
            RandomAccessIterator operator++(int) {
                RandomAccessIterator temp = *this;
                ++(*this);
                return temp;
            }

            // Pre-decrement
            RandomAccessIterator& operator--() {
                --ptr;
                return *this;
            }

            // Post-decrement
            RandomAccessIterator operator--(int) {
                RandomAccessIterator temp = *this;
                --(*this);
                return temp;
            }

            // Arithmetic operations
            RandomAccessIterator operator+(difference_type n) const {
                return RandomAccessIterator(ptr + n);
            }

            RandomAccessIterator operator-(difference_type n) const {
                return RandomAccessIterator(ptr - n);
            }

            difference_type operator-(const RandomAccessIterator& other) const {
                return ptr - other.ptr;
            }

            RandomAccessIterator& operator+=(difference_type n) {
                ptr += n;
                return *this;
            }

            RandomAccessIterator& operator-=(difference_type n) {
                ptr -= n;
                return *this;
            }

            // Subscript operator
            reference operator[](difference_type n) const {
                return ptr[n];
            }

            // Comparison operators
            bool operator==(const RandomAccessIterator& other) const {
                return ptr == other.ptr;
            }

            bool operator!=(const RandomAccessIterator& other) const {
                return ptr != other.ptr;
            }

            bool operator<(const RandomAccessIterator& other) const {
                return ptr < other.ptr;
            }

            bool operator>(const RandomAccessIterator& other) const {
                return ptr > other.ptr;
            }

            bool operator<=(const RandomAccessIterator& other) const {
                return ptr <= other.ptr;
            }

            bool operator>=(const RandomAccessIterator& other) const {
                return ptr >= other.ptr;
            }
        };

        // Default ctor
        vector() : mStart_{nullptr}, mEnd_{nullptr}, mCapacity_{0} { std::cout << "default ctor" << std::endl; }

        // dtor
        ~vector() {
            dealloc();
        }

        // Custom ctors
        explicit vector(const Allocator& alloc) noexcept 
            : mStart_{nullptr}
            , mEnd_{nullptr}
            , mCapacity_{0}
            , mAllocator_{alloc} 
        {}

        vector(size_type count, const T& value, const Allocator& alloc = Allocator()) : mCapacity_{count}, mAllocator_{alloc} {
            mStart_ = allocator_traits::allocate(mAllocator_, mCapacity_);
            mEnd_ = mStart_;
            for (std::size_t idx=0; idx<mCapacity_; ++idx) {
                this->push_back(value);
            }
        }

        explicit vector(size_type count, const Allocator& alloc = Allocator()) : mCapacity_{count}, mAllocator_{alloc} {
            mStart_ = allocator_traits::allocate(mAllocator_, mCapacity_);
            mEnd_ = mStart_;
            for (std::size_t idx=0; idx<mCapacity_; ++idx) {
                this->push_back(0);
            }
        }

        template<typename InputIt>
        requires std::input_iterator<InputIt>
        vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : mStart_{nullptr}
            , mEnd_{nullptr}
            , mCapacity_{0}
            , mAllocator_{alloc}
        {
            for (auto it=first; it!=last; ++it) {
                this->push_back(*it);
            }
        }

        // Copy ctor
        vector(const vector& other) : mCapacity_{other.mCapacity_} {
            mStart_ = allocator_traits::allocate(mAllocator_, mCapacity_);
            mEnd_ = mStart_;
            auto sz = other.size();
            for (auto idx=0; idx<sz; ++idx) {
                this->push_back(other.at(idx));
            }
        }

        // Move ctor
        vector(vector&& other) : mStart_{other.mStart_}, mEnd_{other.mEnd_}, mCapacity_{other.mCapacity_} {
            other.mStart_=nullptr;
            other.mEnd_=nullptr;
            other.mCapacity_=0;
        }

        vector(std::initializer_list<T> init, const Allocator& alloc = Allocator()) : mCapacity_{init.size()}, mAllocator_{alloc} {
            mStart_ = allocator_traits::allocate(mAllocator_, mCapacity_);
            mEnd_ = mStart_;
            for (auto it=init.begin(); it!=init.end(); ++it) {
                this->push_back(*it);
            }
        }

        // copy-assignment
        vector& operator=(const vector& other) {
            if (this != &other) {
                dealloc();
            }
            mCapacity_ = other.mCapacity_;
            mStart_ = allocator_traits::allocate(mAllocator_, mCapacity_);
            mEnd_ = mStart_;
            auto sz=other.size();
            for (std::size_t idx=0; idx<sz; ++idx) {
                this->push_back(other.at(idx));
            }
            return *this;
        }

        // move assignment
        vector& operator=(vector&& other) {
            if (this != &other) {
                dealloc();
            }
            mStart_ = std::exchange(other.mStart_, nullptr);
            mEnd_ = std::exchange(other.mEnd_, nullptr);
            mCapacity_ = std::exchange(other.mCapacity_, 0);
            return *this;
        }
        
        Allocator get_allocator() const { return mAllocator_; }

        value_type& at(size_type pos) {
            if (pos >= size()) {
                throw std::out_of_range("index out of range");
            }
            return mStart_[pos];
        }

        const value_type& at(size_type pos) const {
            if (pos >= size()) {
                throw std::out_of_range("index out of range");
            }
            return mStart_[pos];
        }

        value_type& operator[](size_type pos) {
            return mStart_[pos];
        }

        const value_type& operator[](size_type pos) const {
            return mStart_[pos];
        }

        value_type& front() {
            return *mStart_;
        }

        value_type& back() {
            return *mEnd_;
        }

        value_type* data() {
            return mStart_;
        }

        RandomAccessIterator begin() {
            return RandomAccessIterator(mStart_);
        }

        const RandomAccessIterator begin() const {
            return RandomAccessIterator(mStart_);
        }

        RandomAccessIterator end() {
            return RandomAccessIterator(mEnd_);
        }

        const RandomAccessIterator end() const {
            return RandomAccessIterator(mEnd_);
        }

        void reserve(size_type new_cap) {
            // throw if new_cap > max_size that can be allocated
            if (new_cap > capacity()) {
                realloc(new_cap);
            }
        }

        bool empty() const {
            return (mStart_ == mEnd_);
        }

        size_type size() const {
            return (mEnd_ - mStart_);
        }

        size_type capacity() const {
            return mCapacity_;
        }

        void clear() {
            value_type* ptr = mStart_;
            while (ptr != mEnd_) {
                ptr->~T();
                ++ptr;
            }
            mEnd_=mStart_;
        }

        void insert() {
            // TODO
        }

        void erase() {
            // TODO
        }

        void push_back(const T& value) {
            if (size() == capacity()) {
                // reallocate new mem and destruct, dealloc the old memory
                realloc((capacity() == 0) ? 1:capacity()*GROWTH_FACTOR);
            }
            new (mEnd_) value_type(std::move(value));
            ++mEnd_;
        }

        template<typename T1>
        void push_back(T1&& value) {
            if (size() == capacity()) {
                // reallocate new mem and destruct, dealloc the old memory
                realloc((capacity() == 0) ? 1:capacity()*GROWTH_FACTOR);
            }
            new (mEnd_) value_type(std::forward<T1>(value));
            ++mEnd_;
        }

        template<typename... Args>
        T& emplace_back(Args&&... args) {
            if (size() == capacity()) {
                realloc((capacity() == 0) ? 1:capacity()*GROWTH_FACTOR);
            }
            new (mEnd_) value_type(std::forward<Args>(args)...);
            T* ele = mEnd_++;
            return *ele;
        }

        void pop_back() {
            T* end = (mStart_ + size()-1);
            end->~T();
            mEnd_ = end;
        }

      private:
        value_type* mStart_;
        value_type* mEnd_;
        size_type mCapacity_;
        Allocator mAllocator_;

        void realloc(size_type new_cap) {
            auto sz = size();
            value_type* newStart = allocator_traits::allocate(mAllocator_, new_cap);
            for (std::size_t idx=0; idx<sz; ++idx) {
                new (newStart+idx) value_type(std::move(at(idx)));
                (mStart_+idx)->~T();
            }
            allocator_traits::deallocate(mAllocator_, mStart_, mCapacity_);
            mCapacity_ = new_cap;
            mStart_ = newStart;
            mEnd_ = mStart_+sz;
            std::cout << "// Vector reallocated //\n";
        }

        void dealloc() {
            value_type* ptr = mStart_;
            while (ptr!=mEnd_) {
                ptr->~T();
                ++ptr;
            }
            allocator_traits::deallocate(mAllocator_, mStart_, capacity());
        }

    };

}

struct S {
    int a;
    int b;

    S() : a{0}, b{0} { std::cout << "Default ctor called!\n"; }

    S(int _a, int _b) : a{_a}, b{_b} { std::cout << "Parameterized constructor called!\n"; }

    S(const S& other) : a{other.a}, b{other.b} { std::cout << "Copy constructor called!\n"; }

    S(S&& other) noexcept : a{std::move(other.a)}, b{std::move(other.b)} {
        std::cout << "Move constructor called!\n";
    }

    S& operator=(const S& other) {
        std::cout << "Copy-assignment ctor called!\n";
        if (this != &other) {
            a = other.a;
            b = other.b;
        }
        return *this;
    }

    S& operator=(S&& other) noexcept {
        std::cout << "Move-assignment ctor called!\n";
        a = std::move(other.a);
        b = std::move(other.b);
        return *this;
    }

    ~S() { std::cout << "Dtor called!\n"; }
};

int main() {

    ajr::vector<int> v = {10, 9, 8, 7, 6, 5};
    std::for_each(v.begin(), v.end(), [](auto& val) { val *= 2; });

    std::cout << "Contents of v before sort are (using range-for): ";
    for (const int& a : v) {
        std::cout << a << " ";
    }
    std::cout << std::endl;

    std::sort(v.begin(), v.end());

    std::cout << "Contents of v after sort are (using range-for): ";
    for(const int& a : v) {
        std::cout << a << " ";
    }
    std::cout << std::endl;

    v.pop_back();
    std::cout << "Contents of v after pop_back (using range-for): ";
    for(const int& a : v) {
        std::cout << a << " ";
    }
    std::cout << std::endl;

    v.clear();
    std::cout << "Values after clearing v: size:" << v.size() << ", capacity:" << v.capacity() << std::endl;

    try {
        std::cout << "v[6] = " << v.at(6) << std::endl;
    } catch (std::out_of_range& e) {
        std::cout << "Caught out_of_range: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Generic exception caught!" << std::endl;
    }

    std::cout << "Creating v2 with length 5";
    ajr::vector<std::string> v2{3, "str"};
    std::cout << std::endl;
    for (int i = 0; i < 3; ++i) {
        v2.push_back(std::to_string(i * 100));
    }

    std::cout << "Copying into vs via v2 iterators\n";
    ajr::vector<std::string> vs{v2.begin(), v2.end()};
    vs[2] = "Two";
    vs[3] = std::string{"Three"};

    std::cout << "Contents of v2 are:" << std::endl;
    for (int i = 0; i < v2.size(); ++i) {
        std::cout << "v2[" << i << "] = " << v2[i] << std::endl;
    }

    std::cout << "Contents of vs are:" << std::endl;
    for (int i = 0; i < vs.size(); ++i) {
        std::cout << "vs[" << i << "] = " << vs[i] << std::endl;
    }

    std::cout << "Creating ajr::vector<S> v3 and calling emplace_back 5 times" << std::endl;
    ajr::vector<S> v3;
    for (int i = 0; i < 5; ++i) {
        v3.emplace_back(i*10, i*10+1);
    }

    std::cout << "Creating a new vector v4 and copy constructing it from v3" << std::endl;
    auto v4 = v3;
    std::cout << "Contents of v4 are:" << std::endl;
    for (int i = 0; i < v4.size(); ++i) {
        std::cout << "v4[" << i << "] = {" << v4[i].a << ", " << v4[i].b << "}" << std::endl;
    }

    std::cout << "Creating a new vector v5 and move constructing it from v3" << std::endl;
    auto v5 = std::move(v3);
    std::cout << "Contents of v5 are:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::cout << "v5[" << i << "] = {" << v5[i].a << ", " << v5[i].b << "}" << std::endl;
    }

    ajr::vector<ajr::vector<int>> vv;
    for (int i = 0; i < 10; ++i) {
        vv.push_back(ajr::vector<int>(10, i));
    }

    std::cout << "Contents of vv are: " << std::endl;
    for (int i = 0; i < vv.size(); ++i) {
        std::cout << "{ ";
        for (int j = 0; j < 10; ++j) {
            std::cout << vv[i][j] << ", ";
        }
        std::cout << "}" << std::endl;
    }

    std::cout << "Reserving 12 elements for vector<int>\n";
    ajr::vector<int> res;
    res.reserve(10);
    std::cout << "Values after reserve: size:" << res.size() << ",  capacity:" << res.capacity() << ", sizeof(vector<int>):" << sizeof(res) << std::endl;
    std::cout << "Pushing 10 values in the vector\n";
    for (int i=0; i<10; i++) {
        res.push_back(i);
    }
    std::cout << "Contents: [ ";
    for (int i=0; i<res.size(); ++i) {
        std::cout << res[i] << " ";
    }
    std::cout << "]" << std::endl;
    std::cout << "Inserting 11th element that should cause reallocation\n";
    res.push_back(10);
    std::cout << "Contents: [ ";
    for (int i=0; i<res.size(); ++i) {
        std::cout << res[i] << " ";
    }
    std::cout << "]" << std::endl;

    return 0;
}
