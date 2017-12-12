#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_DEBUG_POLICY_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_DEBUG_POLICY_HPP

/** @file
*
* @brief Allocator Debug Policies.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>
#include <gbBase/Assert.hpp>
#include <gbBase/UnusedVariable.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <unordered_map>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Allocator
{
namespace DebugPolicy
{
class NoDebug {
public:
    using is_thread_safe = std::true_type;

    void onAllocate(std::size_t, std::size_t, std::byte*) {}
    void onDeallocate(std::byte*, std::size_t) {}
    void onReset() {}
};

class AllocateDeallocateCounter {
public:
    using is_thread_safe = std::false_type;
private:
    std::size_t m_count;
public:
    AllocateDeallocateCounter() : m_count(0)
    {}

    ~AllocateDeallocateCounter()
    {
        GHULBUS_ASSERT_MESSAGE(m_count == 0,
                               "Memory resource was destroyed while there where still allocations active.");
    }

    void onAllocate(std::size_t n, std::size_t alignment, std::byte* allocated_ptr)
    {
        GHULBUS_UNUSED_VARIABLE(n);
        GHULBUS_UNUSED_VARIABLE(alignment);
        GHULBUS_UNUSED_VARIABLE(allocated_ptr);
        ++m_count;
    }

    void onDeallocate(std::byte* p, std::size_t n)
    {
        GHULBUS_UNUSED_VARIABLE(p);
        GHULBUS_UNUSED_VARIABLE(n);
        GHULBUS_PRECONDITION(m_count > 0);
        --m_count;
    }

    void onReset()
    {
        GHULBUS_ASSERT_MESSAGE(m_count == 0,
                               "Memory resource was reset while there where still allocations active.");
    }

    std::size_t getCount() const
    {
        return m_count;
    }
};

class AllocateDeallocateTracking {
public:
    using is_thread_safe = std::true_type;

    struct Record {
        std::byte* pointer;
        std::size_t alignment;
        std::size_t n;
        std::size_t count;
    };
private:
    std::unordered_map<std::byte*, Record> m_records;
    std::size_t m_counter;
public:
    AllocateDeallocateTracking() :m_counter(0)
    {}

    ~AllocateDeallocateTracking()
    {
        GHULBUS_ASSERT_MESSAGE(m_records.empty(),
                               "Memory resource was destroyed while there where still allocations active.");
    }

    void onAllocate(std::size_t n, std::size_t alignment, std::byte* allocated_ptr)
    {
        GHULBUS_ASSERT_MESSAGE(m_records.find(allocated_ptr) == end(m_records),
                               "Same memory block was allocated twice.");
        m_records.insert(std::make_pair(allocated_ptr, Record{ allocated_ptr, alignment, n, m_counter }));
        ++m_counter;
    }

    void onDeallocate(std::byte* p, std::size_t n)
    {
        auto it_rec = m_records.find(p);
        GHULBUS_ASSERT_MESSAGE(it_rec != end(m_records),
                               "Deallocating a block that was not allocated from this recource.");
        GHULBUS_ASSERT_MESSAGE(it_rec->second.n == n, 
                               "Deallocation size does not match allocation size.");
        m_records.erase(it_rec);
    }

    void onReset()
    {
        GHULBUS_ASSERT_MESSAGE(m_records.empty(),
                               "Memory resource was reset while there where still allocations active.");
    }

    std::vector<Record> getRecords() const {
        std::vector<Record> ret;
        ret.reserve(m_records.size());
        std::transform(begin(m_records), end(m_records), std::back_inserter(ret),
                       [](auto const& p) { return p.second; });
        std::sort(begin(ret), end(ret), [](Record const& lhs, Record const& rhs) { return lhs.count < rhs.count; });
        return ret;
    }
};
}
}
}
#endif
