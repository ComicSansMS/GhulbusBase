#include <gbBase/LogHandlers.hpp>
#include <gbBase/Assert.hpp>
#include <gbBase/Exception.hpp>

#include <iostream>
#include <iterator>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Log
{
namespace Handlers
{
void logToCout(LogLevel log_level, std::stringstream&& log_stream)
{
    std::ostream& outstr = (log_level >= LogLevel::Error) ? std::cerr : std::cout;
    outstr << log_stream.str() << '\n';
}

LogToFile::LogToFile(char const* filename)
    : m_logFile(filename, std::ios_base::out | std::ios_base::app)
{
    if(!m_logFile) {
        GHULBUS_THROW(Exceptions::IOError() << Exception_Info::filename(filename),
                      "File could not be opened for writing.");
    }
}

LogToFile::operator LogHandler()
{
    return [this](LogLevel, std::stringstream&& os) {
        m_logFile << os.rdbuf() << '\n';
    };
}

LogSynchronizeMutex::LogSynchronizeMutex(LogHandler downstream_handler)
    :m_downstreamHandler(downstream_handler)
{
}

LogSynchronizeMutex::operator LogHandler()
{
    return [this](LogLevel log_level, std::stringstream&& os) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_downstreamHandler(log_level, std::move(os));
    };
}


LogAsync::LogAsync(LogHandler downstream_handler)
    :m_downstreamHandler(downstream_handler), m_stopRequested(false)
{
}

void LogAsync::start()
{
    m_stopRequested = false;
    m_ioThread = std::thread([this]() {
        for(;;) {
            std::stringstream sstr;
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_condvar.wait(lk, [this]() -> bool { return (!m_queue.empty()) || m_stopRequested; });
                if (m_stopRequested) {
                    // termination was requested; flush outstanding messages and get out
                    for(auto& msg : m_queue) {
                        m_downstreamHandler(LogLevel::Error, std::move(msg));
                    }
                    m_queue.clear();
                    break;
                }
                GHULBUS_ASSERT(!m_queue.empty());
                sstr = std::move(m_queue.front());
                m_queue.pop_front();
            }
            m_downstreamHandler(LogLevel::Error, std::move(sstr));
        }
    });
}

void LogAsync::stop()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stopRequested = true;
    }
    m_condvar.notify_all();
    m_ioThread.join();
    GHULBUS_ASSERT(m_queue.empty());
}

LogAsync::operator LogHandler()
{
    return [this](LogLevel log_level, std::stringstream&& os) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.emplace_back(std::move(os));
        m_condvar.notify_one();
    };
}
}
}
}
