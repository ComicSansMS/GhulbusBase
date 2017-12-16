#include <gbBase/Allocator/DebugPolicy.hpp>

#include <gbBase/Assert.hpp>

#include <MockDebugPolicy.hpp>

#include <catch.hpp>

#include <array>

namespace {
struct NoReturn {};

[[noreturn]] void testAssertHandler(GHULBUS_BASE_NAMESPACE::Assert::HandlerParameters const&) {
    throw NoReturn{};
}
}

TEST_CASE("Debug Policy No Debug")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    Allocator::DebugPolicy::NoDebug pol;

    SECTION("Policy does not carry any state")
    {
        CHECK(sizeof(pol) == sizeof(char));
    }

    SECTION("All operations are no-ops")
    {
        pol.onAllocate(0, 0, nullptr);
        pol.onDeallocate(nullptr, 0);
        pol.onReset();
    }
}

TEST_CASE("Debug Policy Count Calls")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    CHECK(Assert::getAssertionHandler() == &Assert::failAbort);
    Assert::setAssertionHandler(testAssertHandler);

    Allocator::DebugPolicy::AllocateDeallocateCounter pol;

    SECTION("Policy carries only a single counter")
    {
        CHECK(sizeof(pol) == sizeof(std::size_t));
    }

    SECTION("Counting of allocate deallocate calls")
    {
        CHECK(pol.getCount() == 0);
        pol.onAllocate(1, 1, nullptr);
        CHECK(pol.getCount() == 1);
        pol.onAllocate(23, 42, nullptr);
        CHECK(pol.getCount() == 2);
        pol.onDeallocate(nullptr, 0);
        CHECK(pol.getCount() == 1);
        pol.onDeallocate(nullptr, 0);
        CHECK(pol.getCount() == 0);
    }

    SECTION("Assert when resetting without deallocating")
    {
        pol.onReset();
        pol.onAllocate(1, 2, nullptr);
        bool did_throw = false;
        try { pol.onReset(); } catch(NoReturn const&) { did_throw = true; };
        CHECK(did_throw);
        pol.onDeallocate(nullptr, 0);
        pol.onReset();
    }

    Assert::setAssertionHandler(Assert::failAbort);
}

TEST_CASE("Debug Policy Tracking")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    CHECK(Assert::getAssertionHandler() == &Assert::failAbort);
    Assert::setAssertionHandler(testAssertHandler);

    Allocator::DebugPolicy::AllocateDeallocateTracking pol;

    using PolicyRecord = Allocator::DebugPolicy::AllocateDeallocateTracking::Record;

    auto records_are_equal = [](PolicyRecord const& lhs, PolicyRecord const& rhs) -> bool {
        return (lhs.pointer   == rhs.pointer)   &&
               (lhs.alignment == rhs.alignment) &&
               (lhs.size      == rhs.size)      &&
               (lhs.id        == rhs.id);
    };

    std::byte x, y, z;
    auto ptr1 = &x, ptr2 = &y, ptr3 = &z;

    SECTION("Tracking of allocate deallocate calls")
    {
        CHECK(pol.getRecords().empty());

        pol.onAllocate(5, 10, ptr1);
        std::vector<PolicyRecord> expected_records;
        PolicyRecord rec1;
        rec1.pointer = ptr1;
        rec1.alignment = 10;
        rec1.size = 5;
        rec1.id = 0;
        expected_records.push_back(rec1);
        auto check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onDeallocate(ptr1, 5);
        CHECK(pol.getRecords().empty());

        pol.onAllocate(20, 22, ptr1);
        rec1.pointer = ptr1;
        rec1.alignment = 22;
        rec1.size = 20;
        rec1.id = 1;
        expected_records.clear();
        expected_records.push_back(rec1);
        check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onAllocate(7, 87, ptr2);
        rec1.pointer = ptr2;
        rec1.alignment = 87;
        rec1.size = 7;
        rec1.id = 2;
        expected_records.push_back(rec1);
        check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onDeallocate(ptr1, 20);
        expected_records.erase(expected_records.begin());
        check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onAllocate(2978, 448, ptr3);
        rec1.pointer = ptr3;
        rec1.alignment = 448;
        rec1.size = 2978;
        rec1.id = 3;
        expected_records.push_back(rec1);
        check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onDeallocate(ptr3, 2978);
        expected_records.pop_back();
        check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onDeallocate(ptr2, 7);
        CHECK(pol.getRecords().empty());
    }

    SECTION("Assert on double alloc")
    {
        pol.onAllocate(20, 22, ptr1);
        bool did_throw = false;
        try { pol.onAllocate(20, 22, ptr1); } catch (NoReturn const&) { did_throw = true; }
        CHECK(did_throw);

        pol.onDeallocate(ptr1, 20);
    }

    SECTION("Assert on unknown dealloc")
    {
        pol.onAllocate(20, 22, ptr1);
        bool did_throw = false;
        try { pol.onDeallocate(ptr2, 20); } catch (NoReturn const&) { did_throw = true; }
        CHECK(did_throw);

        pol.onDeallocate(ptr1, 20);
    }

    SECTION("Assert on mismatching size in dealloc")
    {
        pol.onAllocate(20, 22, ptr1);
        bool did_throw = false;
        try { pol.onDeallocate(ptr1, 1); } catch (NoReturn const&) { did_throw = true; }
        CHECK(did_throw);

        pol.onDeallocate(ptr1, 20);
    }

    SECTION("Assert when resetting without deallocating")
    {
        pol.onReset();

        pol.onAllocate(20, 22, ptr1);
        bool did_throw = false;
        try { pol.onReset(); } catch (NoReturn const&) { did_throw = true; }
        CHECK(did_throw);

        pol.onDeallocate(ptr1, 20);
        pol.onReset();
    }

    Assert::setAssertionHandler(Assert::failAbort);
}

TEST_CASE("Debug Policy Combined Policy")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    using testing::MockDebugPolicy;

    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;

    Allocator::DebugPolicy::CombinedPolicy<MockDebugPolicy> pol_single;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;
    pol_single.onAllocate(0, 0, nullptr);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
    CHECK(MockDebugPolicy::number_on_reset_calls == 0);
    pol_single.onDeallocate(nullptr, 0);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
    CHECK(MockDebugPolicy::number_on_reset_calls == 0);
    pol_single.onReset();
    CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
    CHECK(MockDebugPolicy::number_on_reset_calls == 1);

    Allocator::DebugPolicy::CombinedPolicy<MockDebugPolicy, MockDebugPolicy> pol_double;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;
    pol_double.onAllocate(0, 0, nullptr);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
    CHECK(MockDebugPolicy::number_on_reset_calls == 0);
    pol_double.onDeallocate(nullptr, 0);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 2);
    CHECK(MockDebugPolicy::number_on_reset_calls == 0);
    pol_double.onReset();
    CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 2);
    CHECK(MockDebugPolicy::number_on_reset_calls == 2);


    Allocator::DebugPolicy::CombinedPolicy<MockDebugPolicy, MockDebugPolicy, MockDebugPolicy> pol_triple;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;
    pol_triple.onAllocate(0, 0, nullptr);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 3);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
    CHECK(MockDebugPolicy::number_on_reset_calls == 0);
    pol_triple.onDeallocate(nullptr, 0);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 3);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 3);
    CHECK(MockDebugPolicy::number_on_reset_calls == 0);
    pol_triple.onReset();
    CHECK(MockDebugPolicy::number_on_allocate_calls == 3);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 3);
    CHECK(MockDebugPolicy::number_on_reset_calls == 3);

    Allocator::DebugPolicy::CombinedPolicy<MockDebugPolicy, Allocator::DebugPolicy::AllocateDeallocateCounter> pol1;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    pol1.onAllocate(0, 0, nullptr);
    CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
    CHECK(pol1.getContainedPolicy<1>().getCount() == 1);
    pol1.onDeallocate(nullptr, 0);
    CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
    CHECK(pol1.getContainedPolicy<1>().getCount() == 0);
}

TEST_CASE("Debug Policy Debug Heap")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    Allocator::DebugPolicy::DebugHeap pol;

    std::array<std::byte, 12> mem;
    mem.fill(std::byte(0));

    pol.onAllocate(10, 1, mem.data() + 1);
    CHECK(mem[0] == std::byte(0x00));
    for(int i=0; i<10; ++i) {
        CHECK(mem[1 + i] == std::byte(0xcd));
    }
    CHECK(mem[11] == std::byte(0x00));

    pol.onDeallocate(mem.data() + 1, 10);
    CHECK(mem[0] == std::byte(0x00));
    for(int i=0; i<10; ++i) {
        CHECK(mem[1 + i] == std::byte(0xdd));
    }
    CHECK(mem[11] == std::byte(0x00));
}
