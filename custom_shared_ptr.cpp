// custom shared ptr

#include <iostream>
#include <memory>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

struct control_block_base {
    ~control_block_base() {};

    virtual void incr_count() = 0;
    virtual void decr_count() = 0;
    virtual long use_count() const = 0;
};

template<typename T>
struct control_block : control_block_base {

    using pointer_type = T*;

    control_block(pointer_type p): mptr_{p} {}

    ~control_block() {};

    void incr_count() override { ++ref_count; }
    void decr_count() override { 
        --ref_count;
        if (ref_count == 0) {
            delete mptr_;
        }
    }
    long use_count() const override { return ref_count; }

  private:
    std::atomic<long> ref_count{1};
    pointer_type mptr_;
};

template<typename T>
struct custom_shared_ptr {

    template<typename T1>
    friend class custom_shared_ptr;

    using element_type = T;
    using pointer_type = T*;
    using reference_type = T&;

    custom_shared_ptr() : mptr_{}, mcbptr_{} {}

    custom_shared_ptr(std::nullptr_t) : mptr_{}, mcbptr_{} {}

    template<typename T1>
    custom_shared_ptr(T1* p) noexcept : mptr_{p}, mcbptr_{new control_block<T1>{p}} {}

    custom_shared_ptr(pointer_type p) noexcept : mptr_{p}, mcbptr_{new control_block<T>(p)} {
        p=nullptr;
    }

    custom_shared_ptr(const custom_shared_ptr& p) noexcept :
        mptr_{p.mptr_},
        mcbptr_{p.mcbptr_}
    {
        if (mcbptr_) { mcbptr_->incr_count(); }
    }

    template<typename T1>
    custom_shared_ptr(const custom_shared_ptr<T1>& p) noexcept : 
        mptr_{p.mptr_},
        mcbptr_{p.mcbptr_}
    {
        if (mcbptr_) { mcbptr_->incr_count(); }
    }

    custom_shared_ptr(custom_shared_ptr&& p) noexcept :
        mptr_{std::move(p.mptr_)},
        mcbptr_{std::move(p.mcbptr_)}
    {
        p.mptr_=nullptr;
        p.mcbptr_=nullptr;
    }

    template<typename T1>
    custom_shared_ptr(custom_shared_ptr<T1>&& p) noexcept :
        mptr_{std::move(p.mptr_)},
        mcbptr_(p.mcbptr_)
    {
        p.mptr_=nullptr;
        p.mcbptr_=nullptr;
    }

    template<typename T1>
    custom_shared_ptr(std::unique_ptr<T1>&& p) : custom_shared_ptr<T>(p.release()) {}

    ~custom_shared_ptr() {
        if (mcbptr_) {
            mcbptr_->decr_count();
        }
    }

    custom_shared_ptr& operator=(const custom_shared_ptr& p) {
        mptr_=p.mptr_;
        mcbptr_=p.mcbptr_;
        if (mcbptr_) {
            mcbptr_->incr_count();
        }
        return *this;
    }

    template<typename T1>
    custom_shared_ptr& operator=(const custom_shared_ptr<T1>& p) {
        mptr_=p.mptr_;
        mcbptr_=p.mcbptr_;
        if (mcbptr_) {
            mcbptr_->incr_count();
        }
        return *this;
    }

    custom_shared_ptr& operator=(custom_shared_ptr&& p) {
        mptr_=std::move(p.mptr_);
        mcbptr_=std::move(p.mcbptr_);
        p.reset();
        return *this;
    }

    template<typename T1>
    custom_shared_ptr& operator=(custom_shared_ptr<T1>&& p) {
        mptr_=std::move(p.mptr_);
        mcbptr_=std::move(p.mcbptr_);
        p.reset();
        return *this;
    }

    void swap(custom_shared_ptr& p) {
        using std::swap;
        swap(p.mptr_, mptr_);
        swap(p.mcbptr_, mcbptr_);
    }

    void reset() {
        custom_shared_ptr{}.swap(*this);
    }

    template<typename Y>
    void reset(Y* p) {
        custom_shared_ptr{p}.swap(*this);
    }

    long use_count() const {
        return mcbptr_ ? mcbptr_->use_count() : 0;
    }

    pointer_type get() const {
        return mptr_;
    }

    reference_type operator*() {
        return *mptr_;
    }

    pointer_type operator->() {
        return mptr_;
    }

    operator bool() {
        return get()!=nullptr;
    }

  private:
    pointer_type mptr_;
    control_block_base* mcbptr_;
};

template<typename T, typename... Args>
inline custom_shared_ptr<T> custom_make_shared(Args&&... args) {
    return custom_shared_ptr<T>(new T(std::forward<Args>(args)...));
}

//////////////////////////////////////////////////////////////

using namespace std::chrono_literals;

struct Base {
    Base() { std::cout << "Base::Base()\n"; }
    ~Base() { std::cout << "Base::~Base()\n"; }
    virtual void bar() { std::cout << "Base::bar" << std::endl; }
};

struct Derived : public Base {
    Derived() { std::cout << "Derived::Derived()\n"; }
    ~Derived() { std::cout << "Derived::~Derived()\n"; }
    void bar() override { std::cout << "Derived::bar" << std::endl; }
};

void print(const char* rem, custom_shared_ptr<Base> const& sp) {
    std::cout << rem << "\n\tget() = " << sp.get()
              << ", use_count() = " << sp.use_count() << '\n';
}

void thr(custom_shared_ptr<Base> p) {
    std::this_thread::sleep_for(987ms);
    custom_shared_ptr<Base> lp = p; // thread-safe, even though the
                                  // shared use_count is incremented
    {
        static std::mutex io_mutex;
        std::lock_guard<std::mutex> lk(io_mutex);
        print("Local pointer in a thread:", lp);
    }
}

int main() {

    std::cout << "===============shared_ptr demo===============" << std::endl;

    std::cout << "\n1) Shared ownership semantics demo\n";
    {
        auto sp = custom_make_shared<Derived>(); // sp is a shared_ptr that manages a D
        sp->bar(); // and p manages the D object
        std::cout << sp.use_count() << std::endl;
        auto sp2(sp); // copy constructs a new shared_ptr sp2
        std::cout << sp.use_count() << std::endl;
    } // ~D called here

    std::cout << "\n2) Runtime polymorphism demo\n";
    {
        custom_shared_ptr<Base> sp = custom_make_shared<Derived>(); // sp is a shared_ptr that manages a D as a pointer to B
        sp->bar(); // virtual dispatch, calls D::bar

        std::vector<custom_shared_ptr<Base>> v; // shared_ptr can be stored in a container
        v.push_back(custom_make_shared<Derived>());
        v.push_back(std::move(sp));
        v.emplace_back(new Derived{});
        for (auto& sp: v) sp->bar(); // virtual dispatch, calls D::bar
    } // ~D called 3 times

    std::cout << "\n3) Shared ownership from multiple threads\n";
    {
        custom_shared_ptr<Base> p = custom_make_shared<Derived>();
 
        print("Created a shared Derived (as a pointer to Base)", p);
 
        std::thread t1{thr, p}, t2{thr, p}, t3{thr, p};
        p.reset(); // release ownership from main
 
        print("Shared ownership between 3 threads and released ownership from main:", p);
 
        t1.join();
        t2.join();
        t3.join();
 
        std::cout << "All threads completed, the last one deleted Derived.\n";
    }

    std::cout << "\n4) custom_shared_ptr from unique_ptr\n";
    {
        auto up_ = std::make_unique<Derived>();
        custom_shared_ptr<Base> shp_ {std::move(up_)};
        std::cout << "count : " << shp_.use_count() << std::endl;
    }

    return 0;
}


