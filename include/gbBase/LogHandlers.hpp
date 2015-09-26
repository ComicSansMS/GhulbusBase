#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_LOG_HANDLERS_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_LOG_HANDLERS_HPP

/** @file
*
* @brief Log Handlers.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>
#include <gbBase/Log.hpp>

#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Log
{
namespace Handlers
{

GHULBUS_BASE_API void logToCout(LogLevel log_level, std::stringstream&& log_stream);

class LogToFile {
private:
    std::ofstream m_logFile;
public:
    GHULBUS_BASE_API LogToFile(char const* filename);

    GHULBUS_BASE_API operator LogHandler();
};

class LogSynchronizeMutex {
private:
    std::mutex m_mutex;
    LogHandler m_downstreamHandler;
public:
    GHULBUS_BASE_API LogSynchronizeMutex(LogHandler downstream_handler);

    GHULBUS_BASE_API operator LogHandler();
};

class LogAsync {
private:
    std::mutex m_mutex;
    std::deque<std::stringstream> m_queue;
    bool m_stopRequested;
    std::condition_variable m_condvar;
    LogHandler m_downstreamHandler;
    std::thread m_ioThread;
public:
    GHULBUS_BASE_API LogAsync(LogHandler downstream_handler);

    GHULBUS_BASE_API void start();
    GHULBUS_BASE_API void stop();

    GHULBUS_BASE_API operator LogHandler();
};
}
}
}
#endif
