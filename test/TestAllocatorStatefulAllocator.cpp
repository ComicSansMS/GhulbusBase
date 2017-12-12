#include <gbBase/Allocator/StatefulAllocator.hpp>

#include <gbBase/Assert.hpp>

#include <catch.hpp>

namespace {
struct MockState {
    std::byte* allocate_return_value;
    int number_allocate_calls;
    struct AllocateCallParams {
        std::size_t n;
        std::size_t alignment;
    } last_allocate_call;

    int number_deallocate_calls;
    struct DeallocateCallParams {
        std::byte* p;
        std::size_t n;
    } last_deallocate_call;

    MockState() : allocate_return_value(nullptr), number_allocate_calls(0), number_deallocate_calls(0)
    {}

    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        ++number_allocate_calls;
        last_allocate_call.n = n;
        last_allocate_call.alignment = alignment;
        return allocate_return_value;
    }

    void deallocate(std::byte* p, std::size_t n)
    {
        ++number_deallocate_calls;
        last_deallocate_call.p = p;
        last_deallocate_call.n = n;
    }
};
}

TEST_CASE("Stateful Allocator")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("Construction")
    {
        MockState state;

        Allocator::StatefulAllocator<std::byte, MockState> alloc(state);
        CHECK(alloc.getState() == &state);
    }

    SECTION("Copy Construction")
    {
        MockState state;

        Allocator::StatefulAllocator<std::byte, MockState> alloc(state);
        Allocator::StatefulAllocator<std::byte, MockState> alloc2(alloc);

        CHECK(alloc.getState() == alloc2.getState());
    }

    SECTION("Rebind construction")
    {
        MockState state;

        Allocator::StatefulAllocator<std::byte, MockState> alloc(state);
        std::allocator_traits<decltype(alloc)>::rebind_alloc<int> alloc2(alloc);

        CHECK(alloc.getState() == alloc2.getState());
    }

    SECTION("Equality")
    {
        MockState s1;
        MockState s2;

        Allocator::StatefulAllocator<std::byte, MockState> alloc1(s1);
        Allocator::StatefulAllocator<std::byte, MockState> alloc2(s1);
        Allocator::StatefulAllocator<std::byte, MockState> alloc3(s2);

        CHECK(alloc1 == alloc1);
        CHECK(!(alloc1 != alloc1));

        CHECK(alloc1 == alloc2);
        CHECK(!(alloc1 != alloc2));
        CHECK(alloc2 == alloc1);
        CHECK(!(alloc2 != alloc1));

        CHECK(!(alloc1 == alloc3));
        CHECK(alloc1 != alloc3);
        CHECK(!(alloc3 == alloc1));
        CHECK(alloc3 != alloc1);
        CHECK(!(alloc2 == alloc3));
        CHECK(alloc2 != alloc3);
        CHECK(!(alloc3 == alloc2));
        CHECK(alloc3 != alloc2);
    }

    SECTION("Copy assignment")
    {
        MockState s1;
        MockState s2;

        Allocator::StatefulAllocator<std::byte, MockState> alloc1(s1);
        Allocator::StatefulAllocator<std::byte, MockState> alloc2(s2);

        CHECK(alloc1 != alloc2);

        alloc2 = alloc1;
        CHECK(alloc2.getState() == &s1);
        CHECK(alloc1 == alloc2);
    }

    SECTION("Allocate calls get forwarded to underlying state")
    {
        MockState state;

        Allocator::StatefulAllocator<std::byte, MockState> alloc(state);
        CHECK(alloc.getState() == &state);
        CHECK(state.number_allocate_calls == 0);

        std::byte x;
        state.allocate_return_value = &x;
        CHECK(alloc.allocate(42) == &x);

        CHECK(state.number_allocate_calls == 1);
        CHECK(state.last_allocate_call.n == 42);
        CHECK(state.last_allocate_call.alignment == 1);
    }

    SECTION("Alignment is determined from type template parameter")
    {
        struct alignas(4) AlignedType {};

        MockState state;

        Allocator::StatefulAllocator<AlignedType, MockState> alloc(state);
        CHECK(alloc.getState() == &state);
        CHECK(state.number_allocate_calls == 0);

        AlignedType x;
        state.allocate_return_value = reinterpret_cast<std::byte*>(&x);
        CHECK(alloc.allocate(42) == &x);

        CHECK(state.number_allocate_calls == 1);
        CHECK(state.last_allocate_call.n == 42 * sizeof(AlignedType));
        CHECK(state.last_allocate_call.alignment == 4);
    }

    SECTION("Allocate void has no alignment requirement")
    {
        MockState state;
        Allocator::StatefulAllocator<void, MockState> alloc(state);
        CHECK(alloc.getState() == &state);
        CHECK(state.number_allocate_calls == 0);

        std::byte x;
        state.allocate_return_value = &x;
        CHECK(alloc.allocate(42) == &x);

        CHECK(state.number_allocate_calls == 1);
        CHECK(state.last_allocate_call.n == 42);
        CHECK(state.last_allocate_call.alignment == 1);
    }

    SECTION("Deallocate calls get forwarded to underlying state")
    {
        MockState state;

        Allocator::StatefulAllocator<double, MockState> alloc(state);
        CHECK(alloc.getState() == &state);
        CHECK(state.number_deallocate_calls == 0);

        double x;
        alloc.deallocate(&x, 42);
        CHECK(state.number_deallocate_calls == 1);
        CHECK(state.last_deallocate_call.p == reinterpret_cast<std::byte*>(&x));
        CHECK(state.last_deallocate_call.n == 42);
    }
}
