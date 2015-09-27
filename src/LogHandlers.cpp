#include <gbBase/LogHandlers.hpp>
#include <gbBase/Assert.hpp>
#include <gbBase/Exception.hpp>

#include <iostream>
#include <iterator>

/* Initial performance tests for log handlers:
 * Logging 100k messages à 480 byte from single thread
 * MSVC 2015, Release:
 * Empty log handler -  460ms
 * std::fstream      - 1050ms
 * FILE* cstdio      -  600ms
 * Async handler     -  540ms
 *
 * Fstream is much slower than FILE*, but since async outperforms them both (regardless of whether using fstream
 *  or FILE* as downstream) we stick with fstream for now.
 */

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
    GHULBUS_PRECONDITION(downstream_handler);
}

LogSynchronizeMutex::operator LogHandler()
{
    return [this](LogLevel log_level, std::stringstream&& os) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_downstreamHandler(log_level, std::move(os));
    };
}


LogAsync::LogAsync(LogHandler downstream_handler)
    :m_stopRequested(false), m_downstreamHandler(downstream_handler)
{
    GHULBUS_PRECONDITION(downstream_handler);
}

void LogAsync::start()
{
    GHULBUS_PRECONDITION_PRD(!m_ioThread.joinable());
    m_stopRequested = false;
    m_ioThread = std::thread([this]() {
        for(;;) {
            std::stringstream sstr;
            LogLevel ll;
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_condvar.wait(lk, [this]() -> bool { return (!m_queue.empty()) || m_stopRequested; });
                if (m_stopRequested) {
                    // termination was requested; flush outstanding messages and get out
                    for(auto& qe : m_queue) {
                        m_downstreamHandler(std::get<0>(qe), std::move(std::get<1>(qe)));
                    }
                    m_queue.clear();
                    break;
                }
                GHULBUS_ASSERT(!m_queue.empty());
                ll = std::get<0>(m_queue.front());
                sstr = std::move(std::get<1>(m_queue.front()));
                m_queue.pop_front();
            }
            // invoke the downstream handler outside the lock
            m_downstreamHandler(ll, std::move(sstr));
        }
    });
}

void LogAsync::stop()
{
    GHULBUS_PRECONDITION(m_ioThread.joinable());
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
        m_queue.emplace_back(log_level, std::move(os));
        m_condvar.notify_one();
    };
}
}
}
}
