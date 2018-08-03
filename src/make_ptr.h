#pragma once

#include <type_traits>
#include <memory>

namespace detail {

/** 
 C++17 metafunction to check if T is an instance of template U
 is_templ_instance<std::list<int>, std::list>::value == true
 is_templ_instance<std::list<int>, std::vector>::value == false
*/
template <typename T, template <typename, typename...> typename U>
struct is_templ_instance : public std::false_type {};

template <typename...Ts, template <typename, typename...> typename U>
struct is_templ_instance<U<Ts...>, U> : public std::true_type {};

/** check it T is a shared pointer */ 
template <typename T>
struct is_shared_ptr : public is_templ_instance<T, std::shared_ptr> {
};


// c++ doesn't allow partial specialization of function templates,
// and variadic templates can not be fully specialized,
// so this wrapper struct is needed to make partial specialization
// for shared_ptr to be created with make shared
template <typename R, typename Ptr, bool spec = is_shared_ptr<typename R::ptr>::value, typename ...Args>
struct mp {
    Ptr operator()(Args&& ...args) {
        return Ptr{new R(std::forward<Args>(args)...)};
    }
};

template <typename R, typename Ptr, typename ...Args>
struct mp <R, Ptr, true, Args...> {
    Ptr operator()(Args&&... args) {
        return std::make_shared<R>(std::forward<Args>(args)...);
    }
};

template <int i> struct prio: prio<i - 1> {};
template <> struct prio<0> {};

// create (smart) pointer based on R::ptr
template <typename R, typename ...Args>
typename R::ptr
make_ptr_(prio<0>, Args&&... args) {
    return detail::mp<R, typename R::ptr, detail::is_shared_ptr<typename R::ptr>::value, Args...>{}(std::forward<Args>(args)...);
}

// create (smart) pointer based on R::ptr_temlp<R>

template <typename R, typename ...Args>
typename R::template ptr_templ<R>
make_ptr_(prio<1>, Args&&... args) {
    return detail::mp<R, typename R::template ptr_templ<R>, detail::is_shared_ptr<typename R::template ptr_templ<R>>::value, Args...>{}(std::forward<Args>(args)...);
}

} // end detail namespace


/** 
 * create an appropriate (smart) pointer based on R::ptr_templ<R> or R::ptr
 * R::ptr_templ takes precedence over R::ptr
 * This allows to specify the type of managing smart pointer in a single place
 * 
 * Example:
 * @code {.cpp}
 * struct A {
 *    template <typename A> using ptr_templ = std::shared_ptr<A>;
 * };
 * struct B : public A {};
 * make_ptr<B>(); // returns std::shared_ptr<B>
 * @endcode
 */
template <typename R, typename ...Args>
auto make_ptr(Args&&... args) {
    return detail::make_ptr_<R, Args...>(detail::prio<1>{}, std::forward<Args>(args)...);
}
