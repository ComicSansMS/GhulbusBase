#include <gbBase/Exception.hpp>

#include <catch.hpp>

TEST_CASE("Exception")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("GHULBUS_THROW throws exceptions")
    {
        auto tester = [](auto the_exception) {
            CHECK_THROWS_AS(GHULBUS_THROW(the_exception, "Lorem ipsum"), decltype(the_exception));
        };
        tester(Exceptions::NotImplemented());
        tester(Exceptions::IOError());
        tester(Exceptions::InvalidArgument());
        tester(Exceptions::ProtocolViolation());
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
        auto tester = [](auto the_exception) {
            std::string testtext("Lorem ipsum");
            bool was_caught = false;
            try {
                GHULBUS_THROW(the_exception, testtext);
            } catch(std::exception& e) {
                was_caught = true;
                e.what();
                auto const info = boost::get_error_info<Exception_Info::description>(e);
                REQUIRE(info);
                CHECK(*info == testtext);
            } catch(...) {
            }
            CHECK(was_caught);
        };
        tester(Exceptions::NotImplemented());
        tester(Exceptions::IOError());
        tester(Exceptions::InvalidArgument());
        tester(Exceptions::ProtocolViolation());
    }
}
