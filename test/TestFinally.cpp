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
        int was_invoked = 0;
        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
        {
            Finalizer<decltype(finalize_func)> f(finalize_func);
            CHECK(was_invoked == 0);
        }
        CHECK(was_invoked == 1);
    }

    SECTION("Move construction")
    {
        int was_invoked = 0;
        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
        {
            std::unique_ptr<Finalizer<decltype(finalize_func)>> move_to(nullptr);
            {
                Finalizer<decltype(finalize_func)> f(finalize_func);
                CHECK(was_invoked == 0);
                move_to.reset(new decltype(f)(std::move(f)));
                CHECK(was_invoked == 0);
            }
            CHECK(was_invoked == 0);
        }
        CHECK(was_invoked == 1);
    }

    SECTION("Defusing")
    {
        int was_invoked = 0;
        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
        {
            Finalizer<decltype(finalize_func)> f(finalize_func);
            CHECK(was_invoked == 0);
            f.defuse();
        }
        CHECK(was_invoked == 0);
    }

    SECTION("Moving from defused")
    {
        int was_invoked = 0;
        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
        {
            std::unique_ptr<Finalizer<decltype(finalize_func)>> move_to(nullptr);
            {
                Finalizer<decltype(finalize_func)> f(finalize_func);
                CHECK(was_invoked == 0);
                f.defuse();
                move_to.reset(new decltype(f)(std::move(f)));
                CHECK(was_invoked == 0);
            }
            CHECK(was_invoked == 0);
        }
        CHECK(was_invoked == 0);
    }

    SECTION("Passing through functions")
    {
        int was_invoked = 0;
        {
            auto outer = [&was_invoked]() {
                return [&was_invoked]() {
                    return [&was_invoked]() {
                        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
                        return Finalizer<decltype(finalize_func)>(finalize_func);
                    }();
                }();
            }();
            CHECK(was_invoked == 0);
        }
        CHECK(was_invoked == 1);
    }
}

TEST_CASE("AnyFinalizer")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("Default construction")
    {
        AnyFinalizer af;
        CHECK(!af);
    }

    SECTION("Move construction")
    {
        int was_invoked = 0;
        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
        {
            AnyFinalizer af;
            {
                Finalizer<decltype(finalize_func)> f(finalize_func);
                af = std::move(f);
            }
            CHECK(af);
            CHECK(was_invoked == 0);
        }
        CHECK(was_invoked == 1);
    }

    SECTION("Move assignment")
    {
        int was_invoked_0 = 0;
        auto const finalize_func_0 = [&was_invoked_0]() { ++was_invoked_0; };
        int was_invoked_1 = 0;
        auto const finalize_func_1 = [&was_invoked_1]() { ++was_invoked_1; };
        {
            AnyFinalizer af(Finalizer<decltype(finalize_func_0)>{finalize_func_0});
            {
                AnyFinalizer af_inner{ Finalizer<decltype(finalize_func_1)>{finalize_func_1} };
                CHECK(was_invoked_0 == 0);
                CHECK(was_invoked_1 == 0);
                af = std::move(af_inner);
                CHECK(was_invoked_0 == 1);
                CHECK(was_invoked_1 == 0);
                CHECK(!af_inner);
            }
            CHECK(was_invoked_1 == 0);
        }
        CHECK(was_invoked_1 == 1);
    }

    SECTION("Defusing")
    {
        int was_invoked = 0;
        auto const finalize_func = [&was_invoked]() { ++was_invoked; };
        {
            AnyFinalizer af{ Finalizer<decltype(finalize_func)>(finalize_func) };;
            CHECK(was_invoked == 0);
            af.defuse();
        }
        CHECK(was_invoked == 0);
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
