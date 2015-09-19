#include <gbBase/Assert.hpp>

#include <gbBase/Exception.hpp>

#include <catch.hpp>

TEST_CASE("Assert")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    SECTION("Default handler is failAbort")
    {
        CHECK(Assert::getAssertionHandler() == &Assert::failAbort);
    }

    SECTION("Getting and setting handler")
    {
        auto handler = [](Assert::HandlerParameters const& param) {};
        Assert::setAssertionHandler(handler);
        REQUIRE(Assert::getAssertionHandler() == handler);
        Assert::setAssertionHandler(&Assert::failAbort);
    }

    SECTION("Default user param is NULL")
    {
        CHECK(Assert::getHandlerParam() == nullptr);
    }

    SECTION("Getting and setting user param")
    {
        int token;
        Assert::setHandlerParam(&token);
        REQUIRE(Assert::getHandlerParam() == &token);
        Assert::setHandlerParam(nullptr);
    }

    SECTION("assertionFailed should invoke currently active handler")
    {
        auto handler = [](Assert::HandlerParameters const& param) {
            CHECK(param.file == std::string("file"));
            CHECK(param.line == 42);
            CHECK(param.function == std::string("func"));
            CHECK(param.condition == std::string("cond"));
            CHECK(param.message == std::string("msg"));
            *static_cast<bool*>(param.user_param) = true;
        };
        Assert::setAssertionHandler(handler);
        bool handlerWasCalled = false;
        Assert::assertionFailed(Assert::HandlerParameters{ "file", 42, "func", "cond", "msg", &handlerWasCalled });
        CHECK(handlerWasCalled);
        Assert::setAssertionHandler(&Assert::failAbort);
    }

    SECTION("Macro invokes assertion handler with user param")
    {
        auto handler = [](Assert::HandlerParameters const& param) {
            CHECK(param.message == std::string("hello from the test!"));
            *static_cast<bool*>(param.user_param) = true;
        };
        Assert::setAssertionHandler(handler);
        bool handlerWasCalled = false;
        Assert::setHandlerParam(&handlerWasCalled);
        GHULBUS_ASSERT_PRD_MESSAGE(false, "hello from the test!");
        CHECK(handlerWasCalled);
        Assert::setAssertionHandler(&Assert::failAbort);
    }

    SECTION("Throw Handler throws AssertFailed exception")
    {
        Assert::setAssertionHandler(&Assert::failThrow);
        CHECK_THROWS_AS(GHULBUS_ASSERT_PRD(false), Exceptions::AssertFailed);
        Assert::setAssertionHandler(&Assert::failAbort);
    }
}
