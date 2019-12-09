#include <gbBase/PerfLog.hpp>

#include <gbBase/Finally.hpp>
#include <gbBase/LogHandlers.hpp>

#include <catch.hpp>

TEST_CASE("PerfLog")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    SECTION("Default Construction")
    {
        PerfLog l;
        REQUIRE(l.getLog().size() == 1);
        CHECK(l.getLog().back().label == "Epoch");
    }

    SECTION("Labelled Construction")
    {
        PerfLog l("My Label");
        REQUIRE(l.getLog().size() == 1);
        CHECK(l.getLog().back().label == "My Label");
    }

    SECTION("Tick")
    {
        PerfLog l;
        REQUIRE(l.getLog().size() == 1);
        l.tick("abc");
        REQUIRE(l.getLog().size() == 2);
        CHECK(l.getLog().back().label == "abc");
        CHECK(l.getLog().back().t >= l.getLog().front().t);
    }

    SECTION("Tick with Logging")
    {
        Log::initializeLogging();
        auto guard_loggig = finally([]() { Log::shutdownLogging(); });
        auto guard_log_handler = finally([]() { Log::setLogHandler(Log::Handlers::logToCout); });
        bool was_called = false;
        Log::setLogHandler([&was_called](LogLevel log_level, std::stringstream&& log_stream) {
                CHECK(log_level == LogLevel::Error);
                CHECK(!log_stream.str().empty());
                CHECK(log_stream.str().find("abc") != std::string::npos);
                was_called = true;
            });
        PerfLog l;
        CHECK(l.getLog().size() == 1);
        CHECK(!was_called);
        l.tick(LogLevel::Error, "abc");
        CHECK(was_called);
        REQUIRE(l.getLog().size() == 2);
        CHECK(l.getLog().back().label == "abc");
        CHECK(l.getLog().back().t >= l.getLog().front().t);
    }
}
