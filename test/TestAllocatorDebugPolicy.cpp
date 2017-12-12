#include <gbBase/Allocators/DebugPolicy.hpp>

#include <gbBase/Assert.hpp>

#include <catch.hpp>

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
               (lhs.n         == rhs.n)         &&
               (lhs.count     == rhs.count);
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
        rec1.n = 5;
        rec1.count = 0;
        expected_records.push_back(rec1);
        auto check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onDeallocate(ptr1, 5);
        CHECK(pol.getRecords().empty());

        pol.onAllocate(20, 22, ptr1);
        rec1.pointer = ptr1;
        rec1.alignment = 22;
        rec1.n = 20;
        rec1.count = 1;
        expected_records.clear();
        expected_records.push_back(rec1);
        check_records = pol.getRecords();
        CHECK(std::equal(begin(check_records), end(check_records), begin(expected_records), records_are_equal));

        pol.onAllocate(7, 87, ptr2);
        rec1.pointer = ptr2;
        rec1.alignment = 87;
        rec1.n = 7;
        rec1.count = 2;
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
        rec1.n = 2978;
        rec1.count = 3;
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
