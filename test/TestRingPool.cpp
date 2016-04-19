#include <gbBase/RingPool.hpp>

#include <catch.hpp>

TEST_CASE("Ring pool allocator")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("foo")
    {
        Memory::RingPool rp(1024);
        auto p1 = rp.allocate(500);
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

    SECTION("NewFallback")
    {
        Memory::RingPool_T<
            Memory::RingPoolPolicies::Policies<Memory::RingPoolPolicies::FallbackPolicies::AllocateWithGlobalNew>>
            rp(1000);
        auto p1 = rp.allocate(800);
        REQUIRE(p1);
        auto p2 = rp.allocate(800);
        CHECK(p2);
        auto p3 = rp.allocate(0);
        CHECK(p3);
        rp.free(p1);
        rp.free(p2);
        rp.free(p3);
    }
}


