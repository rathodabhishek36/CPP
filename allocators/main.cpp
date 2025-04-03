// File: main.cpp

#include "LinearAllocator.hpp"

int main() {

    LinearAllocator allocator(100);

    auto ptr1 = static_cast<int*>(allocator.allocate(sizeof(int)*4, std::alignment_of<int>::value));
    auto ptr2 = static_cast<double*>(allocator.allocate(sizeof(double)*4, std::alignment_of<double>::value));

    ptr1[0] = 1000;
    ptr1[1] = 2;
    ptr1[2] = 3;
    ptr1[3] = 4;

    ptr2[0] = 2.0;
    ptr2[1] = 2.0;
    ptr2[2] = 3.0;
    ptr2[3] = 4.0;

    // Prints memory content
    // Note that the memory is laid out in little endian format
    allocator.printMemory(); 

    return 0;
}
