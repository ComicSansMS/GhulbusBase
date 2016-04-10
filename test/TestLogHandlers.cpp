#include <gbBase/LogHandlers.hpp>

#include <catch.hpp>

#include <atomic>
#include <utility>

namespace {
    struct MockHandler {
    public:
        std::vector<std::pair<GHULBUS_BASE_NAMESPACE::LogLevel, std::string>> received_messages;
    public:
        operator GHULBUS_BASE_NAMESPACE::Log::LogHandler()
        {
            using namespace GHULBUS_BASE_NAMESPACE;
            return [this](LogLevel log_level, std::stringstream&& os) {
                received_messages.push_back(std::make_pair(log_level, os.str()));
            };
        }
    };
}

TEST_CASE("TestLogAsync")
{
    // not the most useful test in the world, but the vast majority of LogAsyncs implementation deals with
    // thread synchronization, which is hard to cover in a unit test.
    using namespace GHULBUS_BASE_NAMESPACE;
    Log::initializeLogging();
    auto const original_log_level = Log::getLogLevel();
    auto const original_log_handler = Log::getLogHandler();

    std::atomic<int> callCount(0);
    std::vector<std::string> messages;
    std::vector<LogLevel> levels;
    auto testHandler = [&callCount, &messages, &levels](LogLevel ll, std::stringstream&& sstr) {
        ++callCount;
        levels.push_back(ll);
        messages.push_back(sstr.str());
    };
    Log::Handlers::LogAsync log_async(testHandler);
    Log::setLogHandler(log_async);
    Log::setLogLevel(LogLevel::Trace);
    GHULBUS_LOG(Trace, "Test1");
    GHULBUS_LOG(Debug, "Test2");
    GHULBUS_LOG(Info, "Test3");
    GHULBUS_LOG(Warning, "Test4");
    GHULBUS_LOG(Error, "Test5");
    GHULBUS_LOG(Critical, "Test42");
    CHECK(callCount == 0);
    log_async.start();
    int localCallCount = callCount;
    CHECK(localCallCount >= 0);
    CHECK(localCallCount <= 6);
    log_async.stop();
    CHECK(callCount == 6);

    SECTION("Check log messages")
    {
        REQUIRE(messages.size() == 6);
        auto const ends_with = [](std::string const& str, std::string const& substr) -> bool
        {
            return str.substr(str.size() - substr.size(), substr.size()) == substr;
        };
        auto const has_level = [](std::string const& str, LogLevel ll) -> bool
        {
            std::stringstream sstr;
            sstr << ll;
            std::string const ll_str = sstr.str();
            return str.substr(0, ll_str.size()) == ll_str;
        };
        CHECK(ends_with(messages[0], "Test1"));
        CHECK(has_level(messages[0], LogLevel::Trace));
        CHECK(ends_with(messages[1], "Test2"));
        CHECK(has_level(messages[1], LogLevel::Debug));
        CHECK(ends_with(messages[2], "Test3"));
        CHECK(has_level(messages[2], LogLevel::Info));
        CHECK(ends_with(messages[3], "Test4"));
        CHECK(has_level(messages[3], LogLevel::Warning));
        CHECK(ends_with(messages[4], "Test5"));
        CHECK(has_level(messages[4], LogLevel::Error));
        CHECK(ends_with(messages[5], "Test42"));
        CHECK(has_level(messages[5], LogLevel::Critical));
    }

    SECTION("Check log levels")
    {
        REQUIRE(levels.size() == 6);
        CHECK(levels[0] == LogLevel::Trace);
        CHECK(levels[1] == LogLevel::Debug);
        CHECK(levels[2] == LogLevel::Info);
        CHECK(levels[3] == LogLevel::Warning);
        CHECK(levels[4] == LogLevel::Error);
        CHECK(levels[5] == LogLevel::Critical);
    }

    Log::setLogHandler(original_log_handler);
    Log::setLogLevel(original_log_level);
    Log::shutdownLogging();
}

TEST_CASE("TestLogMultiSink")
{
    using namespace GHULBUS_BASE_NAMESPACE;
    Log::initializeLogging();
    auto const original_log_level = Log::getLogLevel();
    auto const original_log_handler = Log::getLogHandler();

    MockHandler handler1;
    MockHandler handler2;
    Log::Handlers::LogMultiSink multi_sink_handler(handler1, handler2);
    Log::setLogHandler(multi_sink_handler);
    Log::setLogLevel(LogLevel::Trace);

    CHECK(handler1.received_messages.empty());
    CHECK(handler2.received_messages.empty());

    char const* testtext = "Testtext";
    GHULBUS_LOG(Info, testtext);

    REQUIRE(handler1.received_messages.size() == 1);
    REQUIRE(handler2.received_messages.size() == 1);
    CHECK(handler1.received_messages[0].first == LogLevel::Info);
    CHECK(handler1.received_messages[0].second.find(testtext) != std::string::npos);
    CHECK(handler2.received_messages[0] == handler1.received_messages[0]);

    Log::setLogHandler(original_log_handler);
    Log::setLogLevel(original_log_level);
    Log::shutdownLogging();
}
