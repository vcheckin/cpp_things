#pragma once
/// Copyright (c) 2018 Vassily Checkin. See included license file.
#include <atomic>
#include <type_traits>

/// shared pointer with intrusive reference counting
/// @tparam T pointed to type (element_type) 
/// @tparam P reference counting policy
template <typename T, typename Policy = typename T::policy_type>
class refc_ptr {
public:
    using element_type = T;
    using policy = Policy;
    constexpr refc_ptr() noexcept = default;
    explicit constexpr refc_ptr(std::nullptr_t) noexcept
    {}
    constexpr refc_ptr(T *p, bool ref = true) noexcept
        : ptr(p)
    {
        if (p && ref)
            policy::add_ref(p);
    }

    refc_ptr(const refc_ptr &o) noexcept
        : ptr(o.ptr)
    {
        if (o)
            policy::add_ref(ptr);
    }

    refc_ptr(refc_ptr &&o) noexcept
        : ptr(o.ptr)
    {
        o.ptr = nullptr;
    }

    template <typename U,
              typename = std::enable_if_t<std::is_convertible<U *, T *>::value>>
    refc_ptr(const refc_ptr<U> &o) noexcept
    {
        ptr = o.get();
        if (ptr)
            policy::add_ref(ptr);
    }

    refc_ptr &operator=(const refc_ptr &rhs)
    {
        refc_ptr(rhs).swap(*this);
        return *this;
    }

    template <class U> refc_ptr &operator=(const refc_ptr<U> &rhs)
    {
        refc_ptr(rhs).swap(*this);
        return *this;
    }

    template <class U> refc_ptr &operator=(refc_ptr<U> &&rhs) noexcept
    {
        refc_ptr(std::move(rhs)).swap(*this);
        return *this;
    }

    ~refc_ptr()
    {
        if (ptr)
            policy::release(ptr);
    }

    T *get() const noexcept
    {
        return ptr;
    }

    T *operator->() const noexcept
    {
        return ptr;
    }

    explicit operator bool() const noexcept
    {
        return ptr;
    }

    void swap(refc_ptr &rhs) noexcept
    {
        std::swap(ptr, rhs.ptr);
    }
    void reset() noexcept
    {
        refc_ptr().swap(*this);
    }

    void reset(T *to, bool add_ref = true) noexcept
    {
        refc_ptr(to, add_ref).swap(*this);
    }

    T *detach() noexcept
    {
        auto rv = ptr;
        ptr = nullptr;
        return rv;
    }

    T &operator*() const noexcept
    {
        return *ptr;
    }
private:
    T *ptr = nullptr;
};

/// weak pointer with intrusive reference counting 
template <typename T, typename Policy = typename T::policy_type, typename WeakPolicy = typename T::weak_policy_type>
class refc_weak_ptr {
public:
    using policy = Policy;
    using weak_policy = WeakPolicy;
    constexpr refc_weak_ptr() noexcept = default;
    explicit constexpr refc_weak_ptr(std::nullptr_t) noexcept
    {}
    refc_weak_ptr(const refc_ptr<T> &o) noexcept
    {
        if (o) {
            ptr = o.get();
            weak_policy::add_ref(ptr);
        }
    }
    refc_weak_ptr(const refc_weak_ptr &o) noexcept
    {
        if (o.ptr) {
            ptr = o.ptr;
            weak_policy::add_ref(ptr);
        }
    }
    refc_weak_ptr(refc_weak_ptr &&o) noexcept
    {
        ptr = o.ptr;
        o.ptr = nullptr;
    }
    template <typename U,
              typename = std::enable_if_t<std::is_convertible<U *, T *>::value>>
    refc_weak_ptr(const refc_weak_ptr<U> &o) noexcept
    {
        ptr = o.get();
        if (ptr)
            weak_policy::add_ref(ptr);
    }

    refc_weak_ptr &operator=(const refc_weak_ptr &rhs)
    {
        refc_weak_ptr(rhs).swap(*this);
        return *this;
    }
    refc_weak_ptr &operator=(refc_weak_ptr &&rhs) noexcept
    {
        refc_weak_ptr(std::move(rhs)).swap(*this);
        return *this;
    }
    refc_weak_ptr &operator=(const refc_ptr<T> &rhs)
    {
        refc_weak_ptr(rhs).swap(*this);
        return *this;
    }
    ~refc_weak_ptr()
    {
        if (ptr)
            weak_policy::release(ptr);
    }
    void swap(refc_weak_ptr &rhs) noexcept
    {
        std::swap(ptr, rhs.ptr);
    }
    /// lock weak pointer to shared pointer
    /// @return shared pointer or empty shared pointer if object is gone 
    refc_ptr<T> lock() const noexcept
    {
        if(!ptr)
            return {};
        auto c = policy::try_ref(ptr);
        if(c == 0) {
            return {};
        }
        return refc_ptr<T>(ptr, false);
    }
private:
    T *ptr = nullptr;
};

template <class T, class U>
inline bool operator==(const refc_ptr<T> &a, const refc_ptr<U> &b) noexcept
{
    return a.get() == b.get();
}

template <class T, class U>
inline bool operator==(refc_ptr<T> const &a, U *b) noexcept
{
    return a.get() == b;
}

template <class T, class U>
inline bool operator==(T *a, refc_ptr<U> const &b) noexcept
{
    return a == b.get();
}

template <class T, class U>
inline bool operator!=(refc_ptr<T> const &a, refc_ptr<U> const &b) noexcept
{
    return a.get() != b.get();
}

template <class T, class U>
inline bool operator!=(refc_ptr<T> const &a, U *b) noexcept
{
    return a.get() != b;
}

template <class T, class U>
inline bool operator!=(T *a, refc_ptr<U> const &b) noexcept
{
    return a != b.get();
}

template <class T>
inline bool operator<(refc_ptr<T> const &a, refc_ptr<T> const &b) noexcept
{
    return a.get() < b.get();
}
template <class T>
inline bool operator!=(refc_ptr<T> const &a, refc_ptr<T> const &b) noexcept
{
    return a.get() != b.get();
}

namespace std {
// pointer casts

template <class T, class U>
refc_ptr<T> static_pointer_cast(refc_ptr<U> const &p)
{
    return static_cast<T *>(p.get());
}

template <class T, class U> refc_ptr<T> const_pointer_cast(refc_ptr<U> const &p)
{
    return const_cast<T *>(p.get());
}

#ifdef __GXX_RTTI
template <class T, class U>
refc_ptr<T> dynamic_pointer_cast(refc_ptr<U> const &p)
{
    return dynamic_cast<T *>(p.get());
}
#endif
template <class T, class U> refc_ptr<T> static_pointer_cast(refc_ptr<U> &&p)
{
    return refc_ptr<T>(static_cast<T *>(p.detach()), false);
}

template <class T, class U> refc_ptr<T> const_pointer_cast(refc_ptr<U> &&p)
{
    return refc_ptr<T>(const_cast<T *>(p.detach()), false);
}

#ifdef __GXX_RTTI
template <class T, class U> refc_ptr<T> dynamic_pointer_cast(refc_ptr<U> &&p)
{
    T *p2 = dynamic_cast<T *>(p.get());
    refc_ptr<T> r(p2, false);
    if (p2)
        p.detach();
    return r;
}
#endif
}

/// @brief base class for reference counted objects
/// @tparam T derived class for CRTP
template <typename T> class refc {
public:
    // default reference counting policy for refc_ptr with `delete p` on release
    struct refc_policy {
        static auto add_ref(const refc *p) noexcept
        {
            return p->rc.fetch_add(1, std::memory_order_relaxed);
        }
        static void release(const refc *p) noexcept
        {
            if (p->rc.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete p;
            }
        }
    };
    using policy_type = refc_policy;
    using ptr = refc_ptr<T, policy_type>;
    using cptr = refc_ptr<const T, policy_type>;

    constexpr unsigned long refcount() const noexcept
    {
        return rc.load(std::memory_order_relaxed);
    }
    template <typename Y = T,
              std::enable_if_t<std::is_base_of_v<refc<T>, Y>, bool> = true>
    refc_ptr<Y> shared_from_this()
    {
        return static_cast<Y *>(this);
    }

protected:
    refc(const refc &) = delete;
    refc &operator=(const refc &) = delete;

    constexpr refc() = default;

    virtual ~refc() = default;
    mutable std::atomic<unsigned long> rc{ 0 };
};

/// Base class for reference counted objects with weak references
/// @tparam T derived type for CRTP
template <typename T> class refc_weak_base : public refc<T> {
public:
    struct weak_refc_policy {
        static auto add_ref(const refc_weak_base *x) noexcept
        {
            return x->weak_rc.fetch_add(1, std::memory_order_relaxed);
        }
        static void release(const refc_weak_base *x) noexcept
        {
            if (x->weak_rc.fetch_sub(1, std::memory_order_release) == 1) {
                ::operator delete(const_cast<refc_weak_base *>(x));
            }
        }
    };

    struct strong_refc_policy {
        static auto add_ref(const refc_weak_base *x) noexcept
        {
            x->weak_rc.fetch_add(1, std::memory_order_relaxed);
            return x->rc.fetch_add(1, std::memory_order_relaxed);
        }
        static auto try_ref(const refc_weak_base *x) noexcept
        {
            x->weak_rc.fetch_add(1, std::memory_order_relaxed);
            auto r = x->rc.load(std::memory_order_relaxed);
            while(r > 0) {
                if(x->rc.compare_exchange_weak(r, r + 1, std::memory_order_relaxed)) {
                    return r;
                }
            };
            x->weak_rc.fetch_sub(1, std::memory_order_relaxed);
            return r;
        }
        static void release(const refc_weak_base *x) noexcept
        {
            if (x->rc.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                std::destroy_at(x);
            }
            if (x->weak_rc.fetch_sub(1, std::memory_order_release) == 1) {
                ::operator delete(const_cast<refc_weak_base *>(x));
            }
        }
    };
    using policy_type = strong_refc_policy;
    using weak_policy_type = weak_refc_policy;

protected:
    using refc<T>::refc;
    mutable std::atomic<unsigned long> weak_rc{ 0 };
};
