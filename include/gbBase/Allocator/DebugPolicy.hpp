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
/** Empty policy, does nothing.
 */
class NoDebug {
public:
    using is_thread_safe = std::true_type;

    /** Invoked by the AllocationStrategy on every allocation.
     */
    void onAllocate(std::size_t, std::size_t, std::byte*) {}

    /** Invoked by the AllocationStrategy on every deallocation.
    */
    void onDeallocate(std::byte*, std::size_t) {}

    /** Invoked by the AllocationStrategy on explicit resets (not all strategies provide such a reset).
    */
    void onReset() {}
};

/** Counts allocations and deallocations.
 * The internal counter is incremented with each allocate call and decremented with each deallocate call.
 * The policy asserts that the counter is 0 upon resetting and destruction.
 * The policy asserts that the total number of deallocations does not exceed the number of allocations.
 * The policy does not track whether deallocate calls actually match previous allocate calls.
 */
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

    /** @copydoc NoDebug::onAllocate() */
    void onAllocate(std::size_t n, std::size_t alignment, std::byte* allocated_ptr)
    {
        GHULBUS_UNUSED_VARIABLE(n);
        GHULBUS_UNUSED_VARIABLE(alignment);
        GHULBUS_UNUSED_VARIABLE(allocated_ptr);
        ++m_count;
    }

    /** @copydoc NoDebug::onDeallocate() */
    void onDeallocate(std::byte* p, std::size_t n)
    {
        GHULBUS_UNUSED_VARIABLE(p);
        GHULBUS_UNUSED_VARIABLE(n);
        GHULBUS_PRECONDITION(m_count > 0);
        --m_count;
    }

    /** @copydoc NoDebug::onReset() */
    void onReset()
    {
        GHULBUS_ASSERT_MESSAGE(m_count == 0,
                               "Memory resource was reset while there where still allocations active.");
    }

    /** Returns the current allocation count.
     * The counter is increased for each allocation and decreased for each deallocation.
     */
    std::size_t getCount() const
    {
        return m_count;
    }
};

/** Maintains a full track record of all active allocations.
 * Each allocation is saved to an internal list.
 * Deallocations are checked to match entries in the list of active allocations.
 * The policy asserts that no allocations are active upon reset and destruction.
 * A list of all active allocations can be retrieved through getRecords().
 */
class AllocateDeallocateTracking {
public:
    using is_thread_safe = std::false_type;

    /** Allocation record maintained by the AllocateDeallocateTracking debug policy.
     */
    struct Record {
        std::byte* pointer;         ///< Pointer to the memory block returned by the allocation.
        std::size_t alignment;      ///< Requested alignment for the allocation.
        std::size_t size;           ///< Requested size in bytes for the allocation.
        std::size_t id;             ///< Id assigned by the policy. Ids are monotonically increasing and
                                    ///  unique per policy up to overflow.
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

    /** @copydoc NoDebug::onAllocate() */
    void onAllocate(std::size_t n, std::size_t alignment, std::byte* allocated_ptr)
    {
        GHULBUS_ASSERT_MESSAGE(m_records.find(allocated_ptr) == end(m_records),
                               "Same memory block was allocated twice.");
        m_records.insert(std::make_pair(allocated_ptr, Record{ allocated_ptr, alignment, n, m_counter }));
        ++m_counter;
    }

    /** @copydoc NoDebug::onDeallocate() */
    void onDeallocate(std::byte* p, std::size_t n)
    {
        auto it_rec = m_records.find(p);
        GHULBUS_ASSERT_MESSAGE(it_rec != end(m_records),
                               "Deallocating a block that was not allocated from this resource.");
        GHULBUS_ASSERT_MESSAGE(it_rec->second.size == n, 
                               "Deallocation size does not match allocation size.");
        m_records.erase(it_rec);
    }

    /** @copydoc NoDebug::onReset() */
    void onReset()
    {
        GHULBUS_ASSERT_MESSAGE(m_records.empty(),
                               "Memory resource was reset while there where still allocations active.");
    }

    /** Returns a list of all active allocations.
     * An allocation is active if it was allocated but not yet deallocated.
     * The order of entries in the returned list matches the order of the corresponding allocations.
     */
    std::vector<Record> getRecords() const {
        std::vector<Record> ret;
        ret.reserve(m_records.size());
        std::transform(begin(m_records), end(m_records), std::back_inserter(ret),
                       [](auto const& p) { return p.second; });
        std::sort(begin(ret), end(ret), [](Record const& lhs, Record const& rhs) { return lhs.id < rhs.id; });
        return ret;
    }
};
}
}
}
#endif
