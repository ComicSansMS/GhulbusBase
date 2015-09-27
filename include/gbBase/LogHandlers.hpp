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
#include <tuple>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Log
{
/** Handlers for use with Ghulbus::Log::setLogHandler().
 */
namespace Handlers
{

/** Simple, unsynchronized logging to `std::cout` and `std::cerr`.
 * Log messages of Ghulbus::LogLevel::Error or higher go to `std::cerr`, all others to `std::cout`.
 * Logging is not synchronized, so logging concurrently from multiple threads is not safe with this handler.
 * Use one of the \ref log_handler_adapters "thread-safe adapters" LogSynchronizeMutex or LogAsync if you need to
 * log from more than one thread.
 * @note Logging to the console is usually quite slow and can slow down an application significantly if many log
 *       messages are produces. Use the LogAsync adapter if performance is an issue.
 */
GHULBUS_BASE_API void logToCout(LogLevel log_level, std::stringstream&& log_stream);

/** Unsynchronized file logging.
 * All log messages will be appended to the given log file. %Log messages may be buffered in memory but will be
 * flushed upon destruction of the handler object.
 * Logging is not synchronized, so logging concurrently from multiple threads is not safe with this handler.
 * Use one of the \ref log_handler_adapters "thread-safe adapters" LogSynchronizeMutex or LogAsync if you need to
 * log from more than one thread.
 * @note This handler uses `std::fstream` for file I/O which might perform less than ideal on certain
 *       (*cough* Windows) implementations.
 *       Use the LogAsync adapter if performance is an issue.
 */
class LogToFile {
private:
    std::ofstream m_logFile;
public:
    /** Construct a logger for logging to a file.
     * @param[in] filename Path to the log file. This file will be opened in append mode.
     * @throw Exceptions::IOError If file could not be opened for writing.
     */
    GHULBUS_BASE_API LogToFile(char const* filename);

    /** Convert to a LogHandler function to pass to Ghulbus::Log::setLogHandler().
     * @attention Note that an object must not be destroyed while it is set as log handler.
     */
    GHULBUS_BASE_API operator LogHandler();
};

/** @defgroup log_handler_adapters Handler adapters.
 * A handler adapter is not a full handler by itself, but rather wraps around a downstream handler to add
 * additional functionality (like thread safety).
 * @{
 */
/** Simple synchronization.
 * The mutex adapter synchronizes concurrent access to the downstream handler via a mutex.
 * The resulting handler is thread safe.
 */
class LogSynchronizeMutex {
private:
    std::mutex m_mutex;
    LogHandler m_downstreamHandler;
public:
    /** Adapting Constructor.
     * @param[in] downstream_handler The log handler that is to be wrapped.
     *                               The downstream handler need not be thread safe.
     *                               The downstream handler must not be empty. If the downstream handler dies before
     *                               this adapter, the behavior is undefined.
     */
    GHULBUS_BASE_API LogSynchronizeMutex(LogHandler downstream_handler);

    /** Convert to a LogHandler function to pass to Ghulbus::Log::setLogHandler().
     * @attention Note that an object must not be destroyed while it is set as log handler.
     */
    GHULBUS_BASE_API operator LogHandler();
};

/** Asynchronous logging.
 * This adapter defers execution of the downstream handler in a separate thread. This allows the logging thread
 * to continue immediately after assembling the log message without having to wait for the log I/O to complete.
 * The impact on the runtime of the logging thread is far lower than with the other handlers. The downside is
 * that async logging requires spawning an additional I/O thread to execute the downstream handler.
 * Note that all access to the downstream handler is serialized through this I/O thread, so you can use a thread
 * unsafe handler as downstream handler.
 *
 * @note Messages will be queued in memory for processing. If an application produces log messages faster than the
 *       adapted handler can process them, this can in theory consume all available memory on the machine. Be careful
 *       not to overload the logging thread in this way.
 *
 * @note While the async handler works very well with small numbers of threads, it still requires concurrent access
 *       to the single queue of log messages. If you need highly concurrent logging, consider using an adapter that
 *       uses several thread-local queues instead.
 */
class LogAsync {
private:
    typedef std::tuple<LogLevel, std::stringstream> QueueElement;
private:
    std::mutex m_mutex;                     ///< mutex protexting access to the m_queue
    std::deque<QueueElement> m_queue;       ///< queue of log messages
    bool m_stopRequested;                   ///< flag indicating that the user called stop() to stop the I/O thread
    std::condition_variable m_condvar;      ///< signal that a message was pushed to the queue or stop was requested
    LogHandler m_downstreamHandler;
    std::thread m_ioThread;
public:
    /** @copydoc LogSynchronizeMutex::LogSynchronizeMutex(LogHandler)
     */
    GHULBUS_BASE_API LogAsync(LogHandler downstream_handler);

    /** Start the I/O thread.
     * Invoking the log handler obtained from this class will put the respective log message to an in-memory queue.
     * This function will spawn a thread that waits for messages to be put into that queue and forwards them to the
     * downstream handler. Note that while that thread is not running, messages will just keep piling up in memory,
     * so it is best to start the adapter *before* setting it as the active log handler.
     * Note that the object must not be destroyed while the I/O thread is running.
     * @see stop()
     * @pre The I/O thread is not already running.
     * @note This function is thread-safe.
     */
    GHULBUS_BASE_API void start();

    /** Stop the I/O thread.
     * Processes all outstanding log messages still in the queue and then joins the I/O thread.
     * %Log messages arriving after stop() has started executing will remain unprocessed. So you will probably want
     * to remove the adapter from active log handler before calling stop().
     * @see start()
     * @pre The I/O thread is currently running.
     * @note This function is thread-safe.
     */
    GHULBUS_BASE_API void stop();

    /** Convert to a LogHandler function to pass to Ghulbus::Log::setLogHandler().
     * @note Note that the handler needs to be \ref start() "started" for any log messages to be processed.
     * @attention Note that an object must not be destroyed while it is set as log handler.
     */
    GHULBUS_BASE_API operator LogHandler();
};
/** @} */
}
}
}
#endif
