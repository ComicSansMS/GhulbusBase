#include <gbBase/RingPool.hpp>

#include <catch.hpp>

TEST_CASE("Ring pool allocator")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("foo")
    {
        Memory::RingPool rp(1026);
        auto p1 = rp.allocate(501);
        REQUIRE(p1);
        auto p2 = rp.allocate(500);
        REQUIRE(p2);
        CHECK(!rp.allocate(500));
        rp.free(p1);
        auto p3 = rp.allocate(500);
        REQUIRE(p3);
        rp.free(p3);
        rp.free(p2);
        rp.free(rp.allocate(5));
    }
}


