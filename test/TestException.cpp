#include <gbBase/Exception.hpp>

#include <catch.hpp>

TEST_CASE("Exception")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("GHULBUS_THROW throws exceptions")
    {
        CHECK_THROWS_AS(GHULBUS_THROW(Exceptions::NotImplemented(), "Lorem ipsum"), Exceptions::NotImplemented);
    }

    SECTION("Exception decorating")
    {
        Exceptions::NotImplemented e;
        std::string testtext("Lorem ipsum");
        e << Exception_Info::description(testtext);
        auto const info = boost::get_error_info<Exception_Info::description>(e);
        REQUIRE(info);
        CHECK(*info == testtext);
    }

    SECTION("Exceptions can be caught as std exceptions")
    {
        std::string testtext("Lorem ipsum");
        bool was_caught = false;
        try {
            GHULBUS_THROW(Exceptions::NotImplemented(), testtext);
        } catch(std::exception& e) {
            was_caught = true;
            e.what();
            auto const info = boost::get_error_info<Exception_Info::description>(e);
            REQUIRE(info);
            CHECK(*info == testtext);
        } catch(...) {
        }
        CHECK(was_caught);
    }
}
