#include <gbBase/Allocator/AllocationStrategyRing.hpp>

#include <gbBase/Allocator/Storage.hpp>

#include <MockDebugPolicy.hpp>
#include <MockStorage.hpp>

#include <catch.hpp>

#include <type_traits>

TEST_CASE("Ring Allocation Strategy")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    using testing::MockDebugPolicy;
    using testing::MockStorage;

    MockStorage storage;

    Allocator::AllocationStrategy::Ring<MockStorage, MockDebugPolicy> ring(storage);
    using Header = decltype(ring)::Header;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    std::size_t const free_marker = std::numeric_limits<std::size_t>::max();

    SECTION("Allocator Header")
    {
        Header h1(nullptr);
        CHECK(h1.previousHeader() == nullptr);
        CHECK(h1.nextHeader() == nullptr);
        CHECK(!h1.wasFreed());

        Header h2(&h1);
        CHECK(h2.previousHeader() == &h1);
        CHECK(h2.nextHeader() == nullptr);
        CHECK(!h2.wasFreed());

        Header h3(nullptr);
        h2.setNextHeader(&h3);
        CHECK(h2.nextHeader() == &h3);
        h2.clearNextHeader();
        CHECK(h2.nextHeader() == nullptr);

        CHECK(h2.previousHeader() == &h1);
        h2.clearPreviousHeader();
        CHECK(h2.previousHeader() == nullptr);

        CHECK(!h2.wasFreed());
        h2.markFree();
        CHECK(h2.wasFreed());
    }


    SECTION("Size and offset")
    {
        std::byte x;
        storage.memory_ptr = &x;
        storage.memory_size = 42;
        CHECK(ring.getFreeMemoryOffset() == 0);
        CHECK(ring.getFreeMemoryContiguous(0) == 42);
    }

    SECTION("Allocate")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        CHECK(ring.getFreeMemoryOffset() == 0);
        auto* const p1 = ring.allocate(16, 1);
        CHECK(p1 == ptr + sizeof(Header));
        auto const* p2 = ring.allocate(16, 1);
        CHECK(p2 == ptr + 2*sizeof(Header) + 16);
    }

    SECTION("Allocate wrap-around")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        CHECK(ring.getFreeMemoryOffset() == 0);
        auto* const p1 = ring.allocate(64, 1);
        CHECK(p1 == ptr + sizeof(Header));              // p1 [0 .. 80]
        auto* const p2 = ring.allocate(32, 1);
        CHECK(p2 == ptr + 2*sizeof(Header) + 64);       // p2 [80 .. 128]

        CHECK_THROWS_AS(ring.allocate(16, 1), std::bad_alloc);
        ring.deallocate(p1, 64);
        auto* const p3 = ring.allocate(16, 1);
        CHECK(p3 == ptr + sizeof(Header));              // p3 [0 .. 32]
        
        auto* const p4 = ring.allocate(32, 1);
        CHECK(p4 == ptr + 2*sizeof(Header) + 16);       // p4 [32 .. 80]

        CHECK_THROWS_AS(ring.allocate(1, 1), std::bad_alloc);

        ring.deallocate(p3, 16);
        CHECK_THROWS_AS(ring.allocate(1, 1), std::bad_alloc);

        ring.deallocate(p2, 16);
        CHECK_THROWS_AS(ring.allocate(33, 1), std::bad_alloc);
        auto* const p5 = ring.allocate(32, 1);
        CHECK(p5 == ptr + 3*sizeof(Header) + 48);       // p5 [80 .. 128]

        CHECK_THROWS_AS(ring.allocate(17, 1), std::bad_alloc);
        auto* const p6 = ring.allocate(16, 1);
        CHECK(p6 == ptr + sizeof(Header));              // p6 [0 .. 40]

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p5, 24);
        ring.deallocate(p6, 16);
        ring.deallocate(p4, 16);
        CHECK(ring.getFreeMemoryOffset() == 0);
        CHECK(ring.getFreeMemoryContiguous(0) == 128);
    }
}
