// A template meta-programming exercise
// https://www.slamecka.cz/posts/2021-03-17-cpp-metaprogramming-exercises-1/

#include <iostream>
#include <type_traits>

namespace {

/**
 * 1. Define Vector, a template level list of integers.
 * Hint: Use non-type template parameter pack.
 */

// Your code goes here:
template<int...>
struct Vector {};
// ^ Your code goes here

static_assert(std::is_same_v<Vector<1,2>, Vector<1,2>>);

/**
 * 2. Define function print() that prints Vector-s.
 * Example: print(Vector<1,2,3>{}); // prints "1 2 3" (newline)
 * See main() below.
 */

// Your code goes here:
template<int i, int... is>
void print(Vector<i, is...>) {
    std::cout << i << " ";
    print(Vector<is...>{});
}

template<int i>
void print(Vector<i>) {
    std::cout << i << std::endl; // for the last template parameter
}

template<int... i>
void print(Vector<i...>) {
    std::cout << '\n'; // empty vector case
}
// ^ Your code goes here

/**
 * 3. Define Prepend.
 * Hint: Use `using type = ...` inside a struct that has both non-type and type template parameters.
 */
// Your code goes here:
template<int i, typename T>
struct Prepend {};

// Partial specialization
template<int i, int... is>
struct Prepend<i, Vector<is...>> {
    using type = Vector<i, is...>;
};
// ^ Your code goes here

static_assert(std::is_same_v<Prepend<1, Vector<2,3>>::type, Vector<1,2,3>>);


/**
 * 4. Define PrependT that can be used without ::type.
 * Hint: See how enable_if_t is defined in terms of enable_if.
 *
 * This technique is not used further to reduce boilerplate.
 */

// Your code goes here:
template<int i, typename T=void>
using PrependT = typename Prepend<i,T>::type;

// ^ Your code goes here

static_assert(std::is_same_v<PrependT<1, Vector<2,3>>, Vector<1,2,3>>);

/**
 * 5. Define Append.
 */

// Your code goes here:
template<int i, typename T>
struct Append {};

template<int end, int start, int... rem>
struct Append<end, Vector<start, rem...>> {
    using type = typename Prepend<start, typename Append<end, Vector<rem...>>::type>::type;
};

template<int end, int... rem>
struct Append<end, Vector<rem...>> {
    using type = Vector<end>;
};

// ^ Your code goes here

static_assert(std::is_same_v< Append<4, Vector<1,2,3>>::type , Vector<1,2,3,4> >);

/**
 * 6. Define PopBack.
 */

// Your code goes here:
template <typename T>
struct PopBack{};

template <int a, int b, int... rem>
struct PopBack<Vector<a, b, rem...>> {
    using type = typename Prepend<a, typename PopBack<Vector<b, rem...>>::type>::type;
};

template <int a, int... rem>
struct PopBack<Vector<a, rem...>> {
    using type = Vector<rem...>; // didn't return type for the last element.
};
// ^ Your code goes here

static_assert(std::is_same_v< PopBack<Vector<1,2,3,4>>::type , Vector<1,2,3> >);


/**
 * 7. Define RemoveFirst, that removes the first occurence of element R from vector V.
 */

// Your code goes here:
template<int i, typename T>
struct RemoveFirst{};

template <int target, int... rem>
struct RemoveFirst<target, Vector<target, rem...>> {
    using type = Vector<rem...>;
};

template <int target, int ele, int... rem>
struct RemoveFirst<target, Vector<ele, rem...>> {
    using type = typename Prepend<ele, typename RemoveFirst<target, Vector<rem...>>::type>::type;
};

template <int target, int... rem>
struct RemoveFirst<target, Vector<rem...>> {
    using type = Vector<rem...>;
};

// ^ Your code goes here

static_assert(std::is_same_v<RemoveFirst<1, Vector<1,1,2>>::type, Vector<1,2>>);


/**
 * 8. Define RemoveAll, that removes all occurences of element R from vector V.
 */

// Your code goes here:
template <int i, typename T>
struct RemoveAll{};

template <int target, int ele, int... rem>
struct RemoveAll<target, Vector<ele, rem...>> {
    using type = typename Prepend<ele, typename RemoveAll<target, Vector<rem...>>::type>::type;
};

template <int target, int... rem>
struct RemoveAll<target, Vector<target, rem...>> {
    using type = typename RemoveAll<target, Vector<rem...>>::type;
};

template <int target, int... rem>
struct RemoveAll<target, Vector<rem...>> {
    using type = Vector<rem...>;
};
// ^ Your code goes here

static_assert(std::is_same_v<RemoveAll<9, Vector<1,9,2,9,3,9>>::type, Vector<1,2,3>>);


/**
 * 9. Define Length.
 * Hint: Use `static constexpr int value = ...` inside the struct.
 */

// Your code goes here:
template<typename T>
struct Length {};

template <int i, int... is>
struct Length<Vector<i, is...>> {
    static constexpr int value = 1 + Length<Vector<is...>>::value;
};

template <int... is>
struct Length<Vector<is...>> {
    static constexpr int value = 0;
};
// ^ Your code goes here

static_assert(Length<Vector<1,2,3,4>>::value == 4);


/**
 * 10. Define length, which works like Length<V>::value.
 * Hint: See how is_same_v is defined in terms of is_same.
 */

// Your code goes here:
template <typename T>
inline constexpr size_t length = Length<T>::value;
// ^ Your code goes here

static_assert(length<Vector<>> == 0);
static_assert(length<Vector<1,2,3>> == 3);


/**
 * 11. Define Min, that stores the minimum of a vector in its property `value`.
 */

// Your code goes here:
template <typename T>
struct Min{};

template <int i, int j, int... is>
struct Min<Vector<i, j, is...>> {
    static constexpr int value = Min<Vector<(i<j)?i:j, is...>>::value;
};

template <int i, int... is>
struct Min<Vector<i, is...>> {
    static constexpr int value = i;
};
// ^ Your code goes here

static_assert(Min<Vector<3,1,2>>::value == 1);
static_assert(Min<Vector<1,2,3>>::value == 1);
static_assert(Min<Vector<3,2,1>>::value == 1);


/**
 * 12. Define Sort.
 */

// Your code goes here:
template<typename T1, typename T2>
struct SortHelper{};

template <int... sorted, int first, int... rest>
struct SortHelper<Vector<sorted...>, Vector<first, rest...>> {
    static constexpr int min = Min<Vector<first, rest...>>::value;
    using rest_t = typename RemoveFirst<min, Vector<first, rest...>>::type;
    using sorted_t = typename Append<min, Vector<sorted...>>::type;
    using type = typename SortHelper<sorted_t, rest_t>::type;
};

template <int... sorted, int... rest>
struct SortHelper<Vector<sorted...>, Vector<rest...>> {
    using type = Vector<sorted...>;
};

template<typename T>
struct Sort{};

template <int... ele>
struct Sort<Vector<ele...>> {
    using type = typename SortHelper<Vector<>, Vector<ele...>>::type;
};

// ^ Your code goes here

static_assert(std::is_same_v<Sort<Vector<1>>::type, Vector<1>>);
static_assert(std::is_same_v<Sort<Vector<4,1,2,5,6,3>>::type, Vector<1,2,3,4,5,6>>);
static_assert(std::is_same_v<Sort<Vector<3,3,1,1,2,2>>::type, Vector<1,1,2,2,3,3>>);
static_assert(std::is_same_v<Sort<Vector<2,2,1,1,3,3>>::type, Vector<1,1,2,2,3,3>>);


/**
 * 13. Define Uniq.
 */

// Your code goes here:
template<typename T>
struct Uniq{};

template <int a, int... rest>
struct Uniq<Vector<a, a, rest...>> {
    using type = typename Uniq<Vector<a, rest...>>::type;
};

template <int a, int... rest>
struct Uniq<Vector<a, rest...>> {
    using type = typename Prepend<a, typename Uniq<Vector<rest...>>::type>::type;
};

template<int... rest>
struct Uniq<Vector<rest...>> {
    using type = Vector<rest...>;
};

// ^ Your code goes here

static_assert(std::is_same_v<Uniq<Vector<1,1,2,2,1,1>>::type, Vector<1,2,1>>);


/**
 * 14. Define type Set.
 */

// Your code goes here:
template <int... is>
struct Set {
    using type = typename Uniq<typename Sort<Vector<is...>>::type>::type;
};

// ^ Your code goes here

static_assert(std::is_same_v<Set<2,1,3,1,2,3>::type, Set<1,2,3>::type>);


/**
 * 15. Define SetFrom.
 */

template<typename T>
struct SetFrom{};

template<int... is>
struct SetFrom<Vector<is...>> {
    using type = typename Set<is...>::type;
};

// Your code goes here:
// ^ Your code goes here

static_assert(std::is_same_v<SetFrom<Vector<2,1,3,1,2,3>>::type, Set<1,2,3>::type>);


/**
 * 16. Define Get for Vector.
 * Provide an improved error message when accessing outside of Vector bounds.
 */

// Your code goes here:
template<int index, typename T>
struct Get{};

template<int i, int... is>
struct Get<0, Vector<i, is...>> {
    static constexpr int value = i;
};

template<int index, int i, int... is>
struct Get<index, Vector<i, is...>> {
    static_assert(index>length);
};
// ^ Your code goes here

// static_assert(Get<0, Vector<0,1,2>>::value == 0);
// static_assert(Get<1, Vector<0,1,2>>::value == 1);
// static_assert(Get<2, Vector<0,1,2>>::value == 2);
// static_assert(Get<9, Vector<0,1,2>>::value == 2); // How good is your error message?


/**
 * 17. Define BisectLeft for Vector.
 * Given n and arr, return the first index i such that arr[i] >= n.
 * If it doesn't exist, return the length of the array.
 *
 * Don't worry about complexity but aim for the binary search pattern.
 *
 * Hint: You might find it convenient to define a constexpr helper function.
 */

// Your code goes here:
// ^ Your code goes here

// static_assert(BisectLeft<  3, Vector<0,1,2,3,4>>::value == 3);
// static_assert(BisectLeft<  3, Vector<0,1,2,4,5>>::value == 3);
// static_assert(BisectLeft<  9, Vector<0,1,2,4,5>>::value == 5);
// static_assert(BisectLeft< -1, Vector<0,1,2,4,5>>::value == 0);
// static_assert(BisectLeft<  2, Vector<0,2,2,2,2,2>>::value == 1);


/**
 * 18. Define Insert for Vector, it should take position, value and vector.
 * Don't worry about bounds.
 * Hint: use the enable_if trick, e.g.
 *   template<typename X, typename Enable = void> struct Foo;
 *   template<typename X> struct<std::enable_if_t<..something      about X..>> Foo {...};
 *   template<typename X> struct<std::enable_if_t<..something else about X..>> Foo {...};
 */

// Your code goes here:
// ^ Your code goes here

// static_assert(std::is_same_v<Insert<0, 3, Vector<4,5,6>>::type, Vector<3,4,5,6>>);
// static_assert(std::is_same_v<Insert<1, 3, Vector<4,5,6>>::type, Vector<4,3,5,6>>);
// static_assert(std::is_same_v<Insert<2, 3, Vector<4,5,6>>::type, Vector<4,5,3,6>>);
// static_assert(std::is_same_v<Insert<3, 3, Vector<4,5,6>>::type, Vector<4,5,6,3>>);

}

int main()
{
    print(Vector<>{});
    print(Vector<1>{});
    print(Vector<1,2,3,4,5,6>{});
//     std::cout << typeid(Vector<1,2,3,4,5,6>{}).name() << '\n';
}
