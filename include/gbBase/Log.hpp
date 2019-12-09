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

/** Available log levels.
 * Each log message is assigned a log level, indicating its severity.
 * Log levels are sorted from lowest to highest severity.
 */
enum class GHULBUS_BASE_API LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/** Ostream inserter for log levels.
 * Converts the log level to a 7-character string of the form `[LEVEL]`.
 */
GHULBUS_BASE_API std::ostream& operator<<(std::ostream& os, LogLevel log_level);

/** Logging.
 */
namespace Log
{
    /** Signature for log handlers.
     * Functions of this type can be passed to setLogHandler() to customize logging behavior.
     * A collection of predefined handlers can be found in the Ghulbus::Log::Handlers namespace.
     * The first argument to the handler function is the associated log level for the log message.
     * This allows the handler to treat messages of different levels differently.
     * The second argument to the handler is a stringstream containing the message to be logged.
     * Setting an empty function as LogHandler is valid and will cause all log messages to be discarded.
     * @see setLogHandler() getLogHandler()
     */
    typedef std::function<void(LogLevel, std::stringstream&&)> LogHandler;

    /** Initialize the logging subsystem.
     * This function must be called before any other function from the Log namespace.
     * It initializes the static data that is used by the logging subsystem. To free any allocated data,
     * call shutdownLogging(). When calling this function multiple times, each invocation must have
     * a matching call to shutdownLogging() and only the last call will actually perform the shutdown.
     * @attention It is not safe to call this function concurrently with any other function from Log,
     *            including initializeLogging() itself.
     * @see shutdownLogging()
     */
    GHULBUS_BASE_API void initializeLogging();

    /** Shuts down the logging subsystem.
     * Frees any data allocated by initializeLogging(). In case of multiple calls to initializeLogging(),
     * each call must be matched with a call to shutdownLogging() and only the last call will actually
     * perform the shutdown.
     * @attention It is not safe to call this function concurrently with any other function from Log,
     *            including shutdownLogging() itself.
     * @see initializeLogging()
     */
    GHULBUS_BASE_API void shutdownLogging();

    /** A guard object for logging initialization.
     * An object of this class will invoke shutdownLogging() in its destructor.
     */
    struct GHULBUS_BASE_API[[nodiscard]] LoggingInitializeGuard{
    private:
        bool m_doShutdown;
    public:
        LoggingInitializeGuard()
            :m_doShutdown(true)
        {}

        ~LoggingInitializeGuard() {
            if (m_doShutdown) { shutdownLogging(); }
        }

        LoggingInitializeGuard(LoggingInitializeGuard const&) = delete;
        LoggingInitializeGuard& operator=(LoggingInitializeGuard const&) = delete;

        LoggingInitializeGuard(LoggingInitializeGuard&& rhs)
            :m_doShutdown(rhs.m_doShutdown)
        {
            rhs.m_doShutdown = false;
        }

        LoggingInitializeGuard& operator=(LoggingInitializeGuard&& rhs) = delete;
    };

    /** The same as initializeLogging(), but returns a guard object that will automatically shutdown on destruction.
     */
    GHULBUS_BASE_API LoggingInitializeGuard initializeLoggingWithGuard();

    /** Set the system wide log level.
     * If a log message has a lower level than the system log level, it will not be evaluated by the GHULBUS_LOG
     * macro.
     * @note This function is thread safe.
     */
    GHULBUS_BASE_API void setLogLevel(LogLevel log_level);

    /** Get the system wide log level.
     * @copydetails setLogLevel
     */
    GHULBUS_BASE_API LogLevel getLogLevel();

    /** Determine how log messages get processed.
     * @see LogHandler
     * @attention In case of stateful handlers, remember that this function does *not* assume ownership of the
     *            underlying log handler object. Logging to a handler that has been destroyed will result in
     *            undefined behavior.
     */
    GHULBUS_BASE_API void setLogHandler(LogHandler handler);

    /** Retrieve the current log handler function.
     * The default log handler is Log::Handlers::logToCout().
     */
    GHULBUS_BASE_API LogHandler getLogHandler();

    /** Create a stringstream for logging.
     * This function is called by GHULBUS_LOG to obtain a stringstream for logging. The returned stream will contain
     * a textual representation of the passed log level and a timestamp.
     */
    GHULBUS_BASE_API std::stringstream createLogStream(LogLevel level);

    /** Invoke the current log handler.
     * Invoke the function returned by getLogHandler() with the given arguments.
     * If the current log handler is the empty function, this function does nothing.
     * @note This function does *not* filter messages based on the current log level.
     *       Use the GHULBUS_LOG macro instead if message filtering is desired.
     */
    GHULBUS_BASE_API void log(LogLevel log_level, std::stringstream&& log_stream);
}

}

#ifndef GHULBUS_CONFIG_DISABLE_LOGGING
/** Log a message.
 * Use this macro for logging a message. All log messages will be prepended by a textual representation of the given
 * log level and the current timestamp. %Log messages will be forwarded to the log handler function returned by
 * Ghulbus::Log::getLogHandler().
 * @param[in] log_level One of the log level identifiers from Ghulbus::LogLevel *without* any additional qualifiers.
 *                      If this log level is lower than the one returned by Ghulbus::Log::getLogLevel(), no code
 *                      will be executed. In particular, the \em expr argument will not be evaluated.
 * @param[in] expr The log message. This can be any expression that can be inserted into an ostream using `operator<<`.
 *                 The expression should be free of visible side-effects to avoid accidental changes in program
 *                 behavior when the system log level changes.
 *
 * @b Example
   @code
   GHULBUS_LOG(Info, "The magic number is " << 42 << ".");
   @endcode
 */
#define GHULBUS_LOG(log_level, expr) GHULBUS_LOG_QUALIFIED(::GHULBUS_BASE_NAMESPACE::LogLevel::log_level, expr)

/** Same as \ref GHULBUS_LOG, except that the log_level parameter has to be fully qualified.
 * Use this for example if the log level is to be determined from a variable at runtime.
 * @b Example
   @code
   Log::LogLevel loglvl = Log::LogLevel::Info;
   GHULBUS_LOG_QUALIFIED(loglvl, "The magic number is " << 42 << ".");
   @endcode
 */
#define GHULBUS_LOG_QUALIFIED(log_level_qualified, expr) do {                                                        \
        if(::GHULBUS_BASE_NAMESPACE::Log::getLogLevel() <= log_level_qualified) {                                    \
            ::GHULBUS_BASE_NAMESPACE::Log::log(log_level_qualified,                                                  \
                static_cast<std::stringstream&&>(                                                                    \
                    ::GHULBUS_BASE_NAMESPACE::Log::createLogStream(log_level_qualified)                              \
                        << expr)                                                                                     \
            );                                                                                                       \
        }                                                                                                            \
    } while(false)
#endif

#endif
