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
    inline PerfLog()
        :PerfLog("Epoch")
    {}

    inline explicit PerfLog(std::string_view initial_label)
    {
        tick(initial_label);
    }

    void tick(std::string_view label)
    {
        m_log.emplace_back(std::chrono::steady_clock::now(), std::string(label));
    }

    void tick(LogLevel log_level, std::string_view label)
    {
        auto const t0 = m_log.back().t;
        tick(label);
        GHULBUS_LOG_QUALIFIED(log_level, label << " - took " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(m_log.back().t - t0).count() << " ms.");
    }

    Log const& getLog() const
    {
        return m_log;
    }
};
}
#endif
