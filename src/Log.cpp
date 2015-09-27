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

namespace GHULBUS_BASE_NAMESPACE
{
namespace
{
std::atomic<LogLevel>                            g_currentLogLevel(LogLevel::Error);
Log::LogHandler                                  g_logHandler(&Log::Handlers::logToCout);

struct current_time_t {} current_time;
inline std::ostream& operator<<(std::ostream& os, current_time_t const&)
{
    std::chrono::system_clock::time_point const now = std::chrono::system_clock::now();
    date::day_point const today = date::floor<date::days>(now);
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
void setLogLevel(LogLevel log_level)
{
    GHULBUS_ASSERT(log_level >= LogLevel::Trace && log_level <= LogLevel::Critical);
    g_currentLogLevel.store(log_level);
}

LogLevel getLogLevel()
{
    return g_currentLogLevel.load();
}

void setLogHandler(LogHandler handler)
{
    g_logHandler = handler;
}

LogHandler getLogHandler()
{
    return g_logHandler;
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
