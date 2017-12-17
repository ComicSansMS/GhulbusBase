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
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 0);
    }

    SECTION("Allocate")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        CHECK(ring.getFreeMemoryOffset() == 0);
        auto const p1 = ring.allocate(16, 1);
        CHECK(p1 == ptr + sizeof(Header));
        auto const p2 = ring.allocate(16, 1);
        CHECK(p2 == ptr + 2*sizeof(Header) + 16);
    }

    SECTION("Allocate exhaustive")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        // |------|
        auto const p1 = ring.allocate(128 - sizeof(Header), alignof(Header));
        // |( 1  )|
        CHECK(p1 == ptr + sizeof(Header));
        CHECK(ring.getFreeMemoryOffset() == 128);
        CHECK(!ring.isWrappedAround());
        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p1, 128 - sizeof(Header));
        // |------|
        CHECK(ring.getFreeMemoryOffset() == 0);
        CHECK(!ring.isWrappedAround());

        auto const p2 = ring.allocate(64 - sizeof(Header), alignof(Header));
        auto const p3 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |(2)(3)|
        CHECK(p2 == ptr + sizeof(Header));
        CHECK(p3 == ptr + sizeof(Header) + 64);
        CHECK(ring.getFreeMemoryOffset() == 128);
        CHECK(!ring.isWrappedAround());
        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p2, 64 - sizeof(Header));
        // |---(3)|
        CHECK(ring.getFreeMemoryOffset() == 128);
        CHECK(!ring.isWrappedAround());
        auto const p4 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |(4)(3)|
        CHECK(p4 == p2);
        CHECK(ring.getFreeMemoryOffset() == 64);
        CHECK(ring.isWrappedAround());

        ring.deallocate(p3, 64 - sizeof(Header));
        // |(4)---|
        CHECK(ring.getFreeMemoryOffset() == 64);
        CHECK(!ring.isWrappedAround());
        auto const p5 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |(4)(5)|
        CHECK(p5 == p3);
        CHECK(ring.getFreeMemoryOffset() == 128);
        CHECK(!ring.isWrappedAround());

        ring.deallocate(p5, 64 - sizeof(Header));
        // |(4)---|
        CHECK(ring.getFreeMemoryOffset() == 64);
        CHECK(!ring.isWrappedAround());
        auto const p6 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |(4)(6)|
        CHECK(ring.getFreeMemoryOffset() == 128);
        CHECK(!ring.isWrappedAround());
        CHECK(p6 == p5);

        ring.deallocate(p4, 64 - sizeof(Header));
        // |---(6)|
        CHECK(ring.getFreeMemoryOffset() == 128);
        CHECK(!ring.isWrappedAround());
        ring.deallocate(p6, 64 - sizeof(Header));
        // |------|
        CHECK(ring.getFreeMemoryOffset() == 0);
        CHECK(!ring.isWrappedAround());
    }

    SECTION("Allocate lost memory at end")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        // |---------------|
        auto const p1 = ring.allocate(72 - sizeof(Header), alignof(Header));
        CHECK(p1 == ptr + sizeof(Header));
        // |(  1    )------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 72);

        auto const p2 = ring.allocate(24 - sizeof(Header), alignof(Header));
        // |(  1    )(2)---|
        CHECK(p2 == ptr + sizeof(Header) + 72);
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 96);

        CHECK_THROWS_AS(ring.allocate(48 - sizeof(Header), alignof(Header)), std::bad_alloc);

        ring.deallocate(p1, 72 - sizeof(Header));
        // |---------(2)---|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 96);

        auto const p3 = ring.allocate(48 - sizeof(Header), alignof(Header));
        // |( 3  )---(2)---|
        CHECK(p3 == p1);
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 48);

        CHECK_THROWS_AS(ring.allocate(48 - sizeof(Header), alignof(Header)), std::bad_alloc);

        auto const p4 = ring.allocate(24 - sizeof(Header), alignof(Header));
        // |( 3  )(4)(2)---|
        CHECK(p4 == ptr + sizeof(Header) + 48);
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 72);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p3, 48 - sizeof(Header));
        // |------(4)(2)---|
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 72);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p2, 48 - sizeof(Header));
        // |------(4)------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 72);

        auto const p5 = ring.allocate(48 - sizeof(Header), alignof(Header));
        // |------(4)( 5  )|
        CHECK(p5 == p2);
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 120);

        auto const p6 = ring.allocate(48 - sizeof(Header), alignof(Header));
        // |( 6  )(4)( 5  )|
        CHECK(p6 == p1);
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 48);

        ring.deallocate(p5, 48 - sizeof(Header));
        // |( 6  )(4)------|
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 48);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p4, 24 - sizeof(Header));
        // |( 6  )---------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 48);

        auto const p7 = ring.allocate(72 - sizeof(Header), alignof(Header));
        // |( 6  )( 7     )|
        CHECK(p7 == p4);
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 120);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p6, 48 - sizeof(Header));
        // |------( 7     )|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 120);
        ring.deallocate(p7, 72 - sizeof(Header));
        // |---------------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 0);
    }


    SECTION("Other memory lost at end")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        // |------------|
        auto const p1 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |( 1  )------|
        CHECK(p1 == ptr + sizeof(Header));
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 64);

        auto const p2 = ring.allocate(32 - sizeof(Header), alignof(Header));
        // |( 1  )(2)---|
        CHECK(p2 == ptr + sizeof(Header) + 64);
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 96);

        ring.deallocate(p1, 64 - sizeof(Header));
        // |------(2)---|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 96);

        auto const p3 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |( 3  )(2)---|
        CHECK(p3 == p1);
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 64);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p3, 64 - sizeof(Header));
        // |------(2)---|
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 0);

        auto const p4 = ring.allocate(32 - sizeof(Header), alignof(Header));
        // |(4)---(2)---|
        CHECK(p4 == p1);
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 32);
        auto const p5 = ring.allocate(32 - sizeof(Header), alignof(Header));
        // |(4)(5)(2)---|
        CHECK(p5 == ptr + sizeof(Header) + 32);
        CHECK(ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 64);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p2, 32 - sizeof(Header));
        // |(4)(5)------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 64);

        auto const p6 = ring.allocate(64 - sizeof(Header), alignof(Header));
        // |(4)(5)( 6  )|
        CHECK(p6 == p2);
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 128);

        ring.deallocate(p5, 32 - sizeof(Header));
        // |(4)---( 6  )|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 128);

        ring.deallocate(p6, 64 - sizeof(Header));
        // |(4)---------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 32);

        ring.deallocate(p4, 32 - sizeof(Header));
        // |------------|
        CHECK(!ring.isWrappedAround());
        CHECK(ring.getFreeMemoryOffset() == 0);
    }

    SECTION("Allocate wrap-around")
    {
        std::aligned_storage_t<128, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        CHECK(ring.getFreeMemoryOffset() == 0);
        auto const p1 = ring.allocate(64, 1);
        CHECK(p1 == ptr + sizeof(Header));              // p1 [0 .. 80]
        auto const p2 = ring.allocate(32, 1);
        CHECK(p2 == ptr + 2*sizeof(Header) + 64);       // p2 [80 .. 128]

        CHECK_THROWS_AS(ring.allocate(16, 1), std::bad_alloc);
        ring.deallocate(p1, 64);
        auto const p3 = ring.allocate(16, 1);
        CHECK(p3 == ptr + sizeof(Header));              // p3 [0 .. 32]
        
        auto const p4 = ring.allocate(32, 1);
        CHECK(p4 == ptr + 2*sizeof(Header) + 16);       // p4 [32 .. 80]

        CHECK_THROWS_AS(ring.allocate(1, 1), std::bad_alloc);

        ring.deallocate(p3, 16);
        CHECK_THROWS_AS(ring.allocate(1, 1), std::bad_alloc);

        ring.deallocate(p2, 16);
        CHECK_THROWS_AS(ring.allocate(33, 1), std::bad_alloc);
        auto const p5 = ring.allocate(32, 1);
        CHECK(p5 == ptr + 3*sizeof(Header) + 48);       // p5 [80 .. 128]

        CHECK_THROWS_AS(ring.allocate(17, 1), std::bad_alloc);
        auto const p6 = ring.allocate(16, 1);
        CHECK(p6 == ptr + sizeof(Header));              // p6 [0 .. 40]

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p5, 24);
        ring.deallocate(p6, 16);
        ring.deallocate(p4, 16);
        CHECK(ring.getFreeMemoryOffset() == 0);
        CHECK(!ring.isWrappedAround());
    }

    SECTION("Zero-sized allocation")
    {
        std::aligned_storage_t<128, 16> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 128;

        auto const p1 = ring.allocate(0, 1);
        CHECK(p1 == ptr + sizeof(Header));
        CHECK(ring.getFreeMemoryOffset() == sizeof(Header));

        auto const p2 = ring.allocate(0, 1);
        CHECK(p2 == ptr + 2*sizeof(Header));
        CHECK(ring.getFreeMemoryOffset() == 2*sizeof(Header));

        auto const p3 = ring.allocate(3, 1);
        CHECK(p3 == ptr + 3*sizeof(Header));
        CHECK(ring.getFreeMemoryOffset() == 3*sizeof(Header) + 3);

        // alignment is always at least header alignment
        auto const p4 = ring.allocate(0, 1);
        CHECK(p4 == ptr + 4*sizeof(Header) + alignof(Header));
        CHECK(ring.getFreeMemoryOffset() == 4*sizeof(Header) + alignof(Header));

        // otherwise alignment is as requested
        auto const p5 = ring.allocate(0, 16);
        CHECK(p5 == ptr + 4*sizeof(Header) + 32);
        CHECK(ring.getFreeMemoryOffset() == 4*sizeof(Header) + 32);

        // 0-size allocations can exhaust memory
        auto const p6 = ring.allocate(0, 16);
        CHECK(ring.getFreeMemoryOffset() == 112);
        auto const p7 = ring.allocate(0, 16);
        CHECK(ring.getFreeMemoryOffset() == 128);

        CHECK_THROWS_AS(ring.allocate(0, 1), std::bad_alloc);

        ring.deallocate(p1, 0);
        ring.deallocate(p2, 0);
        ring.deallocate(p3, 3);
        ring.deallocate(p4, 0);
        ring.deallocate(p5, 0);
        ring.deallocate(p6, 0);
        ring.deallocate(p7, 0);
        CHECK(ring.getFreeMemoryOffset() == 0);
    }
}
