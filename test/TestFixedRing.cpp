#include <gbBase/FixedRing.hpp>

#include <catch.hpp>

TEST_CASE("Fixed Ring")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    FixedRing<int> r{ 5 };

    SECTION("Construction")
    {
        CHECK(r.capacity() == 5);
    }

    SECTION("Fill Up")
    {
        r.push_back(1);
        r.push_back(2);
        r.push_back(3);
        r.push_back(4);
        r.push_back(5);

        CHECK(r.pop_front() == 1);
        CHECK(r.pop_front() == 2);
        CHECK(r.pop_front() == 3);
        CHECK(r.pop_front() == 4);
        CHECK(r.pop_front() == 5);
    }

    SECTION("Fill up and round")
    {
        r.push_back(1);
        r.push_back(2);
        r.push_back(3);
        r.push_back(4);
        r.push_back(5);

        CHECK(r.pop_front() == 1);
        CHECK(r.pop_front() == 2);

        r.push_back(6);
        r.push_back(7);

        CHECK(r.pop_front() == 3);
        CHECK(r.pop_front() == 4);
        CHECK(r.pop_front() == 5);
        CHECK(r.pop_front() == 6);

        r.push_back(8);
        r.push_back(9);
        r.push_back(10);
        r.push_back(11);

        CHECK(r.pop_front() == 7);
        CHECK(r.pop_front() == 8);
        CHECK(r.pop_front() == 9);
        CHECK(r.pop_front() == 10);
        CHECK(r.pop_front() == 11);
    }

    SECTION("free")
    {
        CHECK(r.free() == 5);
        r.push_back(1);
        CHECK(r.free() == 4);
        r.push_back(2);
        CHECK(r.free() == 3);
        r.push_back(3);
        CHECK(r.free() == 2);
        r.push_back(4);
        CHECK(r.free() == 1);
        r.push_back(5);
        CHECK(r.free() == 0);

        CHECK(r.pop_front() == 1);
        CHECK(r.free() == 1);
        CHECK(r.pop_front() == 2);
        CHECK(r.free() == 2);

        r.push_back(6);
        CHECK(r.free() == 1);
        r.push_back(7);
        CHECK(r.free() == 0);

        CHECK(r.pop_front() == 3);
        CHECK(r.free() == 1);
        CHECK(r.pop_front() == 4);
        CHECK(r.free() == 2);
        CHECK(r.pop_front() == 5);
        CHECK(r.free() == 3);
        CHECK(r.pop_front() == 6);
        CHECK(r.free() == 4);

        r.push_back(8);
        CHECK(r.free() == 3);
        r.push_back(9);
        CHECK(r.free() == 2);
        r.push_back(10);
        CHECK(r.free() == 1);
        r.push_back(11);
        CHECK(r.free() == 0);

        CHECK(r.pop_front() == 7);
        CHECK(r.free() == 1);
        CHECK(r.pop_front() == 8);
        CHECK(r.free() == 2);
        CHECK(r.pop_front() == 9);
        CHECK(r.free() == 3);
        CHECK(r.pop_front() == 10);
        CHECK(r.free() == 4);
        CHECK(r.pop_front() == 11);
        CHECK(r.free() == 5);
    }

    SECTION("available")
    {
        CHECK(r.available() == 0);
        r.push_back(1);
        CHECK(r.available() == 1);
        r.push_back(2);
        CHECK(r.available() == 2);
        r.push_back(3);
        CHECK(r.available() == 3);
        r.push_back(4);
        CHECK(r.available() == 4);
        r.push_back(5);
        CHECK(r.available() == 5);

        CHECK(r.pop_front() == 1);
        CHECK(r.available() == 4);
        CHECK(r.pop_front() == 2);
        CHECK(r.available() == 3);

        r.push_back(6);
        CHECK(r.available() == 4);
        r.push_back(7);
        CHECK(r.available() == 5);

        CHECK(r.pop_front() == 3);
        CHECK(r.available() == 4);
        CHECK(r.pop_front() == 4);
        CHECK(r.available() == 3);
        CHECK(r.pop_front() == 5);
        CHECK(r.available() == 2);
        CHECK(r.pop_front() == 6);
        CHECK(r.available() == 1);

        r.push_back(8);
        CHECK(r.available() == 2);
        r.push_back(9);
        CHECK(r.available() == 3);
        r.push_back(10);
        CHECK(r.available() == 4);
        r.push_back(11);
        CHECK(r.available() == 5);

        CHECK(r.pop_front() == 7);
        CHECK(r.available() == 4);
        CHECK(r.pop_front() == 8);
        CHECK(r.available() == 3);
        CHECK(r.pop_front() == 9);
        CHECK(r.available() == 2);
        CHECK(r.pop_front() == 10);
        CHECK(r.available() == 1);
        CHECK(r.pop_front() == 11);
        CHECK(r.available() == 0);
    }

    SECTION("empty")
    {
        CHECK(r.empty());
        r.push_back(1);
        CHECK(!r.empty());
        r.push_back(2);
        CHECK(!r.empty());
        r.pop_front();
        CHECK(!r.empty());
        r.pop_front();
        CHECK(r.empty());
        r.push_back(3);
        r.push_back(4);
        r.push_back(5);
        r.push_back(6);
        r.push_back(7);
        CHECK(!r.empty());
        r.pop_front();
        r.pop_front();
        r.pop_front();
        r.pop_front();
        CHECK(!r.empty());
        r.pop_front();
        CHECK(r.empty());
    }

    SECTION("full")
    {
        CHECK(!r.full());
        r.push_back(1);
        CHECK(!r.full());
        r.push_back(2);
        CHECK(!r.full());
        r.push_back(3);
        CHECK(!r.full());
        r.push_back(4);
        CHECK(!r.full());
        r.push_back(5);
        CHECK(r.full());
        r.pop_front();
        CHECK(!r.full());
        r.push_back(6);
        CHECK(r.full());
        r.pop_front();
        CHECK(!r.full());
        r.pop_front();
        CHECK(!r.full());
        r.pop_front();
        CHECK(!r.full());
        r.pop_front();
        CHECK(!r.full());
        r.pop_front();
        CHECK(!r.full());
    }

    SECTION("Equality")
    {
        FixedRing<int> r1{ 5 };
        FixedRing<int> r2{ 5 };
        FixedRing<int> r3{ 3 };

        r1.push_back(1);
        r1.push_back(2);
        r1.push_back(3);

        r2.push_back(1);
        r2.push_back(2);
        r2.push_back(3);
        CHECK(r1 == r2);
        CHECK_FALSE(r1 != r2);

        r3.push_back(1);
        r3.push_back(2);
        r3.push_back(3);
        CHECK(r1 == r3);
        CHECK_FALSE(r1 != r3);

        r1.pop_front();
        CHECK_FALSE(r1 == r2);
        CHECK(r1 != r2);

        r1.pop_front();
        r1.pop_front();

        r1.push_back(1);
        r1.push_back(2);
        r1.push_back(3);
        CHECK(r1 == r2);
        CHECK_FALSE(r1 != r2);
        CHECK(r1 == r3);
        CHECK_FALSE(r1 != r3);
    }

    SECTION("Array access")
    {
        r.push_back(1);
        r.push_back(2);
        r.push_back(3);
        CHECK(r[0] == 1);
        CHECK(r[1] == 2);
        CHECK(r[2] == 3);

        r[1] = 42;
        CHECK(r[0] == 1);
        CHECK(r[1] == 42);
        CHECK(r[2] == 3);

        r.pop_front();
        CHECK(r[0] == 42);
        CHECK(r[1] == 3);

        r.push_back(4);
        r.push_back(5);
        r.push_back(6);
        CHECK(r[0] == 42);
        CHECK(r[1] == 3);
        CHECK(r[2] == 4);
        CHECK(r[3] == 5);
        CHECK(r[4] == 6);

        r.pop_front();
        CHECK(r[0] == 3);
        CHECK(r[1] == 4);
        CHECK(r[2] == 5);
        CHECK(r[3] == 6);

        r.pop_front();
        r.pop_front();
        CHECK(r[0] == 5);
        CHECK(r[1] == 6);

        r.pop_front();
        CHECK(r[0] == 6);
    }

    SECTION("Const Array access")
    {
        FixedRing<int> const& cr = r;
        r.push_back(1);
        r.push_back(2);
        r.push_back(3);
        CHECK(cr[0] == 1);
        CHECK(cr[1] == 2);
        CHECK(cr[2] == 3);

        r.pop_front();
        CHECK(cr[0] == 2);
        CHECK(cr[1] == 3);

        r.push_back(4);
        r.push_back(5);
        r.push_back(6);
        CHECK(cr[0] == 2);
        CHECK(cr[1] == 3);
        CHECK(cr[2] == 4);
        CHECK(cr[3] == 5);
        CHECK(cr[4] == 6);

        r.pop_front();
        CHECK(cr[0] == 3);
        CHECK(cr[1] == 4);
        CHECK(cr[2] == 5);
        CHECK(cr[3] == 6);

        r.pop_front();
        r.pop_front();
        CHECK(cr[0] == 5);
        CHECK(cr[1] == 6);

        r.pop_front();
        CHECK(cr[0] == 6);
    }

    SECTION("Front access")
    {
        FixedRing<int> const& cr = r;
        r.push_back(1);
        r.push_back(2);
        r.push_back(3);

        CHECK(r.front() == 1);
        CHECK(cr.front() == 1);

        r.front() = 5;
        CHECK(r.front() == 5);
        CHECK(cr.front() == 5);

        r.pop_front();
        CHECK(r.front() == 2);
        CHECK(cr.front() == 2);

        r.push_back(4);
        r.push_back(5);
        r.push_back(6);
        CHECK(r.front() == 2);
        CHECK(cr.front() == 2);

        r.pop_front();
        r.pop_front();
        r.pop_front();
        CHECK(r.front() == 5);
        CHECK(cr.front() == 5);

        r.pop_front();
        CHECK(r.front() == 6);
        CHECK(cr.front() == 6);
    }

    SECTION("Back access")
    {
        FixedRing<int> const& cr = r;
        r.push_back(1);
        CHECK(r.back() == 1);
        CHECK(cr.back() == 1);
        r.push_back(2);
        CHECK(r.back() == 2);
        CHECK(cr.back() == 2);
        r.push_back(3);
        CHECK(r.back() == 3);
        CHECK(cr.back() == 3);

        r.pop_front();
        CHECK(r.back() == 3);
        CHECK(cr.back() == 3);

        r.back() = 42;
        CHECK(r.back() == 42);
        CHECK(cr.back() == 42);

        r.push_back(4);
        CHECK(r.back() == 4);
        CHECK(cr.back() == 4);
        r.push_back(5);
        CHECK(r.back() == 5);
        CHECK(cr.back() == 5);
        r.push_back(6);
        CHECK(r.back() == 6);
        CHECK(cr.back() == 6);
        r.pop_front();
        r.push_back(7);
        CHECK(r.back() == 7);
        CHECK(cr.back() == 7);

        r.pop_front();
        CHECK(r.back() == 7);
        CHECK(cr.back() == 7);
        r.pop_front();
        CHECK(r.back() == 7);
        CHECK(cr.back() == 7);
        r.pop_front();
        CHECK(r.back() == 7);
        CHECK(cr.back() == 7);
        r.pop_front();
        CHECK(r.back() == 7);
        CHECK(cr.back() == 7);
    }

}
