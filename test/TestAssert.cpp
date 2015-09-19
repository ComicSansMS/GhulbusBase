#include <gbBase/Assert.hpp>

#include <catch.hpp>

TEST_CASE("Assert")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    SECTION("Default handler is failAbort")
    {
        CHECK(Assert::getAssertionHandler() == &Assert::failAbort);
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
        REQUIRE(Assert::getAssertionHandler() == handler);
        bool handlerWasCalled = false;
        Assert::assertionFailed(Assert::HandlerParameters{ "file", 42, "func", "cond", "msg", &handlerWasCalled });
        CHECK(handlerWasCalled);
        Assert::setAssertionHandler(&Assert::failAbort);
    }

#if defined GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG || !defined GHULBUS_CONFIG_ASSERT_LEVEL_PRODUCTION
    SECTION("Macro invokes assertion handler with user param")
    {
        auto handler = [](Assert::HandlerParameters const& param) {
            CHECK(param.message == std::string("hello from the test!"));
            *static_cast<bool*>(param.user_param) = true;
        };
        Assert::setAssertionHandler(handler);
        REQUIRE(Assert::getAssertionHandler() == handler);
        bool handlerWasCalled = false;
        Assert::setHandlerParam(&handlerWasCalled);
        REQUIRE(Assert::getHandlerParam() == &handlerWasCalled);
        GHULBUS_ASSERT_MESSAGE(false, "hello from the test!");
        CHECK(handlerWasCalled);
        Assert::setAssertionHandler(&Assert::failAbort);
    }
#endif
}
