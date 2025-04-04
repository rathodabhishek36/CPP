// File: main.cpp

#include "LinearAllocator.hpp"
#include "StackAllocator.hpp"
#include "PoolAllocator.hpp"
#include "Mallocator.hpp"

int main() {

    {
        LinearAllocator<100> allocator;

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
    }
    std::cout << std::endl;
    {
        StackAllocator<100> allocator;

        auto ptr1 = static_cast<int*>(allocator.allocate(sizeof(int)*4, std::alignment_of<int>::value));
        auto ptr2 = static_cast<char*>(allocator.allocate(sizeof(char)*5, std::alignment_of<char>::value));
        auto ptr3 = static_cast<double*>(allocator.allocate(sizeof(double)*3, std::alignment_of<double>::value));

        ptr1[0] = 1000;
        ptr1[1] = 2;
        ptr1[2] = 3;
        ptr1[3] = 4;

        ptr2[0] = 'a';
        ptr2[1] = 'b';
        ptr2[2] = 'c';
        ptr2[3] = 'd';
        ptr2[4] = 'e';

        ptr3[0] = 1.0;
        ptr3[1] = 2.5;
        ptr3[2] = 3.25;

        allocator.printMemory();
    }
    std::cout << std::endl;
    {
        PoolAllocator allocator (1024, 64);

        auto ptr1 = static_cast<char*>(allocator.allocate());
        auto ptr2 = static_cast<int*>(allocator.allocate());

        ptr1[0] = 'a';
        ptr1[1] = 'b';
        ptr1[2] = 'c';
        ptr1[3] = 'd';
        ptr1[4] = 'e';

        ptr2[0] = 1000;
        ptr2[1] = 2;
        ptr2[2] = 3;
        ptr2[3] = 4;

        allocator.deallocate(ptr2);

        auto ptr3 = static_cast<char*>(allocator.allocate());

        ptr3[0] = 'f';
        ptr3[1] = 'g';
        ptr3[2] = 'h';
        ptr3[3] = 'i';
        ptr3[4] = 'j';

        allocator.printMemory();

    }
    std::cout << std::endl;

    return 0;
}
