#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_STACK_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_STACK_HPP

/** @file
*
* @brief Stack allocation strategy.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <gbBase/Allocator/DebugPolicy.hpp>
#include <gbBase/Allocator/Storage.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <new>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Allocator
{
namespace AllocationStrategy
{

/** The stack allocation strategy.
 * Memory is always allocated from the end, similar to the Monotonic allocator.
 * Stack allocation also allows deallocation in a LIFO fashion. That is, memory can only be reclaimed when the
 * most recent allocation gets deallocated. When deallocation does not happen in strict LIFO-order, deallocating
 * the most recent allocation will reclaim all intermediate blocks until the next-most recent allocation that
 * has not been deallocated yet.
 *
 * The following picture shows the internal state after 4 successful allocations of regions p1 to p4.
 *  - Each allocated block is preceded by a Header and optionally by a padding region
 *    to satisfy alignment requirements.
 *  - Padding is performed such that each pN meets the requested alignment requirement *and*
 *    the preceding header meets the natural alignment requirement for Header.
 *  - Each header contains a pointer to the start of the previous header and the size of
 *    the following block in bytes.
 *  - m_topHeader points to the top-most header that has not been deallocated.
 *  - The end of the top-most block marks the start of the free memory.
 *
 * <pre>
 *                              +---prev_header-------+
 *   +------prev_header---------|----+           +----|-----prev_header----------+
 *   v        <-size-->         v    |   <-size--v    |   <--size->              |   <--size->
 *   +--------+-------+---------+--------+-------+--------+-------+---------+--------+-------+-------------+
 *   | Header | Block | Padding | Header | Block | Header | Block | Padding | Header | Block | Free Memory |
 *   +--------+-------+---------+--------+-------+--------+-------+---------+--------+-------+-------------+
 *   ^        ^                          ^                ^                 ^        ^
 *   |        p1                         p2               p3                |        p4
 * m_storage.get()                                                     m_topHeader
  </pre>
 *
 * Upon deallocation:
 *  - The header for the corresponding allocation has its size set to 0xffffffffffffffff.
 *  - The m_topHeader will be moved to the left along the list of previous headers until
 *    it no longer points to a header with a size of 0xffffffffffffffff.
 *
 */
template<typename Storage_T, typename Debug_T = Allocator::DebugPolicy::AllocateDeallocateCounter>
class Stack : private Debug_T {
public:
    /** Header used for internal bookkeeping of allocations.
     * Each block of memory returned by allocate() is preceded by a header.
     */
    struct Header {
        Header* previous_header;        ///< Pointer to the previous header in the list.
        std::size_t allocated_size;     ///< Size of the following memory block in bytes.
    };
private:
    Storage_T* m_storage;
    Header* m_topHeader;            ///< Header of the top-most allocation.
public:
    Stack(Storage_T& storage) noexcept
        :m_storage(&storage), m_topHeader(nullptr)
    {}

    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        // we have to leave room for the header before the pointer that we return
        std::size_t free_space = getFreeMemory() - sizeof(Header);
        void* ptr = reinterpret_cast<void*>(m_storage->get() + getFreeMemoryOffset() + sizeof(Header));
        // the alignment has to be at least alignof(Header) to guarantee that the header is
        // stored at its natural alignment.
        // As usual, this assumes that all alignments are powers of two.
        if(!std::align(std::max(alignment, alignof(Header)), n, ptr, free_space)) {
            throw std::bad_alloc();
        }
        std::byte* ret = reinterpret_cast<std::byte*>(ptr);

        // setup a header in the memory region immediately preceding ret
        Header* new_header = reinterpret_cast<Header*>(ret - sizeof(Header));
        new_header->previous_header = m_topHeader;
        new_header->allocated_size = n;
        m_topHeader = new_header;

        this->onAllocate(n, alignment, ret);
        return ret;
    }

    void deallocate(std::byte* p, std::size_t n)
    {
        this->onDeallocate(p, n);

        // mark the deallocated block as freed in the header
        constexpr std::size_t const was_freed = std::numeric_limits<std::size_t>::max();
        Header* header_start = reinterpret_cast<Header*>(p - sizeof(Header));
        header_start->allocated_size = was_freed;

        // advance the top header to the lest until it no longer points to a freed header
        while(m_topHeader && (m_topHeader->allocated_size == was_freed)) {
            header_start = m_topHeader;
            m_topHeader = header_start->previous_header;
        }
    }

    /** Returns the offset in bytes from the start of the storage to the start of the free memory region.
     */
    std::size_t getFreeMemoryOffset() const noexcept
    {
        auto top_hdr = reinterpret_cast<std::byte*>(m_topHeader);
        return (m_topHeader == nullptr) ? 0 :
               ((top_hdr - m_storage->get()) + sizeof(Header) + m_topHeader->allocated_size);
    }

    /** Returns the size of the free memory region in bytes.
     */
    std::size_t getFreeMemory() const noexcept
    {
        return m_storage->size() - getFreeMemoryOffset();
    }
};
}
}
}
#endif
