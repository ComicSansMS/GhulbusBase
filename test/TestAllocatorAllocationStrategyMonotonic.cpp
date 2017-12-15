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

    Allocator::AllocationStrategy::Monotonic<MockStorage, MockDebugPolicy> monot(storage);
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    SECTION("Size")
    {
        storage.memory_size = 42;

        CHECK(monot.getFreeMemory() == 42);
    }

    SECTION("Allocate")
    {
        std::byte x;
        storage.memory_size = 42;
        storage.memory_ptr = &x;

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

        CHECK(monot.allocate(5, 1) == reinterpret_cast<std::byte*>(&as));
        CHECK_THROWS_AS(monot.allocate(1, 4), std::bad_alloc);
    }

    SECTION("Deallocate")
    {
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

        CHECK(monot.allocate(1, 1) == &x);
        CHECK(monot.getFreeMemory() == 41);

        CHECK(MockDebugPolicy::number_on_reset_calls == 0);
        monot.reset();
        CHECK(MockDebugPolicy::number_on_reset_calls == 1);

        CHECK(monot.getFreeMemory() == 42);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        CHECK(monot.allocate(1, 1) == &x);
    }
}
