#include <gbBase/Exception.hpp>

#include <catch.hpp>

#include <string>

namespace {

struct TestRecord {
    int i;
    std::string s;

    TestRecord(int ii, std::string ss)
        :i(ii), s(ss)
    {}
};

struct to_string_printable {};

std::string to_string(to_string_printable) {
    return "to_string_printable";
}

struct ostream_printable {};

std::ostream& operator<<(std::ostream& os, ostream_printable) {
    return os << "ostream_printable";
}

struct unprintable {};

struct tag_test_tag {};
struct tag_is_std_string {};
struct tag_to_string_printable {};
struct tag_ostream_printable {};
struct tag_unprintable {};

using InfoTestInfo = GHULBUS_BASE_NAMESPACE::ErrorInfo<tag_test_tag, TestRecord>;
using InfoIsStdString = GHULBUS_BASE_NAMESPACE::ErrorInfo<tag_is_std_string, std::string>;
using InfoToStringPrintable = GHULBUS_BASE_NAMESPACE::ErrorInfo<tag_to_string_printable, to_string_printable>;
using InfoOstreamPrintable = GHULBUS_BASE_NAMESPACE::ErrorInfo<tag_ostream_printable, ostream_printable>;
using InfoUnprintable = GHULBUS_BASE_NAMESPACE::ErrorInfo<tag_unprintable, unprintable>;
}

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
        auto const info = getErrorInfo<Exception_Info::description>(e);
        REQUIRE(info);
        CHECK(*info == testtext);
    }

    SECTION("Exception Message - std::string Types")
    {
        Exceptions::NotImplemented e;
        std::string testtext("Lorem ipsum");
        e << InfoIsStdString(testtext);
        std::string const info = getDiagnosticMessage(e);
        INFO(info);
        CHECK(info.find(testtext) != std::string::npos);
    }

    SECTION("Exception Message - to_string Printable Types")
    {
        Exceptions::NotImplemented e;
        e << InfoToStringPrintable();
        std::string const info = getDiagnosticMessage(e);
        INFO(info);
        CHECK(info.find(to_string(to_string_printable{})) != std::string::npos);
    }

    SECTION("Exception Message - ostream Printable Types")
    {
        Exceptions::NotImplemented e;
        e << InfoOstreamPrintable();
        std::string const info = getDiagnosticMessage(e);
        INFO(info);
        CHECK(info.find("ostream_printable") != std::string::npos);
    }

    SECTION("Exception Message - Unprintable Types")
    {
        Exceptions::NotImplemented e;
        e << InfoUnprintable();
        std::string const info = getDiagnosticMessage(e);
        INFO(info);
        // result is implementation-defined here, so there's no reasonable check for this
        CHECK(!info.empty());
    }

    SECTION("Exception decorating through decorate_exception and operator<<")
    {
        std::string const testtext("Lorem ipsum");
        auto const e = decorate_exception(Exceptions::NotImplemented(), Exception_Info::description(testtext));
        auto const info = getErrorInfo<Exception_Info::description>(e);
        REQUIRE(info);
        CHECK(*info == testtext);

        auto const nothing_there = getErrorInfo<Exception_Info::filename>(e);
        CHECK(nothing_there == nullptr);

        std::string const testfile("testfile.txt");
        e << Exception_Info::filename(testfile) << InfoTestInfo(42, "blablub");
        INFO(e.what());
        auto const filename = getErrorInfo<Exception_Info::filename>(e);
        REQUIRE(filename);
        CHECK(*filename == testfile);
        auto const test_record = getErrorInfo<InfoTestInfo>(e);
        REQUIRE(test_record);
        CHECK(test_record->i == 42);
        CHECK(test_record->s == "blablub");

        std::exception x;
        auto const not_an_exception = getErrorInfo<Exception_Info::description>(x);
        CHECK(not_an_exception == nullptr);
    }

    SECTION("Decorating with location is a special case")
    {
        Exceptions::NotImplemented e;
        std::string const file("testfile.txt");
        std::string const func("testfunc.txt");
        long const line = 42;
        e << Exception_Info::location(file.c_str(), func.c_str(), line);

        auto const loc = getErrorInfo<Exception_Info::location>(e);
        REQUIRE(loc);
        CHECK(loc->file == file);
        CHECK(loc->function == func);
        CHECK(loc->line == line);
    }

    SECTION("Decorating with description is a special case")
    {
        Exceptions::NotImplemented e;
        std::string const testtext("lorem ipsum");
        e << Exception_Info::description(testtext);

        auto const desc = getErrorInfo<Exception_Info::description>(e);
        REQUIRE(desc);
        CHECK(*desc == testtext);
    }

    SECTION("Copy construction - built-ins only")
    {
        std::string const testtext("Lorem ipsum");
        std::string const testfile("awesome_source.cpp");
        std::string const testfunc("ultimate_test_function_2k(int, float, long)");
        auto const e = decorate_exception(Exceptions::NotImplemented(),
                                          Exception_Info::description(testtext),
                                          Exception_Info::location(testfile.c_str(), testfunc.c_str(), 42));
        Exceptions::NotImplemented e2(e);
        REQUIRE(getErrorInfo<Exception_Info::description>(e));
        CHECK(*getErrorInfo<Exception_Info::description>(e) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::description>(e2));
        CHECK(*getErrorInfo<Exception_Info::description>(e2) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::location>(e));
        CHECK(getErrorInfo<Exception_Info::location>(e)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::location>(e2));
        CHECK(getErrorInfo<Exception_Info::location>(e2)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->line == 42);
    }

    SECTION("Copy construction - decorated")
    {
        std::string const testtext("Lorem ipsum");
        std::string const testfile("awesome_source.cpp");
        std::string const testfunc("ultimate_test_function_2k(int, float, long)");
        auto const e = decorate_exception(Exceptions::NotImplemented(),
                                          Exception_Info::description(testtext),
                                          Exception_Info::location(testfile.c_str(), testfunc.c_str(), 42),
                                          Exception_Info::filename(testfile),
                                          InfoTestInfo(23, "fooberella"));
        Exceptions::NotImplemented e2(e);
        REQUIRE(getErrorInfo<Exception_Info::description>(e));
        CHECK(*getErrorInfo<Exception_Info::description>(e) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::description>(e2));
        CHECK(*getErrorInfo<Exception_Info::description>(e2) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::location>(e));
        CHECK(getErrorInfo<Exception_Info::location>(e)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::location>(e2));
        CHECK(getErrorInfo<Exception_Info::location>(e2)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::filename>(e));
        CHECK(*getErrorInfo<Exception_Info::filename>(e) == testfile);
        REQUIRE(getErrorInfo<Exception_Info::filename>(e2));
        CHECK(*getErrorInfo<Exception_Info::filename>(e2) == testfile);
        REQUIRE(getErrorInfo<InfoTestInfo>(e));
        CHECK(getErrorInfo<InfoTestInfo>(e)->i == 23);
        CHECK(getErrorInfo<InfoTestInfo>(e)->s == "fooberella");
        REQUIRE(getErrorInfo<InfoTestInfo>(e2));
        CHECK(getErrorInfo<InfoTestInfo>(e2)->i == 23);
        CHECK(getErrorInfo<InfoTestInfo>(e2)->s == "fooberella");
    }

    SECTION("Copy assignment - built-ins only")
    {
        std::string const testtext("Lorem ipsum");
        std::string const testfile("awesome_source.cpp");
        std::string const testfunc("ultimate_test_function_2k(int, float, long)");
        auto const e = decorate_exception(Exceptions::NotImplemented(),
                                          Exception_Info::description(testtext),
                                          Exception_Info::location(testfile.c_str(), testfunc.c_str(), 42));
        Exceptions::NotImplemented e2;
        CHECK(getErrorInfo<Exception_Info::description>(e2)->empty());
        e2 = e;
        REQUIRE(getErrorInfo<Exception_Info::description>(e));
        CHECK(*getErrorInfo<Exception_Info::description>(e) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::description>(e2));
        CHECK(*getErrorInfo<Exception_Info::description>(e2) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::location>(e));
        CHECK(getErrorInfo<Exception_Info::location>(e)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::location>(e2));
        CHECK(getErrorInfo<Exception_Info::location>(e2)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->line == 42);
    }

    SECTION("Copy assignment - decorated")
    {
        std::string const testtext("Lorem ipsum");
        std::string const testfile("awesome_source.cpp");
        std::string const testfunc("ultimate_test_function_2k(int, float, long)");
        auto const e = decorate_exception(Exceptions::NotImplemented(),
                                          Exception_Info::description(testtext),
                                          Exception_Info::location(testfile.c_str(), testfunc.c_str(), 42),
                                          Exception_Info::filename(testfile),
                                          InfoTestInfo(23, "fooberella"));
        Exceptions::NotImplemented e2;
        CHECK(!getErrorInfo<InfoTestInfo>(e2));
        CHECK(!getErrorInfo<Exception_Info::filename>(e2));
        e2 = e;
        REQUIRE(getErrorInfo<Exception_Info::description>(e));
        CHECK(*getErrorInfo<Exception_Info::description>(e) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::description>(e2));
        CHECK(*getErrorInfo<Exception_Info::description>(e2) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::location>(e));
        CHECK(getErrorInfo<Exception_Info::location>(e)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::location>(e2));
        CHECK(getErrorInfo<Exception_Info::location>(e2)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::filename>(e));
        CHECK(*getErrorInfo<Exception_Info::filename>(e) == testfile);
        REQUIRE(getErrorInfo<Exception_Info::filename>(e2));
        CHECK(*getErrorInfo<Exception_Info::filename>(e2) == testfile);
        REQUIRE(getErrorInfo<InfoTestInfo>(e));
        CHECK(getErrorInfo<InfoTestInfo>(e)->i == 23);
        CHECK(getErrorInfo<InfoTestInfo>(e)->s == "fooberella");
        REQUIRE(getErrorInfo<InfoTestInfo>(e2));
        CHECK(getErrorInfo<InfoTestInfo>(e2)->i == 23);
        CHECK(getErrorInfo<InfoTestInfo>(e2)->s == "fooberella");
    }

    SECTION("Move construction")
    {
        struct MovableException : public GHULBUS_BASE_NAMESPACE::Exceptions::impl::ExceptionImpl {
            MovableException() = default;
            MovableException(MovableException&&) = default;
            MovableException& operator=(MovableException&&) = default;
        };
        std::string const testtext("Lorem ipsum");
        std::string const testfile("awesome_source.cpp");
        std::string const testfunc("ultimate_test_function_2k(int, float, long)");
        auto e = decorate_exception(MovableException(),
                                    Exception_Info::description(testtext),
                                    Exception_Info::location(testfile.c_str(), testfunc.c_str(), 42),
                                    Exception_Info::filename(testfile),
                                    InfoTestInfo(23, "fooberella"));
        MovableException e2(std::move(e));
        REQUIRE(getErrorInfo<Exception_Info::description>(e2));
        CHECK(*getErrorInfo<Exception_Info::description>(e2) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::location>(e2));
        CHECK(getErrorInfo<Exception_Info::location>(e2)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::filename>(e2));
        CHECK(*getErrorInfo<Exception_Info::filename>(e2) == testfile);
        REQUIRE(getErrorInfo<InfoTestInfo>(e2));
        CHECK(getErrorInfo<InfoTestInfo>(e2)->i == 23);
        CHECK(getErrorInfo<InfoTestInfo>(e2)->s == "fooberella");
    }

    SECTION("Move assignment")
    {
        struct MovableException : public GHULBUS_BASE_NAMESPACE::Exceptions::impl::ExceptionImpl {
            MovableException() = default;
            MovableException(MovableException&&) = default;
            MovableException& operator=(MovableException&&) = default;
        };
        std::string const testtext("Lorem ipsum");
        std::string const testfile("awesome_source.cpp");
        std::string const testfunc("ultimate_test_function_2k(int, float, long)");
        auto e = decorate_exception(MovableException(),
                                    Exception_Info::description(testtext),
                                    Exception_Info::location(testfile.c_str(), testfunc.c_str(), 42), 
                                    Exception_Info::filename(testfile),
                                    InfoTestInfo(23, "fooberella"));
        MovableException e2;
        CHECK(!getErrorInfo<InfoTestInfo>(e2));
        CHECK(!getErrorInfo<Exception_Info::filename>(e2));
        e2 = std::move(e);
        REQUIRE(getErrorInfo<Exception_Info::description>(e2));
        CHECK(*getErrorInfo<Exception_Info::description>(e2) == testtext);
        REQUIRE(getErrorInfo<Exception_Info::location>(e2));
        CHECK(getErrorInfo<Exception_Info::location>(e2)->file == testfile);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->function == testfunc);
        CHECK(getErrorInfo<Exception_Info::location>(e2)->line == 42);
        REQUIRE(getErrorInfo<Exception_Info::filename>(e2));
        CHECK(*getErrorInfo<Exception_Info::filename>(e2) == testfile);
        REQUIRE(getErrorInfo<InfoTestInfo>(e2));
        CHECK(getErrorInfo<InfoTestInfo>(e2)->i == 23);
        CHECK(getErrorInfo<InfoTestInfo>(e2)->s == "fooberella");
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
                auto const info = getErrorInfo<Exception_Info::description>(e);
                REQUIRE(info);
                INFO(e.what());
                CHECK(*info == testtext);
                CHECK(std::string(e.what()).find(testtext) != std::string::npos);
            }
            CHECK(was_caught);
        };
        tester(Exceptions::NotImplemented());
        tester(Exceptions::IOError());
        tester(Exceptions::InvalidArgument());
        tester(Exceptions::ProtocolViolation());
    }
}
