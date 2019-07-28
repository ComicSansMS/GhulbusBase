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
}
