// Type your code here, or load an example.
#include <iostream>
#include <cstdint>

using namespace std;

template<int n>
struct fibonacci {
    static constexpr int value = fibonacci<n-1>::value + fibonacci<n-2>::value;
};

template<>
struct fibonacci<1> {
    static constexpr int value = 1;
};

template<>
struct fibonacci<0> {
    static constexpr int value = 0;
};


int main() {
    constexpr int n = fibonacci<6>::value;
    static_assert(n==8, "assert faield");

}
