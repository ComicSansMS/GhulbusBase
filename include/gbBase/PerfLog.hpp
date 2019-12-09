#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_PERFLOG_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_PERFLOG_HPP

/** @file
 *
 * @brief Minimalistic Performance Logger.
 * @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
 */

#include <gbBase/config.hpp>
#include <gbBase/Log.hpp>

#include <chrono>
#include <deque>
#include <string>
#include <string_view>

namespace GHULBUS_BASE_NAMESPACE
{
/** A simple performance logger.
 * PerfLog records timestamped events with every tick().
 * These can either be retrieved later or passed on to the logger straight away.
 * Each event consists of a string label and a timestamp.
 */
class PerfLog {
    struct LogEntry {
        std::chrono::steady_clock::time_point t;
        std::string label;

        LogEntry(std::chrono::steady_clock::time_point const& nt, std::string&& nl)
            :t(nt), label(std::move(nl))
        {}
    };
    using Log = std::deque<LogEntry>;
private:
    Log m_log;
public:
    /** Constructor.
     * This will create the initial event labeled "Epoch".
     */
    inline PerfLog()
        :PerfLog("Epoch")
    {}

    /** Constructor.
     * This will create the initial event with the label passed as argument.
     */
    inline explicit PerfLog(std::string_view initial_label)
    {
        tick(initial_label);
    }

    /** Adds a new event to the log.
     * @param[in] label The label associated with the event.
     */
    void tick(std::string_view label)
    {
        m_log.emplace_back(std::chrono::steady_clock::now(), std::string(label));
    }

    /** Adds a new event to the log and log the time passed since the last tick to the logger.
     * @param[in] log_level The log level used for the triggered logging operation.
     * @param[in] label The label associated with the event.
     */
    void tick(LogLevel log_level, std::string_view label)
    {
        auto const t0 = m_log.back().t;
        tick(label);
        GHULBUS_LOG_QUALIFIED(log_level, label << " - took " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(m_log.back().t - t0).count() << " ms.");
    }

    /** Retrieves the event log.
     */
    Log const& getLog() const
    {
        return m_log;
    }
};
}
#endif
