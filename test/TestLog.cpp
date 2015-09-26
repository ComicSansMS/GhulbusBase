#include <gbBase/Log.hpp>
#include <gbBase/LogHandlers.hpp>

#include <catch.hpp>

namespace {
void checkExpectations()
{
    using namespace GHULBUS_BASE_NAMESPACE;
    // default log handler is logToCout
    auto handler = Log::getLogHandler().target<decltype(&Log::Handlers::logToCout)>();
    REQUIRE(handler);
    CHECK(*handler == &Log::Handlers::logToCout);

    // default log level is error
    CHECK(Log::getLogLevel() == LogLevel::Error);
}

void resetExpectations()
{
    using namespace GHULBUS_BASE_NAMESPACE;
    Log::setLogHandler(Log::Handlers::logToCout);
    Log::setLogLevel(LogLevel::Error);
}

void testHandler(GHULBUS_BASE_NAMESPACE::LogLevel, std::ostringstream&&) { /* do nothing */ }
}

TEST_CASE("TestLog")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    checkExpectations();

    SECTION("Setting log handler")
    {
        Log::setLogHandler(testHandler);
        auto handler = Log::getLogHandler().target<decltype(&testHandler)>();
        REQUIRE(handler);
        CHECK(*handler == &testHandler);
        Log::setLogHandler(Log::LogHandler());
        CHECK(!Log::getLogHandler());
    }

    SECTION("Setting log level")
    {
        Log::setLogLevel(LogLevel::Info);
        CHECK(Log::getLogLevel() == LogLevel::Info);
        Log::setLogLevel(LogLevel::Error);
        CHECK(Log::getLogLevel() == LogLevel::Error);
    }

    SECTION("Log invokes log handler")
    {
        bool handlerWasCalled = false;
        std::string testmsg = "foo";
        auto handler = [&handlerWasCalled, testmsg](LogLevel log_level, std::ostringstream&& os) {
            CHECK(log_level == GHULBUS_BASE_NAMESPACE::LogLevel::Error);
            CHECK(os.str().find(testmsg) != std::string::npos);
            handlerWasCalled = true;
        };
        Log::setLogHandler(handler);
        CHECK(!handlerWasCalled);
        GHULBUS_LOG(Error, testmsg);
        CHECK(handlerWasCalled);
    }

    SECTION("Log does nothing if log handler is empty")
    {
        Log::setLogHandler(Log::LogHandler());
        GHULBUS_LOG(Error, "foo");
    }

    SECTION("Log does nothing if set log level is higher than log level of the log call")
    {
        bool handlerWasCalled = false;
        auto handler = [&handlerWasCalled](LogLevel, std::ostringstream&&) {
            handlerWasCalled = true;
        };
        Log::setLogHandler(handler);
        Log::setLogLevel(LogLevel::Warning);
        REQUIRE(LogLevel::Warning > LogLevel::Info);
        GHULBUS_LOG(Info, "");
        CHECK(!handlerWasCalled);
        Log::setLogLevel(LogLevel::Info);
        GHULBUS_LOG(Info, "");
        CHECK(handlerWasCalled);
        handlerWasCalled = false;
        Log::setLogLevel(LogLevel::Trace);
        GHULBUS_LOG(Info, "");
        CHECK(handlerWasCalled);
    }

    resetExpectations();
}

TEST_CASE("LogPerformance")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    // total: 100,000 messages

    SECTION("LogSingleThreaded")
    {
        Log::setLogLevel(LogLevel::Trace);
        GHULBUS_LOG(Info, "Bla " << 42);
    }

    SECTION("LogContention2Threads")
    {
    }

    SECTION("LogContention10Threads")
    {
    }

    SECTION("LogContention100Threads")
    {
    }
}
