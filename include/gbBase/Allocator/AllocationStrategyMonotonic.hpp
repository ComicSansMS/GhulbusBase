#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_MONOTONIC_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_MONOTONIC_HPP

/** @file
*
* @brief Monotonic allocation strategy.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <gbBase/Allocator/DebugPolicy.hpp>
#include <gbBase/Allocator/Storage.hpp>

#include <cstddef>
#include <memory>
#include <new>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Allocator
{
namespace AllocationStrategy
{
/** Monotonic allocation strategy.
 * The monotonic allocator keeps giving out blocks of memory from a region, but never reclaims any memory.
 * If a user has ensured that all previous allocations have been deallocated, they can reclaim the entire
 * memory region by calling reset().
 *
 * The internal state of the monotonic allocator consists of a counter that indicates the offset to
 * the free memory region. Consider the following example showing the state after 3 allocations of regions p1 to p3.
 *  - Unused padding blocks may be inserted to satisfy alignment requirements of an allocation.
 *  - m_offset always indicates the start of the free memory region. Its value keeps growing monotonically unless
 *    explicitly reset to 0 by calling reset().
 *
 * +-------------------------------------------------------------------------------------------------------+
 * | Block    | Block            | Padding | Block          | Free memory                                  |
 * +-------------------------------------------------------------------------------------------------------+
 * ^          ^                            ^                ^
 * p1         p2                           p3               |
 * m_storage.get()                             m_storage.get() + m_offset
 *
 */
template<typename Storage_T, typename Debug_T = Allocator::DebugPolicy::AllocateDeallocateCounter>
class Monotonic : private Debug_T {
private:
    Storage_T* m_storage;
    std::size_t m_offset;
public:
    Monotonic(Storage_T& storage) noexcept
        :m_storage(&storage), m_offset(0)
    {}

    /** Allocate a region of n bytes starting at an address with the specified alignment.
     * It is up to the enclosing allocator to ensure that n and alignment are big enough to hold the requested type.
     * In particular, when allocating memory for an array of elements, n must consider the padding between elements
     * to fulfill the alignment requirements for each individual element.
     */
    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        std::size_t free_space = getFreeMemory();
        void* ptr = reinterpret_cast<void*>(m_storage->get() + m_offset);
        if(!std::align(alignment, n, ptr, free_space)) {
            throw std::bad_alloc();
        }
        std::byte* ret = reinterpret_cast<std::byte*>(ptr);
        m_offset = (ret - m_storage->get()) + n;
        this->onAllocate(n, alignment, ret);
        return ret;
    }

    void deallocate(std::byte* p, std::size_t n)
    {
        GHULBUS_UNUSED_VARIABLE(p);
        GHULBUS_UNUSED_VARIABLE(n);
        this->onDeallocate(p, n);
    }

    /** Returns the size of the free memory region in bytes.
     */
    std::size_t getFreeMemory() const noexcept
    {
        return m_storage->size() - m_offset;
    }

    /** Explicitly resets the allocator, discarding all previously allocated blocks.
     * This function must only be called after all previously allocated blocks have been deallocated.
     */
    void reset()
    {
        this->onReset();
        m_offset = 0;
    }
};
}
}
}
#endif
