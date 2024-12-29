#include <iostream>
#include <algorithm>
#include <iterator>

namespace ajr {

    template<typename T, std::size_t N=0>
    struct array {
        typedef T value_type;
        typedef T& reference_type;
        typedef const T& const_reference_type;
        typedef T* pointer_type;
        typedef const T* const_pointer_type;
        typedef std::size_t size_type;
        typedef T* iterator_type;
        typedef const T* const_iterator_type;

        T mData[N]; // data mem is public in std::array as well, but shouldn't be used directly in users code ofc.
        // doesn't need ctors and dtors as data is stack-allocted

        reference_type at(std::size_t pos) {
            if (pos >=0 && pos < N) {
                return mData[pos];
            }
            throw std::out_of_range("index out of bounds");
        }

        const_reference_type at(std::size_t pos) const {
            if (pos >=0 && pos < N) {
                return mData[pos];
            }
            throw std::out_of_range("index out of bounds");
        }

        constexpr reference_type operator[] (std::size_t pos) {
            return mData[pos];
        }

        constexpr const_reference_type operator[] (std::size_t pos) const {
            return mData[pos];
        }

        constexpr reference_type front() {
            return *mData;
        }

        constexpr const_reference_type front() const {
            return *mData;
        }

        constexpr reference_type back() {
            return *(mData+N);
        }

        constexpr const_reference_type back() const {
            return *(mData+N);
        }

        constexpr pointer_type data() {
            return mData;
        }

        constexpr const_pointer_type data() const {
            return mData;
        }

        constexpr iterator_type begin() {
            return mData;
        }

        constexpr const_iterator_type begin() const {
            return mData;
        }

        constexpr iterator_type end() {
            return mData+N;
        }

        constexpr const_iterator_type end() const {
            return mData+N;
        }

        constexpr const_iterator_type cbegin() const {
            return mData;
        }

        constexpr const_iterator_type cend() const {
            return mData+N;
        }

        //TODO : Didn't implement reverse iterators

        constexpr bool empty() const {
            return begin()==end();
        }

        constexpr std::size_t size() const {
            return N;
        }

        constexpr std::size_t max_size() const {
            return std::distance(begin(), end()); // eqauls N for array
        }

        void fill(const_reference_type value) {
            std::fill(this.begin(), this.end(), value);
            // std::size_t idx=0;
            // while(idx<N) {
            //     mData[idx]=value;
            //     idx++;
            // }
        }

        void swap(array<T,N>& other) {
            std::size_t idx=0;
            while(idx<N) {
                std::swap(mData[idx], other[idx]);
                // std::swap does the same thing as below for most cases
                // in cases, where mData holds containers, it will call swap_ranges with the iterators of the data, which will
                // then in turn run a for loop that runs through the container elements and applies the below logic for 
                // the elements
                // auto tmp = std::move(mData[idx]);
                // mData[idx] = std::move(other[idx]);
                // other[idx] = std::move(tmp);
                idx++;
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const array<T,N>& arr) {
            os << "{ ";
            for (const auto& ele : arr) {
                os << ele << " ";
            }
            os << "}";
            return os;
        }
    };

}

int main() {

    ajr::array<int, 3> a1{2, 1, 3};

    std::cout << "\nContainer operations supported : (sorting)\n";
    std::cout << "  Before sorting : " << a1 << std::endl;
    std::sort(a1.begin(), a1.end());
    std::cout << "  After sorting : " << a1 << std::endl;
    
    try {
        std::cout << "\nIndex Out-Of-Bounds checking : \n";
        ajr::array<int, 3> a2 = {1, 2, 3};
        std::cout << a2.at(3) << std::endl;
    } catch (std::exception& e) {
        std::cout << "Caught exception : " << e.what() << std::endl;
    }
    
    {
        std::cout << "\nRange based for-loop : \n";
        ajr::array<std::string, 2> a3{"E", "\u018E"};
        for (const auto& s : a3) {
            std::cout << s << ' ';
        }
        std::cout << '\n';
    }

    {
        std::cout << "\nSwapping : \n";
        ajr::array<int, 3> a1{1, 2, 3}, a2{4, 5, 6};
 
        auto it1 = a1.begin();
        auto it2 = a2.begin();
        int& ref1 = a1[1];
        int& ref2 = a2[1];
 
        std::cout << a1 << ' ' << *it1 << ' ' << *it2 << ' ' << ref1 << ' ' << ref2 << '\n';
        a1.swap(a2);
        std::cout << a2 << ' ' << *it1 << ' ' << *it2 << ' ' << ref1 << ' ' << ref2 << '\n';
    }
    return 0;
}
