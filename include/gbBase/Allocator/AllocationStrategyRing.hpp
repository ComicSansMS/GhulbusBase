#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_RING_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_RING_HPP

/** @file
*
* @brief Ring allocation strategy.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <gbBase/Allocator/DebugPolicy.hpp>
#include <gbBase/Allocator/StorageView.hpp>

#include <gbBase/Assert.hpp>

#include <cstddef>
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

/** The ring allocation strategy.
 * A ring is an extension of the Stack allocation strategy, that uses a doubly-linked list of Headers, instead of
 * the singly-linked list used by Stack. This allows memory to be reclaimed from both ends of the list, not just
 * the top, enabling both LIFO and FIFO style allocations, as well as mixes of the two.
 * Reclaiming memory from the beginning will leave a gap of free memory at the start of the storage buffer.
 * In order to make use of that memory, the ring will attempt to *wrap-around* when it runs out of memory
 * towards the end of the storage buffer. The wrap-around is imperfect in that a contiguous block cannot be wrapped,
 * thus the end of the storage buffer acts as a natural fragmentation point in the conceptually circular memory space.
 *
 * The following picture shows the internal state after 4 allocations p1 through p4. This state is very similar
 * to that of a Stack allocator, except that all Headers also maintain pointers to the next element.
 *  - Each allocated block is preceded by a Header and optionally by a padding region
 *    to satisfy alignment requirements.
 *  - Padding is performed such that each pN meets the requested alignment requirement *and*
 *    the preceding header meets the natural alignment requirement for Header.
 *  - Each header contains a pointer to the start of the previous header, the start of the next header,
 *    and a flag indicating whether the corresponding block was deallocated.
 *  - m_topHeader points to the top-most header that has not been deallocated.
 *  - m_bottomHeader points to the bottom-most header that has not been deallocated.
 *  - The start of the free memory is pointed to by m_freeMemoryOffset.
 *  - Memory can be reclaimed from both sides by moving the m_bottomHeader pointer to the right, or the
 *    m_topHeader pointer to the left.
 *  - Note that since headers do not track the size of their blocks, deallocation can only move
 *    the free memory offset back to the start of the header of the deallocated block, leaving the
 *    padding bytes in the unavailable memory region. If the next allocation now has a weaker alignment
 *    requirement, those bytes will be effectively lost. It would be possible to use a few additional
 *    bits in the header to store the alignment of the block, but this was not deemed worth the
 *    resulting runtime overhead. The lost bytes will get reclaimed when the previous block is freed.
 *    Padding bytes before the very first block will never be reclaimed. This only applies to deallocation
 *    from the top. Deallocation from the bottom always forwards the m_bottomHeader pointer to the
 *    next header, effectively reclaiming the padding area preceding that header.
 *
 * <pre>
 *   +----------------------<<-next_header-<<-------------------------------------+
 *   |                          +--<<-prev_header-<<--+                           |
 *   +--<<-prev_header-<<-------|----+           +----|--<<-prev_header-<<------ +|
 *   |      +-->>-next_header->>+    | +-next_h->+    | +->>-next_header->>-+    ||
 *   v      |                   v    | |         v    | |                   v    ||
 *   +--------+-------+---------+--------+-------+--------+-------+---------+--------+-------+-------------+
 *   | Header | Block | Padding | Header | Block | Header | Block | Padding | Header | Block | Free Memory |
 *   +--------+-------+---------+--------+-------+--------+-------+---------+--------+-------+-------------+
 *   ^        ^                          ^                ^                 ^        ^       ^
 *   |        p1                         p2               p3                |        p4      |
 *  m_storage.ptr    m_bottomHeader                                      m_topHeader      m_freeMemoryOffset
 *
 * </pre>
 *
 * The following picture illustrates the internal state of a wrapped-around ring. An allocation p5 is located
 * near the end of the ring, but all allocations preceding it have already been freed. The new allocation
 * p6 is too big to fit in the remaining free memory area to the right of p5, so the ring wraps around,
 * placing p6 at the beginning of the storage instead.
 *  - Once a ring has been wrapped around, the free memory at the end of the buffer becomes unavailable,
 *    until all of the bottom allocations preceding it have been freed.
 *  - In the wrapped-around case, m_freeMemoryOffset cannot grow bigger than m_bottomHeader.
 *
 * <pre>
 *        +--------------->>--prev_header-->>---------------------+
 *   +----|-------------------------<<--next_header--<<-----------|-----+
 *   +--<<|prev_header-<<--+                                      |     |
 *   |    |+-next_h->-+    |                                      |     |
 *   v    ||          v    |                                      v     |
 *   +--------+-------+--------+-------+--------------------------+--------+----------+--------------------+
 *   | Header | Block | Header | Block | Free Memory              | Header | Block    | Free Memory (unav.)|
 *   +--------+-------+--------+-------+--------------------------+--------+----------+--------------------+
 *   ^        ^       ^        ^       ^                          ^        ^
 *   |        p6      |        p7      |                          |        p5
 *  m_storage.ptr    m_topHeader    m_freeMemoryOffset           m_bottomHeader
 *
 * </pre>
 *
 * Upon deallocation:
 *  - The header for the corresponding allocation is marked as free.
 *  - The m_topHeader will be moved to the left along the list of previous headers until
 *    it no longer points to a header that is marked free.
 *  - The m_bottomHeader will be moved to the right along the list of next headers until
 *    it no longer points to a header that is marked free.
 *  - The m_freeMemoryOffset will point to the beginning of the last free header encountered
 *    from the top, or to the beginning of the storage if no more headers remain.
 *
 * @tparam Debug_T One of the DebugPolicy policies.
 */
template<typename Debug_T = Allocator::DebugPolicy::AllocateDeallocateCounter>
class Ring : private Debug_T {
public:
    /** Header used for internal bookkeeping of allocations.
     * Each block of memory returned by allocate() is preceded by a header.
     */
    class Header {
    private:
        /** Packed data field.
         * The header needs to store the following information:
         * - pointer to the next Header
         * - pointer to the previous Header
         * - flag indicating whether the block was freed
         * The flag is packed into the least significant bit of the previous pointer,
         * as that one is always 0 due to Header's own alignment requirements.
         */
        std::uintptr_t m_data[2];
    public:
        explicit Header(Header* previous_header)
        {
            static_assert(sizeof(Header*) == sizeof(std::uintptr_t));
            static_assert(alignof(Header) >= 2);
            m_data[0] = 0;
            std::memcpy(&m_data[1], &previous_header, sizeof(Header*));
        }

        void setNextHeader(Header* header)
        {
            GHULBUS_PRECONDITION_DBG(header && !m_data[0]);
            std::memcpy(&m_data, &header, sizeof(Header*));
        }

        void clearPreviousHeader()
        {
            GHULBUS_PRECONDITION_DBG((m_data[1] & ~(std::uintptr_t(0x01) )) != 0);
            std::uintptr_t const tmp = (m_data[1] & 0x01);
            std::memcpy(&m_data[1], &tmp, sizeof(Header*));
        }

        void clearNextHeader()
        {
            GHULBUS_PRECONDITION_DBG(m_data[0] != 0);
            m_data[0] = 0;
        }

        Header* nextHeader() const
        {
            Header* ret;
            std::memcpy(&ret, &m_data, sizeof(Header*));
            return ret;
        }

        Header* previousHeader() const
        {
            std::uintptr_t const tmp = m_data[1] & ~(std::uintptr_t(0x01));
            Header* ret;
            std::memcpy(&ret, &tmp, sizeof(Header*));
            return ret;
        }

        void markFree()
        {
            m_data[1] |= 0x01;
        }

        bool wasFreed() const
        {
            return ((m_data[1] & 0x01) != 0);
        }
    };
private:
    StorageView m_storage;
    Header* m_topHeader;                    ///< Header of the top-most (most-recent) allocation.
    Header* m_bottomHeader;                 ///< Header of the bottom-most (oldest) allocation.
    std::size_t m_freeMemoryOffset;         ///< Offset to the start of the free memory region in bytes

public:
    /** @tparam Storage_T A Storage type that can be used as an argument to makeStorageView().
     */
    template<typename Storage_T>
    explicit Ring(Storage_T& storage) noexcept
        :m_storage(makeStorageView(storage)), m_topHeader(nullptr), m_bottomHeader(nullptr), m_freeMemoryOffset(0)
    {}

    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        auto const getFreeMemoryContiguous = [this](std::size_t offs) -> std::size_t {
                std::byte* offs_ptr = (m_storage.ptr + offs);
                std::byte* bottom_ptr = reinterpret_cast<std::byte*>(m_bottomHeader);
                if(bottom_ptr < offs_ptr) {
                    // linear case: free space from offset to end of storage
                    return m_storage.size - offs;
                } else {
                    // wrap-around case: free space from offset to bottom header
                    return bottom_ptr - offs_ptr;
                }
            };
        // we have to leave room for the header before the pointer that we return
        std::size_t free_space = getFreeMemoryContiguous(m_freeMemoryOffset);
        bool const out_of_memory = (free_space < sizeof(Header));
        free_space -= (out_of_memory) ? 0 : sizeof(Header);
        void* ptr = reinterpret_cast<void*>(m_storage.ptr + getFreeMemoryOffset() + sizeof(Header));
        // the alignment has to be at least alignof(Header) to guarantee that the header is
        // stored at its natural alignment.
        // As usual, this assumes that all alignments are powers of two.
        if(out_of_memory || (!std::align(std::max(alignment, alignof(Header)), n, ptr, free_space))) {
            // we are out of memory, so try wrap around the ring
            if(isWrappedAround() || ((free_space = getFreeMemoryContiguous(0)) < sizeof(Header))) {
                // already wrapped, or not enough space for header even after wrapping
                throw std::bad_alloc();
            }
            free_space -= sizeof(Header);
            ptr = reinterpret_cast<void*>(m_storage.ptr + sizeof(Header));
            if(!std::align(std::max(alignment, alignof(Header)), n, ptr, free_space)) {
                // not enough free space in the beginning either
                throw std::bad_alloc();
            }
        }
        std::byte* ret = reinterpret_cast<std::byte*>(ptr);

        // setup a header in the memory region immediately preceding ret
        Header* new_header = new (ret - sizeof(Header)) Header(m_topHeader);
        if(m_topHeader == nullptr) {
            m_bottomHeader = new_header;
        } else {
            m_topHeader->setNextHeader(new_header);
        }
        m_topHeader = new_header;
        GHULBUS_ASSERT_DBG(m_bottomHeader);
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
            if(m_topHeader) {
                m_topHeader->clearNextHeader();
                m_freeMemoryOffset = (reinterpret_cast<std::byte*>(header_start) - m_storage.ptr);
            } else {
                GHULBUS_ASSERT_DBG(m_bottomHeader == header_start);
                m_bottomHeader = nullptr;
                m_freeMemoryOffset = 0;
            }
            header_start->~Header();
        }

        // advance the bottom header to the right until it no longer points to a freed header
        while(m_bottomHeader && (m_bottomHeader->wasFreed())) {
            header_start = m_bottomHeader;
            m_bottomHeader = header_start->nextHeader();
            if(m_bottomHeader) { m_bottomHeader->clearPreviousHeader(); }
            header_start->~Header();
        }
    }

    /** Returns the offset in bytes from the start of the storage to the start of the free memory region.
     */
    std::size_t getFreeMemoryOffset() const noexcept
    {
        return m_freeMemoryOffset;
    }

    /** Indicated whether the allocator is currently in the wrapped-around state.
     * A ring is wrapped if new allocations are taken from the beginning of the storage,
     * while there are still active allocations at the end of the storage.
     */
    bool isWrappedAround() const
    {
        // we are wrapped iff the current offset is left of the bottom header
        return (m_storage.ptr + getFreeMemoryOffset()) <= reinterpret_cast<std::byte*>(m_bottomHeader);
    }
};
}
}
}
#endif
