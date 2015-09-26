#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_LOG_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_LOG_HPP

/** @file
*
* @brief Logging.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <functional>
#include <sstream>

namespace GHULBUS_BASE_NAMESPACE
{

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

GHULBUS_BASE_API std::ostream& operator<<(std::ostream& os, LogLevel log_level);

namespace Log
{
    typedef std::function<void(LogLevel, std::stringstream&&)> LogHandler;
    GHULBUS_BASE_API void setLogLevel(LogLevel log_level);
    GHULBUS_BASE_API LogLevel getLogLevel();
    GHULBUS_BASE_API void setLogHandler(LogHandler handler);
    GHULBUS_BASE_API LogHandler getLogHandler();
    GHULBUS_BASE_API std::stringstream createLogStream(LogLevel level);
    GHULBUS_BASE_API void log(LogLevel log_level, std::stringstream&& log_stream);
}

// requirements:
// * at most one memalloc on caller side (std::string&& ?)
// * for a set log level, all log calls with a lower level must only result in code for a single if()-check
// * configurable multi threading behavior: unsynced, locking, lockfree
// * configurable output: default handlers - file, console; dispatch per log-level
// * explicit initialization/shutdown

}

#ifndef GHULBUS_CONFIG_DISABLE_LOGGING
#define GHULBUS_LOG(log_level, expr) do {                                                                            \
        if(::GHULBUS_BASE_NAMESPACE::Log::getLogLevel() <= ::GHULBUS_BASE_NAMESPACE::LogLevel::log_level) { \
            ::GHULBUS_BASE_NAMESPACE::Log::log(::GHULBUS_BASE_NAMESPACE::LogLevel::log_level, \
                std::move(static_cast<std::stringstream&>( \
                    ::GHULBUS_BASE_NAMESPACE::Log::createLogStream(::GHULBUS_BASE_NAMESPACE::LogLevel::log_level) \
                        << expr) ) \
            ); \
        } \
    } while(false)
#endif

#endif
