#include <gbBase/Allocator/AllocationStrategyStack.hpp>

#include <gbBase/Allocator/Storage.hpp>

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

TEST_CASE("Stack Allocation Strategy")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    MockStorage storage;

    Allocator::AllocationStrategy::Stack<MockStorage, MockDebugPolicy> stack(storage);
    using Header = decltype(stack)::Header;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    SECTION("Allocator Header")
    {
        Header h1(nullptr);
        CHECK(h1.previousHeader() == nullptr);
        CHECK(!h1.wasFreed());
        Header h2(&h1);
        CHECK(h2.previousHeader() == &h1);
        CHECK(!h2.wasFreed());

        CHECK(!h1.wasFreed());
        h1.markFree();
        CHECK(h1.previousHeader() == nullptr);
        CHECK(h2.previousHeader()->wasFreed());
        CHECK(!h2.wasFreed());
        h2.markFree();
        CHECK(h2.wasFreed());
        CHECK(h2.previousHeader() == &h1);
    }

    SECTION("Size and offset")
    {
        storage.memory_size = 42;
        CHECK(stack.getFreeMemory() == 42);
        CHECK(stack.getFreeMemoryOffset() == 0);
    }

    SECTION("Allocate")
    {
        std::aligned_storage_t<256> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 256;

        CHECK(MockDebugPolicy::number_on_allocate_calls == 0);
        std::byte* const p1 = stack.allocate(1, 1);
        Header const* const h1 = reinterpret_cast<Header const*>(p1 - sizeof(Header));
        CHECK(p1 == ptr + sizeof(Header));
        CHECK(h1->previousHeader() == nullptr);
        CHECK(!h1->wasFreed());
        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(stack.getFreeMemory() == 256 - sizeof(Header) - 1);
        CHECK(stack.getFreeMemoryOffset() == sizeof(Header) + 1);

        std::byte* const p2 = stack.allocate(3, 1);
        Header const* const h2 = reinterpret_cast<Header const*>(p2 - sizeof(Header));
        CHECK(p2 == ptr + 2*sizeof(Header) + 1                      + (alignof(Header) - 1));
        //          ^base   ^2 headers       ^previous allocation      ^padding to second header
        CHECK(h2->previousHeader() == h1);
        CHECK(!h2->wasFreed());
        CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
        CHECK(stack.getFreeMemory() == 256 - 2*sizeof(Header) - alignof(Header) - 3);
        CHECK(stack.getFreeMemoryOffset() == 2*sizeof(Header) + alignof(Header) + 3);

        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        stack.deallocate(p1, 1);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
        stack.deallocate(p2, 3);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 2);
        CHECK(stack.getFreeMemory() == 256);
    }

    SECTION("Allocate out of memory")
    {
        std::aligned_storage_t<64> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 64;

        CHECK(MockDebugPolicy::number_on_allocate_calls == 0);
        CHECK_THROWS_AS(stack.allocate(64, 1), std::bad_alloc);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 0);
    }

    SECTION("Allocate aligned")
    {
        std::aligned_storage_t<256, 16> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 256;

        std::byte* const p1 = stack.allocate(20, 16);
        Header const* const h1 = reinterpret_cast<Header const*>(p1 - sizeof(Header));
        CHECK(p1 == ptr + 16);
        CHECK(!h1->wasFreed());
        CHECK(h1->previousHeader() == nullptr);

        std::byte* const p2 = stack.allocate(4, 16);
        Header const* const h2 = reinterpret_cast<Header const*>(p2 - sizeof(Header));
        CHECK(p2 == p1 + 20      + 4       + sizeof(Header));
        //               ^p1 size  ^ padding  ^p2 header
        CHECK(!h2->wasFreed());
        CHECK(h2->previousHeader() == h1);
    }

    SECTION("Allocate out of aligned memory")
    {
        std::aligned_storage_t<64, 16> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 64;

        std::byte* const p1 = stack.allocate(20, 16);
        CHECK(p1 == ptr + 16);
        CHECK(stack.getFreeMemory() == 20 + sizeof(Header));

        CHECK_THROWS_AS(stack.allocate(20, 16), std::bad_alloc);
    }

    SECTION("Deallocate LIFO")
    {
        std::aligned_storage_t<256, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 256;

        std::byte* const p1 = stack.allocate(8, 8);
        Header const* const h1 = reinterpret_cast<Header const*>(p1 - sizeof(Header));
        std::byte* const p2 = stack.allocate(16, 8);
        Header const* const h2 = reinterpret_cast<Header const*>(p2 - sizeof(Header));
        std::byte* const p3 = stack.allocate(8, 8);
        Header const* const h3 = reinterpret_cast<Header const*>(p3 - sizeof(Header));
        std::byte* const p4 = stack.allocate(32, 8);
        Header const* const h4 = reinterpret_cast<Header const*>(p4 - sizeof(Header));

        CHECK(stack.getFreeMemory() == storage.memory_size - (4*sizeof(Header) + 64));
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        CHECK(!h4->wasFreed());
        stack.deallocate(p4, 32);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
        CHECK(h4->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size - (3*sizeof(Header) + 32));

        CHECK(!h3->wasFreed());
        stack.deallocate(p3, 8);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 2);
        CHECK(h3->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size - (2*sizeof(Header) + 24));

        CHECK(!h2->wasFreed());
        stack.deallocate(p2, 16);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 3);
        CHECK(h2->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size - (sizeof(Header) + 8));

        CHECK(!h1->wasFreed());
        stack.deallocate(p1, 8);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 4);
        CHECK(h1->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size);
    }

    SECTION("Deallocate out-of-order")
    {
        std::aligned_storage_t<256, alignof(Header)> as;
        auto ptr = reinterpret_cast<std::byte*>(&as);
        storage.memory_ptr = ptr;
        storage.memory_size = 256;

        std::byte* const p1 = stack.allocate(8, 8);
        Header const* const h1 = reinterpret_cast<Header const*>(p1 - sizeof(Header));
        std::byte* const p2 = stack.allocate(16, 8);
        Header const* const h2 = reinterpret_cast<Header const*>(p2 - sizeof(Header));
        std::byte* const p3 = stack.allocate(8, 8);
        Header const* const h3 = reinterpret_cast<Header const*>(p3 - sizeof(Header));
        std::byte* const p4 = stack.allocate(32, 8);
        Header const* const h4 = reinterpret_cast<Header const*>(p4 - sizeof(Header));

        CHECK(stack.getFreeMemory() == storage.memory_size - (4*sizeof(Header) + 64));
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        CHECK(!h2->wasFreed());
        stack.deallocate(p2, 16);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
        CHECK(h2->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size - (4*sizeof(Header) + 64));

        CHECK(!h3->wasFreed());
        stack.deallocate(p3, 8);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 2);
        CHECK(h3->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size - (4*sizeof(Header) + 64));

        CHECK(!h4->wasFreed());
        stack.deallocate(p4, 32);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 3);
        CHECK(h4->wasFreed());
        CHECK(stack.getFreeMemory() == storage.memory_size - (sizeof(Header) + 8));

        stack.deallocate(p1, 8);
        CHECK(stack.getFreeMemory() == storage.memory_size);
    }
}
