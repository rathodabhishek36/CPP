#include <iostream>
#include <thread>
#include <atomic>
#include <barrier>
#include <vector>
#include <mutex>
#include <future>
#include <condition_variable>

// Exercise 1:
std::atomic<int> GLOBAL_COUNTER = 0;

void increment() {
    for (int i = 0; i < 16 * 5000; ++i) {
        int old_value = GLOBAL_COUNTER.load(std::memory_order_relaxed);
        int new_value;
        do {
            new_value = (old_value + 1) % 16;
        } while (!GLOBAL_COUNTER.compare_exchange_weak(old_value, new_value, std::memory_order_relaxed));
    }
}

// Exercise 2:
std::atomic_bool print_even = true;
std::condition_variable cv;
std::mutex mtx;

void printEven() {
    while (GLOBAL_COUNTER < 100) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return GLOBAL_COUNTER % 2 == 0; });
        std::cout << GLOBAL_COUNTER << " ";
        GLOBAL_COUNTER++;
        cv.notify_all();
    }
}

void printOdd() {
    while (GLOBAL_COUNTER < 100) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return GLOBAL_COUNTER % 2 == 1; });
        std::cout << GLOBAL_COUNTER << " ";
        GLOBAL_COUNTER++;
        cv.notify_all();
    }
}

// Exercise 3:
class A {
    int noThreads_;
    std::vector<std::thread> threads_;
    std::barrier<void(*)()> barrier_;

public:
    A(int n) : noThreads_(n), barrier_(n, [](){}) {
        threads_.reserve(noThreads_);
        std::cout << "A constructor" << std::endl;
    }

    ~A() { 
        for(auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        threads_.clear();
        std::cout << "A destructor" << std::endl; 
    }

    void doSomething() {
        std::cout << "Thread " << std::this_thread::get_id() << " is waiting" << std::endl;
        barrier_.arrive_and_wait();
        std::cout << "Doing something in A" << std::endl;
    }

    void start() {
        for (int i=0; i<noThreads_; ++i) {
            threads_.emplace_back(&A::doSomething, this);
        }
    }

};

int main() {

    // Exercise 1:
    static_assert(std::atomic<int>::is_always_lock_free, "std::atomic<int> should be atomic");

    for (int i=0; i<100; ++i) {
        GLOBAL_COUNTER = 0; // Reset the counter for each iteration
        {
            auto t1 = std::jthread(increment);
            auto t2 = std::jthread(increment);
        }
    
        std::cout << "Final counter value: " << GLOBAL_COUNTER << std::endl;
    
    }

    // Exercise 2:
    {
        auto t1 = std::jthread(printEven);
        auto t2 = std::jthread(printOdd);
    }
    std::cout << std::endl;

    // Exercise 3:
    int noThreads = 2;
    A a(noThreads);
    a.start();

    // compute sum of sqares upto a certain number using std::async
    auto sum_of_squares = [](int n) {
        std::cout << "Calculating sum of squares..." << std::endl;
        int sum = 0;
        for (int i = 1; i <= n; ++i) {
            sum += i * i;
        }
        return sum;
    };
    auto future = std::async(std::launch::async, sum_of_squares, 100);
    // Do other work while waiting for the result
    std::cout << "Doing other work..." << std::endl;
    // Wait for the result
    std::cout << "Sum of squares: " << future.get() << std::endl;

    return 0;
}