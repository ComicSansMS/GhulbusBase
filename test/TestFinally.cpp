#include <gbBase/Finally.hpp>

#include <catch.hpp>

#include <functional>
#include <memory>

namespace
{
TEST_CASE("Finalizer")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("Destruction invokes finalize function")
    {
        bool was_invoked = false;
        auto const finalize_func = [&was_invoked]() { was_invoked = true; };
        {
            Finalizer<decltype(finalize_func)> f(finalize_func);
            CHECK(!was_invoked);
        }
        CHECK(was_invoked);
    }

    SECTION("Move construction")
    {
        bool was_invoked = false;
        auto const finalize_func = [&was_invoked]() { was_invoked = true; };
        {
            std::unique_ptr<Finalizer<decltype(finalize_func)>> move_to(nullptr);
            {
                Finalizer<decltype(finalize_func)> f(finalize_func);
                CHECK(!was_invoked);
                move_to.reset(new decltype(f)(std::move(f)));
                CHECK(!was_invoked);
            }
            CHECK(!was_invoked);
        }
        CHECK(was_invoked);
    }

    SECTION("Defusing")
    {
        bool was_invoked = false;
        auto const finalize_func = [&was_invoked]() { was_invoked = true; };
        {
            Finalizer<decltype(finalize_func)> f(finalize_func);
            CHECK(!was_invoked);
            f.defuse();
        }
        CHECK(!was_invoked);
    }

    SECTION("Moving from defused")
    {
        bool was_invoked = false;
        auto const finalize_func = [&was_invoked]() { was_invoked = true; };
        {
            std::unique_ptr<Finalizer<decltype(finalize_func)>> move_to(nullptr);
            {
                Finalizer<decltype(finalize_func)> f(finalize_func);
                CHECK(!was_invoked);
                f.defuse();
                move_to.reset(new decltype(f)(std::move(f)));
                CHECK(!was_invoked);
            }
            CHECK(!was_invoked);
        }
        CHECK(!was_invoked);
    }

    SECTION("Passing through functions")
    {
        bool was_invoked = false;
        {
            auto outer = [&was_invoked]() {
                return [&was_invoked]() {
                    return [&was_invoked]() {
                        auto const finalize_func = [&was_invoked]() { was_invoked = true; };
                        return Finalizer<decltype(finalize_func)>(finalize_func);
                    }();
                }();
            }();
            CHECK(!was_invoked);
        }
        CHECK(was_invoked);
    }
}

struct FinallyTester {

    static bool check;

    static void finalize()
    {
        check = true;
    }

    int i;

    FinallyTester() : i(0) {}

    void member_finalize()
    {
        i = 42;
        finalize();
    }
};
bool FinallyTester::check;

TEST_CASE("Finally")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    FinallyTester::check = false;

    SECTION("From static function")
    {
        {
            auto f = finally(FinallyTester::finalize);
            CHECK(!FinallyTester::check);
        }
        CHECK(FinallyTester::check);
    }

    SECTION("From stateless lambda")
    {
        {
            auto f = finally([]() { FinallyTester::check = true; });
            CHECK(!FinallyTester::check);
        }
        CHECK(FinallyTester::check);
    }

    SECTION("From stateful lambda")
    {
        FinallyTester tester;
        CHECK(tester.i == 0);
        {
            auto f = finally([&tester]() { tester.member_finalize(); });
            CHECK(!FinallyTester::check);
        }
        CHECK(FinallyTester::check);
        CHECK(tester.i == 42);
    }

    SECTION("From std::bind")
    {
        FinallyTester tester;
        CHECK(tester.i == 0);
        {
            auto f = finally(std::bind(&FinallyTester::member_finalize, &tester));
            CHECK(!FinallyTester::check);
        }
        CHECK(FinallyTester::check);
        CHECK(tester.i == 42);
    }

    SECTION("From std::function")
    {
        FinallyTester tester;
        std::function<void()> func([&tester]() { tester.member_finalize(); });
        CHECK(tester.i == 0);
        {
            auto f = finally(func);
            CHECK(!FinallyTester::check);
        }
        CHECK(FinallyTester::check);
        CHECK(tester.i == 42);
    }
}
}
