#include <gbBase/LogHandlers.hpp>

#include <iostream>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Log
{
namespace Handlers
{
void logToCout(LogLevel log_level, std::ostringstream&& log_stream)
{
    std::ostream& outstr = (log_level >= LogLevel::Error) ? std::cerr : std::cout;
    outstr << log_stream.str() << '\n';
}

LogToFile::LogToFile(char const* filename)
    :m_logFile(filename)
{
}

LogToFile::operator LogHandler()
{
    return [this](LogLevel, std::ostringstream&& os) {
        m_logFile << os.rdbuf() << '\n';
    };
}

LogSynchronizeMutex::LogSynchronizeMutex(LogHandler downstream_handler)
    :m_downstreamHandler(downstream_handler)
{
}

LogSynchronizeMutex::operator LogHandler()
{
    return [this](LogLevel log_level, std::ostringstream&& os) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_downstreamHandler(log_level, std::move(os));
    };
}
}
}
}
