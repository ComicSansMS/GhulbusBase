#include <gbBase/Log.hpp>
#include <gbBase/LogHandlers.hpp>

#include <catch.hpp>

#include <chrono>

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

void testHandler(GHULBUS_BASE_NAMESPACE::LogLevel, std::stringstream&&) { /* do nothing */ }
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
        auto handler = [&handlerWasCalled, testmsg](LogLevel log_level, std::stringstream&& os) {
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
        auto handler = [&handlerWasCalled](LogLevel, std::stringstream&&) {
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

    SECTION("Printing different log levels")
    {
        for(auto const& ll : { LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
                               LogLevel::Warning, LogLevel::Error, LogLevel::Critical })
        {
            std::stringstream sstr;
            sstr << ll;
            auto const str = sstr.str();
            REQUIRE(str.length() == 7);
            CHECK(str[0] == '[');
            CHECK(str[6] == ']');
        }
    }

    resetExpectations();
}

#if 0
TEST_CASE("LogPerformance")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    int const N_MESSAGES = 100000;
    char const* message_text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    Log::setLogLevel(LogLevel::Trace);
    Log::Handlers::LogToFile logger("test.log");
    Log::Handlers::LogAsync async(logger);
    async.start();
    Log::setLogHandler(async);

    auto const t_start = std::chrono::steady_clock::now();

    SECTION("LogSingleThreaded")
    {
        for(int i = 0; i < N_MESSAGES; ++i) {
            GHULBUS_LOG(Info, message_text);
        }
    }
    /*
    SECTION("LogContention2Threads")
    {
    }

    SECTION("LogContention10Threads")
    {
    }

    SECTION("LogContention100Threads")
    {
    }
    //*/
    auto const t_end = std::chrono::steady_clock::now();
    async.stop();
    resetExpectations();
    GHULBUS_LOG(Error, "Elapsed time was "
        << std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count() << "msecs.");
}
#endif
