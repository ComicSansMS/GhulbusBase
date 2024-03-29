#include <gbBase/AnyInvocable.hpp>

#include <catch.hpp>

#include <functional>

namespace {
int free_function() {
    return 42;
}

struct MoveOnly {
    MoveOnly()
    {}

    MoveOnly(MoveOnly const&) = delete;
    MoveOnly& operator=(MoveOnly const&) = delete;

    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;

    int operator()() {
        return 42;
    }
};

struct CountCopies {
    mutable int count_;
    CountCopies() : count_(0) {}

    CountCopies(CountCopies const& rhs) {
        ++rhs.count_;
        count_ = rhs.count_;
    }

    CountCopies& operator=(CountCopies const& rhs) {
        ++rhs.count_;
        count_ = rhs.count_;
        return *this;
    }

    CountCopies(CountCopies&&) = default;
    CountCopies& operator=(CountCopies&&) = default;
};
}

TEST_CASE("AnyInvocable")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("Defaul Construction")
    {
        AnyInvocable<void()> f;
        CHECK(f.empty());
    }

    SECTION("Construction from free function")
    {
        std::function<int()> ff;
        AnyInvocable<int()> f(free_function);
        CHECK(f() == 42);
    }

    SECTION("Construction from move-only function")
    {
        MoveOnly mo;
        Ghulbus::AnyInvocable<int()> func(std::move(mo));
        CHECK(func() == 42);
    }

    SECTION("Construction from lambda")
    {
        Ghulbus::AnyInvocable<int()> func([]() { return 42; });
        CHECK(func() == 42);
    }

    SECTION("Construction from pointer to member function")
    {
        int (MoveOnly::* p)() = &MoveOnly::operator();
        MoveOnly mo;
        AnyInvocable<int(MoveOnly*)> func(p);
        CHECK(func(&mo) == 42);
    }

    SECTION("Movable arguments")
    {
        int expected = 0;
        auto func = [&expected](CountCopies c) { CHECK(c.count_ == expected); };
        func(CountCopies());

        expected = 1;
        {
            CountCopies c;
            func(c);
        }

        AnyInvocable<void(CountCopies)> stdfunc(func);
        expected = 0;
        {
            CountCopies c;
            stdfunc(std::move(c));
        }
    }

    SECTION("Movable return values")
    {
        int expected = 0;
        auto func = []() -> CountCopies { CountCopies c; return c; };
        {
            CountCopies ret = func();
            CHECK(ret.count_ == 0);
        }

        AnyInvocable<CountCopies()> stdfunc(func);
        {
            CountCopies ret = func();
            CHECK(ret.count_ == 0);
        }
    }

    SECTION("Move Assignment")
    {
        Ghulbus::AnyInvocable<int()> func1([]() { return 42; });
        Ghulbus::AnyInvocable<int()> func2([]() { return 0; });
        CHECK(func2() == 0);
        func2 = std::move(func1);
        CHECK(func2() == 42);
    }

    SECTION("Empty")
    {
        Ghulbus::AnyInvocable<int()> func0;
        CHECK(func0.empty());

        Ghulbus::AnyInvocable<int()> func1([]() { return 42; });
        Ghulbus::AnyInvocable<int()> func2([]() { return 0; });
        CHECK(!func1.empty());
        func2 = std::move(func1);
        CHECK(func1.empty());
    }

    SECTION("Noexcept")
    {
        Ghulbus::AnyInvocable<int()> func([]() { return 42; });
        static_assert(std::is_nothrow_move_constructible_v<decltype(func)>,
                      "Noexcept on move constructor is missing.");
    }
}
