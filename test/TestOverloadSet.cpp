#include <gbBase/OverloadSet.hpp>

#include <catch.hpp>

TEST_CASE("Overload Set")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    int hit_int = 0;
    int hit_float = 0;
    int hit_bool = 0;
    int hit_other = 0;
    auto const overload = OverloadSet{
        [&hit_int](int) { ++hit_int; },
        [&hit_float](float) { ++hit_float; },
        [&hit_bool](bool) { ++hit_bool; },
        [&hit_other](auto) { ++hit_other; },
    };

    CHECK(hit_int == 0);
    CHECK(hit_float == 0);
    CHECK(hit_bool == 0);
    CHECK(hit_other == 0);
    overload(42);
    CHECK(hit_int == 1);
    CHECK(hit_float == 0);
    CHECK(hit_bool == 0);
    CHECK(hit_other == 0);
    overload(42.f);
    CHECK(hit_int == 1);
    CHECK(hit_float == 1);
    CHECK(hit_bool == 0);
    CHECK(hit_other == 0);
    overload(false);
    CHECK(hit_int == 1);
    CHECK(hit_float == 1);
    CHECK(hit_bool == 1);
    CHECK(hit_other == 0);
    overload(42u);
    CHECK(hit_int == 1);
    CHECK(hit_float == 1);
    CHECK(hit_bool == 1);
    CHECK(hit_other == 1);
    overload(nullptr);
    CHECK(hit_int == 1);
    CHECK(hit_float == 1);
    CHECK(hit_bool == 1);
    CHECK(hit_other == 2);
    overload(42.0);
    CHECK(hit_int == 1);
    CHECK(hit_float == 1);
    CHECK(hit_bool == 1);
    CHECK(hit_other == 3);
    overload(-1);
    CHECK(hit_int == 2);
    CHECK(hit_float == 1);
    CHECK(hit_bool == 1);
    CHECK(hit_other == 3);
}
