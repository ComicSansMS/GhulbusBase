#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_LOG_HANDLERS_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_LOG_HANDLERS_HPP

/** @file
*
* @brief Log Handlers.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>
#include <gbBase/Log.hpp>

#include <fstream>
#include <mutex>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Log
{
namespace Handlers
{

GHULBUS_BASE_API void logToCout(LogLevel log_level, std::ostringstream&& log_stream);

class LogToFile {
private:
    std::ofstream m_logFile;
public:
    LogToFile(char const* filename);

    operator LogHandler();
};

class LogSynchronizeMutex {
private:
    std::mutex m_mutex;
    LogHandler m_downstreamHandler;
public:
    LogSynchronizeMutex(LogHandler downstream_handler);

    operator LogHandler();
};

class LogAsync {
public:
    LogAsync();
    void start();
    void stop();
    void setLogHandler(LogHandler handler);
    void setLogHandler(LogHandler handler, void* user_param);
    LogHandler* getLogHandler() const;
    void* getUserParam() const;
};
}
}
}
#endif
