#include <gbBase/Allocator/LinearMemoryResource.hpp>

#include <catch.hpp>

namespace {
struct MockStorage {
    std::byte* memory_ptr = nullptr;
    std::size_t memory_size = 0;

    std::byte* get() noexcept {
        return memory_ptr;
    }

    std::size_t size() const noexcept {
        return memory_size;
    }
};

struct MockDebugPolicy {
    static std::size_t number_on_allocate_calls;
    static std::size_t number_on_deallocate_calls;
    static std::size_t number_on_reset_calls;

    void onAllocate(std::size_t, std::size_t, std::byte*) { ++number_on_allocate_calls; }
    void onDeallocate(std::byte*, std::size_t) { ++number_on_deallocate_calls; }
    void onReset() { ++number_on_reset_calls; }
};

std::size_t MockDebugPolicy::number_on_allocate_calls;
std::size_t MockDebugPolicy::number_on_deallocate_calls;
std::size_t MockDebugPolicy::number_on_reset_calls;
}

TEST_CASE("Linear Memory Resource")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    MockStorage storage;

    Allocator::LinearMemoryResource<MockStorage, MockDebugPolicy> lmr(storage);
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    SECTION("Size")
    {
        storage.memory_size = 42;

        CHECK(lmr.getFreeMemory() == 42);
    }

    SECTION("Allocate")
    {
        std::byte x;
        storage.memory_size = 42;
        storage.memory_ptr = &x;

        CHECK(MockDebugPolicy::number_on_allocate_calls == 0);
        CHECK(lmr.allocate(1, 1) == &x);
        CHECK(lmr.getFreeMemory() == 41);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(lmr.allocate(1, 1) == &x + 1);
        CHECK(lmr.getFreeMemory() == 40);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
    }

    SECTION("Allocate out of memory")
    {
        std::byte x;
        storage.memory_size = 4;
        storage.memory_ptr = &x;

        CHECK(lmr.allocate(1, 1) == &x);

        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(lmr.getFreeMemory() == 3);
        CHECK_THROWS_AS(lmr.allocate(4, 1), std::bad_alloc);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(lmr.getFreeMemory() == 3);
    }

    SECTION("Allocate aligned")
    {
        std::aligned_storage_t<64> as;
        storage.memory_size = 64;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&as);

        CHECK(lmr.allocate(1, 1) == reinterpret_cast<std::byte*>(&as));
        CHECK(lmr.allocate(1, 4) == reinterpret_cast<std::byte*>(&as) + 4);
        CHECK(lmr.getFreeMemory() == 59);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
        CHECK(lmr.allocate(4, 4) == reinterpret_cast<std::byte*>(&as) + 8);
        CHECK(lmr.getFreeMemory() == 52);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 3);
    }

    SECTION("Allocate of of aligned memory")
    {
        std::aligned_storage_t<8> as;
        storage.memory_size = 8;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&as);

        CHECK(lmr.allocate(5, 1) == reinterpret_cast<std::byte*>(&as));
        CHECK_THROWS_AS(lmr.allocate(1, 4), std::bad_alloc);
    }

    SECTION("Deallocate")
    {
        std::byte x;
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        lmr.deallocate(&x, 42);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
    }

    SECTION("Reset")
    {
        std::byte x;
        storage.memory_size = 42;
        storage.memory_ptr = &x;

        CHECK(lmr.allocate(1, 1) == &x);
        CHECK(lmr.getFreeMemory() == 41);

        CHECK(MockDebugPolicy::number_on_reset_calls == 0);
        lmr.reset();
        CHECK(MockDebugPolicy::number_on_reset_calls == 1);

        CHECK(lmr.getFreeMemory() == 42);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        CHECK(lmr.allocate(1, 1) == &x);
    }
}
