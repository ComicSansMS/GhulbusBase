#include <gbBase/Allocator/AllocationStrategyMonotonic.hpp>

#include <MockDebugPolicy.hpp>
#include <MockStorage.hpp>

#include <catch.hpp>

TEST_CASE("Monotonic Allocation Strategy")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    using testing::MockStorage;
    using testing::MockDebugPolicy;

    MockStorage storage;

    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    SECTION("Size")
    {
        storage.memory_size = 42;
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        CHECK(monot.getFreeMemory() == 42);
    }

    SECTION("Allocate")
    {
        std::byte x;
        storage.memory_size = 42;
        storage.memory_ptr = &x;
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        CHECK(MockDebugPolicy::number_on_allocate_calls == 0);
        CHECK(monot.allocate(1, 1) == &x);
        CHECK(monot.getFreeMemory() == 41);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(monot.allocate(1, 1) == &x + 1);
        CHECK(monot.getFreeMemory() == 40);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
    }

    SECTION("Allocate out of memory")
    {
        std::byte x;
        storage.memory_size = 4;
        storage.memory_ptr = &x;
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        CHECK(monot.allocate(1, 1) == &x);

        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(monot.getFreeMemory() == 3);
        CHECK_THROWS_AS(monot.allocate(4, 1), std::bad_alloc);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(monot.getFreeMemory() == 3);
    }

    SECTION("Allocate aligned")
    {
        std::aligned_storage_t<64> as;
        storage.memory_size = 64;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&as);
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        CHECK(monot.allocate(1, 1) == reinterpret_cast<std::byte*>(&as));
        CHECK(monot.allocate(1, 4) == reinterpret_cast<std::byte*>(&as) + 4);
        CHECK(monot.getFreeMemory() == 59);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
        CHECK(monot.allocate(4, 4) == reinterpret_cast<std::byte*>(&as) + 8);
        CHECK(monot.getFreeMemory() == 52);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 3);
    }

    SECTION("Allocate out of aligned memory")
    {
        std::aligned_storage_t<8> as;
        storage.memory_size = 8;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&as);
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        CHECK(monot.allocate(5, 1) == reinterpret_cast<std::byte*>(&as));
        CHECK_THROWS_AS(monot.allocate(1, 4), std::bad_alloc);
    }

    SECTION("Deallocate")
    {
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);
        std::byte x;
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        monot.deallocate(&x, 42);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
    }

    SECTION("Reset")
    {
        std::byte x;
        storage.memory_size = 42;
        storage.memory_ptr = &x;
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        CHECK(monot.allocate(1, 1) == &x);
        CHECK(monot.getFreeMemory() == 41);

        CHECK(MockDebugPolicy::number_on_reset_calls == 0);
        monot.reset();
        CHECK(MockDebugPolicy::number_on_reset_calls == 1);

        CHECK(monot.getFreeMemory() == 42);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        CHECK(monot.allocate(1, 1) == &x);
    }

    SECTION("Exhaust memory")
    {
        std::aligned_storage_t<8> as;
        storage.memory_size = 8;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&as);
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        auto p1 = monot.allocate(7, 1);
        CHECK(monot.getFreeMemory() == 1);
        CHECK(p1 == storage.memory_ptr);
        auto p2 = monot.allocate(1, 1);
        CHECK(monot.getFreeMemory() == 0);
        CHECK(p2 == storage.memory_ptr + 7);

        CHECK_THROWS_AS(monot.allocate(1, 1), std::bad_alloc);
    }

    SECTION("Zero-sized array allocation")
    {
        std::aligned_storage_t<9> as;
        storage.memory_size = 9;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&as);
        Allocator::AllocationStrategy::Monotonic<MockDebugPolicy> monot(storage);

        auto p1 = monot.allocate(4, 1);
        CHECK(monot.getFreeMemory() == 5);
        // pointers returned by allocate(0,...) must be distinct
        auto p2 = monot.allocate(0, 1);
        CHECK(p2 == storage.memory_ptr + 4);
        CHECK(monot.getFreeMemory() == 4);
        auto p3 = monot.allocate(0, 1);
        CHECK(p3 == storage.memory_ptr + 5);
        CHECK(monot.getFreeMemory() == 3);
        // alignment requirements must still be fulfilled:
        auto p4 = monot.allocate(0, 4);
        CHECK(p4 == storage.memory_ptr + 8);
        CHECK(monot.getFreeMemory() == 0);

        // 0-sized allocations can exhaust all available memory
        CHECK_THROWS_AS(monot.allocate(0, 1), std::bad_alloc);
    }
}
