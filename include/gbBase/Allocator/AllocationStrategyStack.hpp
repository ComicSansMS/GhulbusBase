#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_STACK_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_STACK_HPP

/** @file
*
* @brief Stack allocation strategy.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <gbBase/Allocator/DebugPolicy.hpp>
#include <gbBase/Allocator/StorageView.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
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
 *  - Each header contains a pointer to the start of the previous header and a flag indicating
 *    whether the corresponding block was deallocated.
 *  - m_topHeader points to the top-most header that has not been deallocated.
 *  - The start of the free memory is pointed to by m_freeMemoryOffset.
 *  - Note that since headers do not track the size of their blocks, deallocation can only move
 *    the free memory offset back to the start of the header of the deallocated block, leaving the
 *    padding bytes in the unavailable memory region. If the next allocation now has a weaker alignment
 *    requirement, those bytes will be effectively lost. It would be possible to use a few additional
 *    bits in the header to store the alignment of the block, but this was not deemed worth the
 *    resulting runtime overhead. The lost bytes will get reclaimed when the previous block is freed.
 *    Padding bytes before the very first block will never be reclaimed.
 *
 * <pre>
 *                              +---prev_header-------+
 *   +------prev_header---------|----+           +----|-----prev_header----------+
 *   v        <-size-->         v    |   <-size--v    |   <--size->              |   <--size->
 *   +--------+-------+---------+--------+-------+--------+-------+---------+--------+-------+-------------+
 *   | Header | Block | Padding | Header | Block | Header | Block | Padding | Header | Block | Free Memory |
 *   +--------+-------+---------+--------+-------+--------+-------+---------+--------+-------+-------------+
 *   ^        ^                          ^                ^                 ^        ^       ^
 *   |        p1                         p2               p3                |        p4      |
 * m_storage.ptr                                                       m_topHeader       m_freeMemoryOffset
  </pre>
 *
 * Upon deallocation:
 *  - The header for the corresponding allocation is marked as free.
 *  - The m_topHeader will be moved to the left along the list of previous headers until
 *    it no longer points to a header that is marked free.
 *  - The m_freeMemoryOffset will point to the beginning of the last free header encountered.
 *
 * @tparam Debug_T One of the DebugPolicy policies.
 */
template<typename Debug_T = Allocator::DebugPolicy::AllocateDeallocateCounter>
class Stack : private Debug_T {
public:
    /** Header used for internal bookkeeping of allocations.
     * Each block of memory returned by allocate() is preceded by a header.
     */
    class Header {
    private:
        /** Packed data field.
         * The Header needs to store two pieces of information: A pointer to the previous header and a flag
         * indicating whether the header was freed. As a space optimization, the flag is packed into the
         * least significant bit of the pointer, as that one is always 0 due to the Header' own alignment
         * requirements.
         */
        std::uintptr_t m_data;
    public:
        Header(Header* previous_header)
        {
            static_assert(sizeof(Header*) == sizeof(std::uintptr_t));
            static_assert(alignof(Header) >= 2);
            std::memcpy(&m_data, &previous_header, sizeof(std::uintptr_t));
        }

        Header* previousHeader() const
        {
            std::uintptr_t tmp;
            tmp = m_data & ~(std::uintptr_t(0x01));
            Header* ret;
            std::memcpy(&ret, &tmp, sizeof(std::uintptr_t));
            return ret;
        }

        void markFree()
        {
            m_data |= 0x01;
        }

        bool wasFreed() const
        {
            return ((m_data & 0x01) != 0);
        }
    };
private:
    StorageView m_storage;
    Header* m_topHeader;                ///< Header of the top-most allocation.
    std::size_t m_freeMemoryOffset;     ///< Offset to the start of the free memory region in bytes.
public:
    /** @tparam Storage_T A Storage type that can be used as an argument to makeStorageView().
    */
    template<typename Storage_T>
    explicit Stack(Storage_T& storage) noexcept
        :m_storage(makeStorageView(storage)), m_topHeader(nullptr), m_freeMemoryOffset(0)
    {}

    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        // we have to leave room for the header before the pointer that we return
        std::size_t free_space = getFreeMemory();
        if(free_space < sizeof(Header)) {
            // not even enough memory left to store header
            throw std::bad_alloc();
        }
        free_space -= sizeof(Header);
        void* ptr = reinterpret_cast<void*>(m_storage.ptr + getFreeMemoryOffset() + sizeof(Header));
        // the alignment has to be at least alignof(Header) to guarantee that the header is
        // stored at its natural alignment.
        // As usual, this assumes that all alignments are powers of two.
        if(!std::align(std::max(alignment, alignof(Header)), n, ptr, free_space)) {
            throw std::bad_alloc();
        }
        std::byte* ret = reinterpret_cast<std::byte*>(ptr);

        // setup a header in the memory region immediately preceding ret
        Header* new_header = new (ret - sizeof(Header)) Header(m_topHeader);
        m_topHeader = new_header;
        m_freeMemoryOffset = (ret - m_storage.ptr) + n;

        this->onAllocate(n, alignment, ret);
        return ret;
    }

    void deallocate(std::byte* p, std::size_t n)
    {
        this->onDeallocate(p, n);

        // mark the deallocated block as freed in the header
        Header* header_start = reinterpret_cast<Header*>(p - sizeof(Header));
        header_start->markFree();

        // advance the top header to the left until it no longer points to a freed header
        while(m_topHeader && (m_topHeader->wasFreed())) {
            header_start = m_topHeader;
            m_topHeader = header_start->previousHeader();
            m_freeMemoryOffset = (reinterpret_cast<std::byte*>(header_start) - m_storage.ptr);
            header_start->~Header();
        }
    }

    /** Returns the offset in bytes from the start of the storage to the start of the free memory region.
     */
    std::size_t getFreeMemoryOffset() const noexcept
    {
        return m_freeMemoryOffset;
    }

    /** Returns the size of the free memory region in bytes.
     */
    std::size_t getFreeMemory() const noexcept
    {
        return m_storage.size - getFreeMemoryOffset();
    }
};
}
}
}
#endif
