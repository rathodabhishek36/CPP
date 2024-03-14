#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace std;

template<typename T, typename Deleter=std::default_delete<T>>
struct custom_unique_ptr {
    using pointer_type = T*;
    using value_type = T;
    using reference_type = T&;
    using deleter_type = Deleter;

    custom_unique_ptr() : mptr_(nullptr) {}
    
    custom_unique_ptr(pointer_type p) : mptr_(p) { p=nullptr; }
    custom_unique_ptr(pointer_type p, deleter_type&& d) : mptr_(p), mdlt_(std::move(d)) { p=nullptr; }

    custom_unique_ptr(const custom_unique_ptr&) = delete;
    custom_unique_ptr(custom_unique_ptr&& p) : mptr_(p.mptr_), mdlt_(std::move(p.mdlt_)) {
        p.mptr_=nullptr;
    }
    template<typename T1, typename D1>
    custom_unique_ptr(custom_unique_ptr<T1, D1>&& p) : mptr_(p.mptr_), mdlt_(std::move(p.mdlt_)) { 
        p.mptr_=nullptr;
    }

    ~custom_unique_ptr() {
        get_deleter()(get());
    }

    custom_unique_ptr<T>& operator=(const custom_unique_ptr&)=delete;
    
    custom_unique_ptr& operator=(custom_unique_ptr&& p) {
        reset(p.release());
        return *this;
    }

    template<typename T1, typename D1>
    custom_unique_ptr& operator=(custom_unique_ptr<T1, D1>&& p) {
        reset(p.release());
        mdlt_ = std::move(p.get_deleter());
        return *this;
    }

    pointer_type get() {
        return mptr_;
    }

    pointer_type release() {
        auto tmp=mptr_;
        mptr_=nullptr;
        return tmp;
    }

    void reset(pointer_type p=nullptr) {
        get_deleter()(get());
        mptr_=p;
        p=nullptr;
    }

    operator bool() {
        return mptr_ != nullptr;
    }

    reference_type operator*() {
        return *mptr_;
    }

    pointer_type operator->() {
        return mptr_;
    }

    deleter_type& get_deleter() noexcept {
        return mdlt_;
    }

private:
    pointer_type mptr_;
    deleter_type mdlt_;
};

namespace ar {
template<typename T, typename... Args>
custom_unique_ptr<T> make_unique(Args&&... args) {
    return custom_unique_ptr<T>(new T(std::forward(args)...));
}
}

struct B {
    virtual ~B() = default;
    virtual void bar() { std::cout << "B::bar\n"; }
};
 
struct D : B {
    D() { std::cout << "D::D\n"; }
    ~D() { std::cout << "D::~D\n"; }
    void bar() override { std::cout << "D::bar\n"; }
};

// a function consuming a unique_ptr can take it by value or by rvalue reference
custom_unique_ptr<D> pass_through(custom_unique_ptr<D> p) {
    p->bar();
    return p;
}

// helper function for the custom deleter demo below
void close_file(std::FILE* fp) {
    std::fclose(fp);
}

int main() {
    
    std::cout << "1) Unique ownership semantics demo\n";
    {
        // Create a (uniquely owned) resource
        custom_unique_ptr<D> p = ar::make_unique<D>();
 
        // Transfer ownership to `pass_through`,
        // which in turn transfers ownership back through the return value
        custom_unique_ptr<D> q = pass_through(std::move(p));
 
        // p is now in a moved-from 'empty' state, equal to nullptr
        assert(!p);
    }
     std::cout << "\n" "2) Runtime polymorphism demo\n";
    {
        // Create a derived resource and point to it via base type
        std::unique_ptr<B> p = std::make_unique<D>();
 
        // Dynamic dispatch works as expected
        p->bar();
    }
    std::cout << "\n" "3) Custom deleter demo\n";
    std::ofstream("demo.txt") << 'x'; // prepare the file to read
    {
        using unique_file_t = custom_unique_ptr<std::FILE, decltype(&close_file)>;
        unique_file_t fp(std::fopen("demo.txt", "r"), &close_file);
        if (fp)
            std::cout << char(std::fgetc(fp.get())) << '\n';
    } // `close_file()` called here (if `fp` is not null)
 
    std::cout << "\n" "4) Custom lambda-expression deleter and exception safety demo\n";
    try
    {
        std::unique_ptr<D, void(*)(D*)> p(new D, [](D* ptr)
        {
            std::cout << "destroying from a custom deleter...\n";
            delete ptr;
        });
 
        throw std::runtime_error(""); // `p` would leak here if it were a plain pointer
    }
    catch (const std::exception&)
    {
        std::cout << "Caught exception\n";
    }
}
