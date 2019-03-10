/// Copyright (c) 2018 Vassily Checkin. See included license file.
#include <atomic>
#include <gtest/gtest.h>
#include <ptr.h>
#include <thread>
#include <vector>

namespace refc_test {

struct s : public refc<s> {};
struct w : public refc_weak_base<w> {};

template <typename b> struct base_t : public b {
    static std::atomic<int> instance_count;
    static int instances()
    {
        return instance_count;
    }
    base_t()
    {
        instance_count++;
    }
    virtual ~base_t()
    {
        instance_count--;
    }
    std::atomic<unsigned long> &refcount_ref()
    {
        return b::rc;
    };
};

using base = base_t<w>;

template <typename b = base> struct D1_t : public base_t<b> {
    using base_t<b>::base_t;
};

template <typename b = base> struct D2_t : public D1_t<b> {
    using D1_t<b>::D1_t;
};

using D1 = D1_t<>;
using D2 = D2_t<>;

template <typename b> std::atomic<int> base_t<b>::instance_count = 0;
} // namespace refc_test

using namespace refc_test;

template <typename b> class refc_ptr_ste : public testing::Test {
protected:
    using base = refc_test::base_t<b>;
    using D1 = refc_test::D1_t<b>;
    using D2 = refc_test::D2_t<b>;
};

using BaseTypes = testing::Types<refc_test::base_t<w>, refc_test::base_t<s>>;

TYPED_TEST_SUITE(refc_ptr_ste, BaseTypes);

TEST(refc_ptr, weak_ptr_basic)
{
    {
        refc_weak_ptr<base_t<w>> pw;
        {
            refc_ptr<base> p(new base);
            EXPECT_NE(p.get(), nullptr);
            EXPECT_EQ(base::instance_count, 1);
            pw = p;
            EXPECT_EQ(pw.lock().get(), p.get());
            auto pw2 = pw;
            EXPECT_EQ(pw2.lock().get(), p.get());
        }
        auto pw2 = pw;

        EXPECT_EQ(base::instance_count, 0);
        EXPECT_EQ(pw.lock().get(), nullptr);
        EXPECT_EQ(pw2.lock().get(), nullptr);
    }
    EXPECT_EQ(base::instance_count, 0);
}

TEST(refc_ptr, try_ref_race)
{
    struct data : public refc_weak_base<data> {
        int value = 100500;
        std::vector<int> values = {1,2,3};
    };
    constexpr int ITERATIONS = 10000;
    constexpr int THREAD_COUNT = 4;

    std::vector<std::thread> threads;

    std::vector<refc_ptr<data>> ptrs(ITERATIONS);
    std::vector<refc_weak_ptr<data>> weak_ptrs(ITERATIONS);
    for (int i = 0; i < ITERATIONS; i++) {
        ptrs[i] = refc_ptr<data>(new data);
        weak_ptrs[i] = ptrs[i];
    }
    std::atomic<bool> done(false);
    for (int i = 1; i < THREAD_COUNT; i++) {
        threads.push_back(std::thread([&, i]() {
            for (int j = 0; j < ITERATIONS && !done; j++) {
                auto ptr = weak_ptrs[j].lock();
                if(ptr) {
                    EXPECT_EQ(ptr->value, 100500);
                    EXPECT_EQ(ptr->values.size(), 3);
                    EXPECT_EQ(ptr->values[2], 3);
                }
            }
        }));
    }
    for (auto &p : ptrs) {
        p.reset();
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    done = true;
    for (auto &t : threads) {
        t.join();
    }
}

TYPED_TEST(refc_ptr_ste, default_ctor)
{
    refc_ptr<base> p;
    EXPECT_EQ(p.get(), nullptr);
}

TYPED_TEST(refc_ptr_ste, ptr_ctor)
{
    {
        refc_ptr<base> p(new base);
        EXPECT_NE(p.get(), nullptr);
        EXPECT_EQ(base::instance_count, 1);
    }
    EXPECT_EQ(base::instance_count, 0);
}

TYPED_TEST(refc_ptr_ste, ptr_assign)
{
    {
        refc_ptr<D1> p1;

        p1 = p1.get();

        EXPECT_EQ(p1, p1);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        refc_ptr<D1> p2;

        p1 = p2.get();

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        refc_ptr<D1> p3(p1);

        p1 = p3.get();

        EXPECT_TRUE(p1 == p3);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        EXPECT_TRUE(base::instance_count == 0);

        refc_ptr<D1> p4(new D1);

        EXPECT_TRUE(base::instance_count == 1);

        p1 = p4.get();

        EXPECT_TRUE(base::instance_count == 1);

        EXPECT_TRUE(p1 == p4);

        EXPECT_TRUE(p1->refcount() == 2);

        p1 = p2.get();

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(base::instance_count == 1);

        p4 = p3.get();

        EXPECT_TRUE(p4 == p3);
        EXPECT_TRUE(base::instance_count == 0);
    }
    {
        refc_ptr<D1> p1;

        refc_ptr<D2> p2;

        p1 = p2.get();

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        EXPECT_EQ(base::instance_count, 0);

        refc_ptr<D2> p4(new D2);

        EXPECT_EQ(base::instance_count, 1);
        EXPECT_EQ(p4->refcount(), 1);

        refc_ptr<D1> p5(p4);
        EXPECT_EQ(p4->refcount(), 2);

        p1 = p4.get();

        EXPECT_EQ(base::instance_count, 1);

        EXPECT_TRUE(p1 == p4);

        EXPECT_EQ(p1->refcount(), 3);
        EXPECT_EQ(p4->refcount(), 3);

        p1 = p2.get();

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(base::instance_count == 1);
        EXPECT_TRUE(p4->refcount() == 2);

        p4 = p2.get();
        p5 = p2.get();

        EXPECT_TRUE(p4 == p2);
        EXPECT_TRUE(base::instance_count == 0);
    }
}

template <class T> T *get_pointer(refc_ptr<T> const &p) noexcept
{
    return p.get();
}

namespace N {

using base = refc_test::base;

} // namespace N

struct X : public virtual base {};

struct Y : public X {};

TYPED_TEST(refc_ptr_ste, pointer_constructor)
{
    {
        refc_ptr<D1> px(nullptr);
        EXPECT_EQ(px.get(), nullptr);
    }

    {
        refc_ptr<D1> px(nullptr, false);
        EXPECT_EQ(px.get(), nullptr);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        D1 *p = new D1;
        EXPECT_EQ(p->refcount(), 0);

        EXPECT_EQ(base::instance_count, 1);

        refc_ptr<D1> px(p);
        EXPECT_EQ(px.get(), p);
        EXPECT_EQ(px->refcount(), 1);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        D1 *p = new D1;
        EXPECT_EQ(p->refcount(), 0);

        EXPECT_EQ(base::instances(), 1);

        p->refcount_ref()++;
        EXPECT_EQ(p->refcount(), 1);

        refc_ptr<D1> px(p, false);
        EXPECT_EQ(px.get(), p);
        EXPECT_EQ(px->refcount(), 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, copy_ctor)
{
    {
        refc_ptr<D1> px;
        refc_ptr<D1> px2(px);
        EXPECT_TRUE(px2.get() == px.get());
    }

    {
        refc_ptr<D2> py;
        refc_ptr<D1> px(py);
        EXPECT_TRUE(px.get() == py.get());
    }

    {
        refc_ptr<D1> px(nullptr);
        refc_ptr<D1> px2(px);
        EXPECT_TRUE(px2.get() == px.get());
    }

    {
        refc_ptr<D2> py(nullptr);
        refc_ptr<D1> px(py);
        EXPECT_TRUE(px.get() == py.get());
    }

    {
        refc_ptr<D1> px(nullptr, false);
        refc_ptr<D1> px2(px);
        EXPECT_TRUE(px2.get() == px.get());
    }

    {
        refc_ptr<D2> py(nullptr, false);
        refc_ptr<D1> px(py);
        EXPECT_TRUE(px.get() == py.get());
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        refc_ptr<D1> px2(px);
        EXPECT_TRUE(px2.get() == px.get());

        EXPECT_TRUE(base::instances() == 1);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D2> py(new D2);
        refc_ptr<D1> px(py);
        EXPECT_TRUE(px.get() == py.get());

        EXPECT_TRUE(base::instances() == 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, dtor)
{
    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);

        {
            refc_ptr<D1> px2(px);
            EXPECT_TRUE(px->refcount() == 2);
        }

        EXPECT_TRUE(px->refcount() == 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, copy_assign)
{
    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> p1;

        p1 = p1;

        EXPECT_TRUE(p1 == p1);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        refc_ptr<D1> p2;

        p1 = p2;

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        refc_ptr<D1> p3(p1);

        p1 = p3;

        EXPECT_TRUE(p1 == p3);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        EXPECT_TRUE(base::instances() == 0);

        refc_ptr<D1> p4(new D2);

        EXPECT_TRUE(base::instances() == 1);

        p1 = p4;

        EXPECT_TRUE(base::instances() == 1);

        EXPECT_TRUE(p1 == p4);

        EXPECT_TRUE(p1->refcount() == 2);

        p1 = p2;

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(base::instances() == 1);

        p4 = p3;

        EXPECT_TRUE(p4 == p3);
        EXPECT_TRUE(base::instances() == 0);
    }
}

TYPED_TEST(refc_ptr_ste, conv_assign)
{
    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> p1;

        refc_ptr<D2> p2;

        p1 = p2;

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(p1 ? false : true);
        EXPECT_TRUE(!p1);
        EXPECT_TRUE(p1.get() == 0);

        EXPECT_TRUE(base::instances() == 0);

        refc_ptr<D2> p4(new D2);

        EXPECT_TRUE(base::instances() == 1);
        EXPECT_TRUE(p4->refcount() == 1);

        refc_ptr<D1> p5(p4);
        EXPECT_TRUE(p4->refcount() == 2);

        p1 = p4;

        EXPECT_TRUE(base::instances() == 1);

        EXPECT_TRUE(p1 == p4);

        EXPECT_TRUE(p1->refcount() == 3);
        EXPECT_TRUE(p4->refcount() == 3);

        p1 = p2;

        EXPECT_TRUE(p1 == p2);
        EXPECT_TRUE(base::instances() == 1);
        EXPECT_TRUE(p4->refcount() == 2);

        p4 = p2;
        p5 = p2;

        EXPECT_TRUE(p4 == p2);
        EXPECT_TRUE(base::instances() == 0);
    }
}

TYPED_TEST(refc_ptr_ste, reset)
{
    EXPECT_EQ(base::instances(), 0);
    {
        refc_ptr<D1> px;
        EXPECT_EQ(px.get(), nullptr);

        px.reset();
        EXPECT_EQ(px.get(), nullptr);

        D1 *p = new D1;
        EXPECT_EQ(p->refcount(), 0);
        EXPECT_EQ(base::instances(), 1);

        px.reset(p);
        EXPECT_EQ(px.get(), p);
        EXPECT_EQ(px->refcount(), 1);

        px.reset();
        EXPECT_EQ(px.get(), nullptr);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(base::instances() == 1);

        px.reset(0);
        EXPECT_TRUE(px.get() == 0);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(base::instances() == 1);

        px.reset(0, false);
        EXPECT_TRUE(px.get() == 0);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(base::instances() == 1);

        px.reset(0, true);
        EXPECT_TRUE(px.get() == 0);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        D1 *p = new D1;
        EXPECT_TRUE(p->refcount() == 0);

        EXPECT_TRUE(base::instances() == 1);

        refc_ptr<D1> px;
        EXPECT_TRUE(px.get() == 0);

        px.reset(p, true);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px->refcount() == 1);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        D1 *p = new D1;
        EXPECT_TRUE(p->refcount() == 0);

        EXPECT_TRUE(base::instances() == 1);

        refc_ptr<D1> px;

        refc_ptr<D1>::policy::add_ref(p);
        EXPECT_TRUE(p->refcount() == 1);

        EXPECT_TRUE(px.get() == 0);

        px.reset(p, false);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px->refcount() == 1);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(px.get() != 0);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);

        D1 *p = new D1;
        EXPECT_TRUE(p->refcount() == 0);

        EXPECT_TRUE(base::instances() == 2);

        px.reset(p);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(px.get() != 0);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);

        D1 *p = new D1;
        EXPECT_TRUE(p->refcount() == 0);

        EXPECT_TRUE(base::instances() == 2);

        px.reset(p, true);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(px.get() != 0);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);

        D1 *p = new D1;
        EXPECT_TRUE(p->refcount() == 0);

        refc_ptr<D1>::policy::add_ref(p);
        EXPECT_TRUE(p->refcount() == 1);

        EXPECT_TRUE(base::instances() == 2);

        px.reset(p, false);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px->refcount() == 1);

        EXPECT_TRUE(base::instances() == 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, operator_tests)
{
    {
        refc_ptr<D1> px;
        EXPECT_TRUE(px ? false : true);
        EXPECT_TRUE(!px);
    }

    {
        refc_ptr<D1> px(nullptr);
        EXPECT_TRUE(px ? false : true);
        EXPECT_TRUE(!px);
    }

    {
        refc_ptr<D1> px(new D1);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_EQ(&*px, px.get());
        EXPECT_EQ(px.operator->(), px.get());
    }

    {
        refc_ptr<D1> px;
        D1 *detached = px.detach();
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(detached == 0);
    }

    {
        D1 *p = new D1;
        EXPECT_TRUE(p->refcount() == 0);

        refc_ptr<D1> px(p);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px->refcount() == 1);

        D1 *detached = px.detach();
        EXPECT_TRUE(px.get() == 0);

        EXPECT_TRUE(detached == p);
        EXPECT_TRUE(detached->refcount() == 1);

        delete detached;
    }
}

TYPED_TEST(refc_ptr_ste, swap)
{
    {
        refc_ptr<D1> px;
        refc_ptr<D1> px2;

        px.swap(px2);

        EXPECT_EQ(px.get(), nullptr);
        EXPECT_EQ(px2.get(), nullptr);

        using std::swap;
        swap(px, px2);

        EXPECT_EQ(px.get(), nullptr);
        EXPECT_EQ(px2.get(), nullptr);
    }

    {
        D1 *p = new D1;
        refc_ptr<D1> px;
        refc_ptr<D1> px2(p);
        refc_ptr<D1> px3(px2);

        px.swap(px2);

        EXPECT_EQ(px.get(), p);
        EXPECT_EQ(px->refcount(), 2);
        EXPECT_EQ(px2.get(), nullptr);
        EXPECT_EQ(px3.get(), p);
        EXPECT_EQ(px3->refcount(), 2);

        using std::swap;
        swap(px, px2);

        EXPECT_EQ(px.get(), nullptr);
        EXPECT_EQ(px2.get(), p);
        EXPECT_EQ(px2->refcount(), 2);
        EXPECT_EQ(px3.get(), p);
        EXPECT_EQ(px3->refcount(), 2);
    }

    {
        D1 *p1 = new D1;
        D1 *p2 = new D1;
        refc_ptr<D1> px(p1);
        refc_ptr<D1> px2(p2);
        refc_ptr<D1> px3(px2);

        px.swap(px2);

        EXPECT_TRUE(px.get() == p2);
        EXPECT_TRUE(px->refcount() == 2);
        EXPECT_TRUE(px2.get() == p1);
        EXPECT_TRUE(px2->refcount() == 1);
        EXPECT_TRUE(px3.get() == p2);
        EXPECT_TRUE(px3->refcount() == 2);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == p1);
        EXPECT_TRUE(px->refcount() == 1);
        EXPECT_TRUE(px2.get() == p2);
        EXPECT_TRUE(px2->refcount() == 2);
        EXPECT_TRUE(px3.get() == p2);
        EXPECT_TRUE(px3->refcount() == 2);
    }
}

template <class T, class U>
void test2(refc_ptr<T> const &p, refc_ptr<U> const &q)
{
    EXPECT_TRUE((p == q) == (p.get() == q.get()));
    EXPECT_TRUE((p != q) == (p.get() != q.get()));
}

template <class T> void test3(refc_ptr<T> const &p, refc_ptr<T> const &q)
{
    EXPECT_TRUE((p == q) == (p.get() == q.get()));
    EXPECT_TRUE((p.get() == q) == (p.get() == q.get()));
    EXPECT_TRUE((p == q.get()) == (p.get() == q.get()));
    EXPECT_TRUE((p != q) == (p.get() != q.get()));
    EXPECT_TRUE((p.get() != q) == (p.get() != q.get()));
    EXPECT_TRUE((p != q.get()) == (p.get() != q.get()));

    // 'less' moved here as a g++ 2.9x parse error workaround
    std::less<T *> less;
    EXPECT_TRUE((p < q) == less(p.get(), q.get()));
}

TYPED_TEST(refc_ptr_ste, compare)
{
    {
        refc_ptr<D1> px;
        test3(px, px);

        refc_ptr<D1> px2;
        test3(px, px2);

        refc_ptr<D1> px3(px);
        test3(px3, px3);
        test3(px, px3);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px;

        refc_ptr<D1> px2(new D1);
        test3(px, px2);
        test3(px2, px2);

        refc_ptr<D1> px3(new D1);
        test3(px2, px3);

        refc_ptr<D1> px4(px2);
        test3(px2, px4);
        test3(px4, px4);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D1);

        refc_ptr<D2> py(new D2);
        test2(px, py);

        refc_ptr<D1> px2(py);
        test2(px2, py);
        test3(px, px2);
        test3(px2, px2);
    }
}

TYPED_TEST(refc_ptr_ste, static_cast_test)
{
    {
        refc_ptr<D1> px(new D2);

        refc_ptr<D2> py = std::static_pointer_cast<D2>(px);
        EXPECT_TRUE(px.get() == py.get());
        EXPECT_TRUE(px->refcount() == 2);
        EXPECT_TRUE(py->refcount() == 2);

        refc_ptr<D1> px2(py);
        EXPECT_TRUE(px2.get() == px.get());
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D2> py = std::static_pointer_cast<D2>(refc_ptr<D1>(new D2));
        EXPECT_TRUE(py.get() != 0);
        EXPECT_TRUE(py->refcount() == 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, const_cast_test)
{
    {
        refc_ptr<const D1> px;

        refc_ptr<D1> px2 = std::const_pointer_cast<D1>(px);
        EXPECT_TRUE(px2.get() == 0);
    }

    {
        refc_ptr<D1> px2 = std::const_pointer_cast<D1>(refc_ptr<const D1>());
        EXPECT_TRUE(px2.get() == 0);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<const D1> px(new D1);

        refc_ptr<D1> px2 = std::const_pointer_cast<D1>(px);
        EXPECT_TRUE(px2.get() == px.get());
        EXPECT_TRUE(px2->refcount() == 2);
        EXPECT_TRUE(px->refcount() == 2);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px =
            std::const_pointer_cast<D1>(refc_ptr<const D1>(new D1));
        EXPECT_TRUE(px.get() != 0);
        EXPECT_TRUE(px->refcount() == 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, dynamic_cast_test)
{
    {
        refc_ptr<D1> px;

        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(px);
        EXPECT_TRUE(py.get() == 0);
    }

    {
        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(refc_ptr<D1>());
        EXPECT_TRUE(py.get() == 0);
    }

    {
        refc_ptr<D1> px(static_cast<D1 *>(0));

        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(px);
        EXPECT_TRUE(py.get() == 0);
    }

    {
        refc_ptr<D2> py =
            std::dynamic_pointer_cast<D2>(refc_ptr<D1>(static_cast<D1 *>(0)));
        EXPECT_TRUE(py.get() == 0);
    }

    {
        refc_ptr<D1> px(new D1);

        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(px);
        EXPECT_TRUE(py.get() == 0);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(refc_ptr<D1>(new D1));
        EXPECT_TRUE(py.get() == 0);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D2);

        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(px);
        EXPECT_TRUE(py.get() == px.get());
        EXPECT_TRUE(py->refcount() == 2);
        EXPECT_TRUE(px->refcount() == 2);
    }

    EXPECT_EQ(base::instances(), 0);

    {
        refc_ptr<D1> px(new D2);

        refc_ptr<D2> py = std::dynamic_pointer_cast<D2>(refc_ptr<D1>(new D2));
        EXPECT_TRUE(py.get() != 0);
        EXPECT_TRUE(py->refcount() == 1);
    }

    EXPECT_EQ(base::instances(), 0);
}

TYPED_TEST(refc_ptr_ste, transitive)
{
    struct Dn : public base {
        refc_ptr<Dn> next;
    };
    refc_ptr<Dn> p(new Dn);
    p->next = refc_ptr<Dn>(new Dn);
    EXPECT_TRUE(!p->next->next);
    p = p->next;
    EXPECT_TRUE(!p->next);
}

class self_ref : public base {
public:
    self_ref()
        : self_(this)
    {}

    void reset()
    {
        self_.reset();
    }

private:
    refc_ptr<self_ref> self_;
};

TYPED_TEST(refc_ptr_ste, self_reference)
{
    self_ref *ptr = new self_ref;
    EXPECT_EQ(base::instances(), 1);

    ptr->reset();
    EXPECT_EQ(base::instances(), 0);
}
