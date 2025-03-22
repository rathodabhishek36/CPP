#include <iostream>
#include <memory>
#include <functional>

// TODO - Optimize by storing function pointers and reference wrappers over stack
// rather than allocating additional memory
// Follow : https://devblogs.microsoft.com/oldnewthing/20200514-00/?p=103749

namespace ajr {

template<typename Signature>
class function;

template<typename Ret, typename... Args>
class function<Ret(Args...)> {

    struct CallableInterface {
        virtual Ret call(Args...) = 0;

        // To support copy constructor of function
        virtual std::unique_ptr<CallableInterface> clone() = 0; 
        
        virtual ~CallableInterface() = default;
        
        // support rule of 5 ??
    };

    template<typename Function>
    struct Callable : public CallableInterface {
        Function function_;

        Callable(Function f) : function_(std::move(f)) {
        }

        ~Callable() override = default;

        Callable(const Callable& other) : function_(other.function_) {
        }

        Callable(Callable&& other) : function_(std::move(other.function_)) {
        }

        Ret call(Args... args) override {
            // std::invoke is used to support member function / data member calls
            // for function pointers/lambdas, function_(args...) would suffice
            return std::invoke(function_, args...); 
        }

        std::unique_ptr<CallableInterface> clone() override {
            return std::make_unique<Callable>(function_);
        }

    };

    std::unique_ptr<CallableInterface> callable_;

public:
    function() : callable_(nullptr) {}

    template<typename FunctionObject>
    function(FunctionObject f) : callable_(std::make_unique<Callable<FunctionObject>>(std::move(f))) {
    }

    function(const function& other) : callable_(other.callable_ ? other.callable_->clone() : nullptr) {
    }

    function(function&& other) : callable_(std::move(other.callable_)) {
        other.callable_ = nullptr;
    }

    function& operator=(const function& other) {
        if (this != &other) {
            callable_ = other.callable_ ? other.callable_->clone() : nullptr;
        }
        return *this;
    }

    function& operator=(function&& other) {
        if (this != &other) {
            callable_ = std::move(other.callable_);
            other.callable_ = nullptr;
        }
        return *this;
    }

    template<typename FunctionObject>
    function& operator=(FunctionObject&& f) {
        callable_ = std::make_unique<Callable<FunctionObject>>(std::forward<FunctionObject>(f));
        return *this;
    }

    Ret operator()(Args... args) {
        if (callable_) {
            return callable_->call(args...);
        }
        throw std::bad_function_call();
    }

    void swap(function& other) {
        std::swap(callable_, other.callable_);
    }

    operator bool() noexcept {
        return callable_ != nullptr;
    }

    const std::type_info& target_type() const noexcept {
        if (!callable_) {
            return typeid(void);
        }
        return typeid(*callable_);
    }

    template<typename FunctionObject>
    FunctionObject* target() noexcept {
        if (target_type() == typeid(FunctionObject)) {
            return dynamic_cast<FunctionObject*>(&callable_.get().function_);
        }
        return nullptr;
    }

};

}

struct Foo {
    Foo(int num) : num_(num) {}
    void print_add(int i) const { std::cout << num_ + i << '\n'; }
    int num_;
};
 
void print_num(int i) {
    std::cout << i << '\n';
}
 
struct PrintNum {
    void operator()(int i) const {
        std::cout << i << '\n';
    }
};

int main() {

    // store a free function
    ajr::function<void(int)> f_display = print_num;
    f_display(-9);
 
    // store a lambda
    ajr::function<void()> f_display_42 = []() { print_num(42); };
    f_display_42();

    // catch unintialized function call
    try {
        ajr::function<void(int, double)> f;
        f(1, 1.3f);
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << '\n';
    }
 
    // store the result of a call to std::bind
    ajr::function<void()> f_display_31337 = std::bind(print_num, 31337);
    f_display_31337();
 
    // store a call to a member function
    ajr::function<void(const Foo&, int)> f_add_display = &Foo::print_add;
    const Foo foo(314159);
    f_add_display(foo, 1);
    f_add_display(314159, 1);
 
    // store a call to a data member accessor
    ajr::function<int(Foo const&)> f_num = &Foo::num_;
    std::cout << "num_: " << f_num(foo) << '\n';
 
    // store a call to a member function and object
    using std::placeholders::_1;
    ajr::function<void(int)> f_add_display2 = std::bind(&Foo::print_add, foo, _1);
    f_add_display2(2);
 
    // store a call to a member function and object ptr
    ajr::function<void(int)> f_add_display3 = std::bind(&Foo::print_add, &foo, _1);
    f_add_display3(3);
 
    // store a call to a function object
    ajr::function<void(int)> f_display_obj = PrintNum();
    f_display_obj(18);
 
    auto factorial = [](int n)
    {
        // store a lambda object to emulate "recursive lambda"; aware of extra overhead
        ajr::function<int(int)> fac = [&](int n) { return (n < 2) ? 1 : n * fac(n - 1); };
        // note that "auto fac = [&](int n) {...};" does not work in recursive calls
        return fac(n);
    };
    for (int i{5}; i != 8; ++i)
        std::cout << i << "! = " << factorial(i) << ";  ";
    std::cout << '\n';

    return 0;
}
