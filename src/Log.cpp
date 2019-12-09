#include <gbBase/Log.hpp>
#include <gbBase/Assert.hpp>
#include <gbBase/LogHandlers.hpp>

#include <boost/predef.h>

#if BOOST_COMP_MSVC
#pragma warning(push)
#pragma warning(disable: 4146 4244)
#endif
#include <hinnant_date/date.h>
#if BOOST_COMP_MSVC
#pragma warning(pop)
#endif

#include <atomic>
#include <chrono>
#include <cstring>
#include <type_traits>

namespace GHULBUS_BASE_NAMESPACE
{
namespace
{
/** Non-trivial static state that requires explicit initialization via initializeLogging().
 */
struct StaticState {
    std::atomic<LogLevel> currentLogLevel;
    Log::LogHandler       logHandler;

    StaticState()
        :currentLogLevel(LogLevel::Error), logHandler(&Log::Handlers::logToCout)
    {}
};

/** Trivial static state that is initialized at compile time.
 * This is mainly used for doing the bookkeeping for initializeLogging() and shutdownLogging().
 */
struct StaticData {
    int staticStorageRefcount;
    std::aligned_storage<sizeof(StaticState), alignof(StaticState)>::type staticStateStorage;
    StaticState* logState;
} g_staticData;

static_assert(std::is_trivial<StaticData>::value,
              "Non-trivial types not allowed in StaticData to avoid static initialization order headaches.");

struct current_time_t {} current_time;
inline std::ostream& operator<<(std::ostream& os, current_time_t const&)
{
#if _MSC_FULL_VER < 190023918
    using date::floor;
#endif
    std::chrono::system_clock::time_point const now = std::chrono::system_clock::now();
    date::sys_days const today = floor<date::days>(now);
    // the duration cast here determines the precision of the resulting time_of_day in the output
    auto const time_since_midnight = std::chrono::duration_cast<std::chrono::milliseconds>(now - today);
    return os << date::year_month_day(today) << ' ' << date::make_time(time_since_midnight);
}
}

std::ostream& operator<<(std::ostream& os, LogLevel log_level)
{
    switch(log_level) {
    default: GHULBUS_UNREACHABLE();
    case LogLevel::Trace:    os << "[TRACE]"; break;
    case LogLevel::Debug:    os << "[DEBUG]"; break;
    case LogLevel::Info:     os << "[INFO ]"; break;
    case LogLevel::Warning:  os << "[WARN ]"; break;
    case LogLevel::Error:    os << "[ERROR]"; break;
    case LogLevel::Critical: os << "[CRIT ]"; break;
    }
    return os;
}

namespace Log
{
void initializeLogging()
{
    auto& staticData = g_staticData;
    GHULBUS_ASSERT(staticData.staticStorageRefcount >= 0);
    if(staticData.staticStorageRefcount == 0)
    {
        staticData.logState = new(&staticData.staticStateStorage) StaticState();
    }
    ++staticData.staticStorageRefcount;
}

void shutdownLogging()
{
    auto& staticData = g_staticData;
    GHULBUS_ASSERT(g_staticData.staticStorageRefcount > 0);
    if(staticData.staticStorageRefcount == 1)
    {
         staticData.logState->~StaticState();
         std::memset(&staticData.staticStateStorage, 0, sizeof(staticData.staticStateStorage));
         staticData.logState = nullptr;
    }
    --staticData.staticStorageRefcount;
}

LoggingInitializeGuard initializeLoggingWithGuard()
{
    initializeLogging();
    return LoggingInitializeGuard{};
}

void setLogLevel(LogLevel log_level)
{
    GHULBUS_ASSERT(log_level >= LogLevel::Trace && log_level <= LogLevel::Critical);
    auto& staticData = g_staticData;
    staticData.logState->currentLogLevel.store(log_level);
}

LogLevel getLogLevel()
{
    auto const& staticData = g_staticData;
    return staticData.logState->currentLogLevel.load();
}

void setLogHandler(LogHandler handler)
{
    auto& staticData = g_staticData;
    staticData.logState->logHandler = handler;
}

LogHandler getLogHandler()
{
    auto const& staticData = g_staticData;
    return staticData.logState->logHandler;
}

std::stringstream createLogStream(LogLevel level)
{
    std::stringstream log_stream;
    log_stream << level << ' ' << current_time << " - ";
    return log_stream;
}

void log(LogLevel log_level, std::stringstream&& log_stream)
{
    auto const handler = getLogHandler();
    if (handler)
    {
        handler(log_level, std::move(log_stream));
    }
}
}
}
