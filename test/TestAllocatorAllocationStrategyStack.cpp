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

    //MockStorage storage;

    Allocator::Storage::Dynamic storage(70);

    Allocator::AllocationStrategy::Stack<Allocator::Storage::Dynamic, MockDebugPolicy> stack(storage);
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    SECTION("Size")
    {
        //storage.memory_size = 42;
        //
        //CHECK(stack.getFreeMemory() == 42);
    }

    SECTION("Allocate")
    {
        char* ptr1 = reinterpret_cast<char*>(stack.allocate(1, alignof(char)));
        int* ptr = reinterpret_cast<int*>(stack.allocate(5 * sizeof(int), alignof(int)));
        for(int i=0; i<5; ++i) { ptr[i] = 42; }

        stack.deallocate(reinterpret_cast<std::byte*>(ptr1), 20);
        stack.deallocate(reinterpret_cast<std::byte*>(ptr), 1);
        CHECK(stack.getFreeMemory() == 70);
    }

}
