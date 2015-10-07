#include <gbBase/Assert.hpp>

#ifndef GHULBUS_CONFIG_BASE_BARE_BUILD
#include <gbBase/Exception.hpp>
#endif

#include <catch.hpp>

namespace
{
// catch's exception checking does not seem to work
// with abstract exception types; let's roll our own!
template<typename T>
void checkExceptionType(void (*fun)())
{
    bool was_caught = false;
    try {
        fun();
    } catch(T&) {
        was_caught = true;
    } catch(...) {
        CHECK_FALSE("Incorrect exception type");
    }
    CHECK(was_caught);
}

template<typename T>
void checkHandlerEquals(T const& t)
{
    using namespace GHULBUS_BASE_NAMESPACE;
    auto const handler_func = Assert::getAssertionHandler().target<T>();
    REQUIRE(handler_func);
    CHECK(*handler_func == t);
}
}

TEST_CASE("Assert")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    // Default handler is failAbort
    checkHandlerEquals(&Assert::failAbort);

    SECTION("Getting and setting handler")
    {
        auto handler = [](Assert::HandlerParameters const& param) {};
        Assert::setAssertionHandler(&Assert::failHalt);
        checkHandlerEquals(&Assert::failHalt);
    }

    SECTION("assertionFailed should invoke currently active handler")
    {
        bool handlerWasCalled = false;
        auto handler = [&handlerWasCalled](Assert::HandlerParameters const& param) {
            CHECK(param.file == std::string("file"));
            CHECK(param.line == 42);
            CHECK(param.function == std::string("func"));
            CHECK(param.condition == std::string("cond"));
            CHECK(param.message == std::string("msg"));
            handlerWasCalled = true;
        };
        Assert::setAssertionHandler(handler);
        Assert::assertionFailed(Assert::HandlerParameters{ "file", 42, "func", "cond", "msg" });
        CHECK(handlerWasCalled);
    }

#ifndef GHULBUS_CONFIG_BASE_BARE_BUILD
    SECTION("Throw Handler throws AssertFailed exception")
    {
        // the big gotcha of this test is that we throw across dll boundaries here.
        // on some platforms, this can mess up the rtti if the exception was not declared correctly.
        // technically, this is probably more a test of Exceptions::AssertFailed than GHULBUS_ASSERT...
        Assert::setAssertionHandler(&Assert::failThrow);
        auto doAssert = []() { GHULBUS_ASSERT_PRD(false); };
        checkExceptionType<Exceptions::AssertFailed>(doAssert);
        checkExceptionType<Exception>(doAssert);
        checkExceptionType<std::exception>(doAssert);
    }

    SECTION("Assert with message should forward message to exception")
    {
        Assert::setAssertionHandler(&Assert::failThrow);
        bool was_caught = false;
        char const* test_msg = "Just an example error message for testing purposes";
        try {
            GHULBUS_ASSERT_PRD_MESSAGE(false, test_msg);
        } catch(Exceptions::AssertFailed& e) {
            auto const err_msg = boost::get_error_info<Exception_Info::description>(e);
            REQUIRE(err_msg);
            CHECK(*err_msg == std::string("false - ") + test_msg);
            was_caught = true;
        }
        CHECK(was_caught);
    }
#endif

    SECTION("Precondition macro should behave the same as assert")
    {
        bool handlerWasCalled = false;
        auto handler = [&handlerWasCalled](Assert::HandlerParameters const&) {
            handlerWasCalled = true;
        };
        Assert::setAssertionHandler(handler);
        GHULBUS_PRECONDITION_PRD(true);
        CHECK(!handlerWasCalled);
        GHULBUS_PRECONDITION_PRD(false);
        CHECK(handlerWasCalled);
    }

    SECTION("Unreachable macro should trigger failing assertion")
    {
        bool handlerWasCalled = false;
        auto handler = [&handlerWasCalled](Assert::HandlerParameters const&) {
            handlerWasCalled = true;
        };
        Assert::setAssertionHandler(handler);
        GHULBUS_UNREACHABLE();
        CHECK(handlerWasCalled);
    }

    Assert::setAssertionHandler(&Assert::failAbort);
}
