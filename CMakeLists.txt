cmake_minimum_required(VERSION 3.10)

# Set the project name
project(CPP)

#Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set Output build directory
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# Set output directory for all the executables
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/exe)
set(CMAKE_BUILD_TYPE release)

# Add executables
add_executable(array custom_std/custom_std_array.cpp)
add_executable(vector custom_std/custom_vector.cpp)
add_executable(shared_ptr custom_std/custom_shared_ptr.cpp)
add_executable(unique_ptr custom_std/custom_unique_ptr.cpp)

add_executable(spsc_queue concurrency/spsc_queue.cpp)

add_executable(fibonacci metaprogramming/compile_time_fibonacci.cpp)

add_executable(mmap_file mmap/mmapfile.cpp)
add_executable(double_mapper_ringbuffer mmap/doubleMappedRingBuffer.cpp)
add_executable(price_time_order_book trading/price_time_order_book.cpp)