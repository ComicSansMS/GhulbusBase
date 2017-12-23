#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_POOL_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_POOL_HPP

/** @file
*
* @brief Pool allocation strategy.
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

/** The pool allocation strategy.
 * A pool partitions its storage into equally sized chunks of memory and with each allocation always hands out
 * a complete chunk. The allocator maintains a linked list of free chunks, where new allocations always take the
 * first element from the list and deallocation inserts the respective chunk at the beginning of the list.
 * Initially, the list is ordered such that subsequent allocations are handed out in order of ascending addresses.
 *
 * The following picture shows the internal state after three allocations p1 through p3, where p2 was already
 * deallocated again.
 *  - Each block handed out by the allocator has the same size, regardless of how much memory was requested
 *    (if the requested amount was larger than the chunk size, allocation will fail).
 *  - A singly-linked list of free chunks is maintained, with the most recently deallocated chunk at the front.
 *  - Allocations always hand out the first free chunk.
 *  - If the total size of the storage is not evenly divisible into chunks, the leftover memory will be
 *    unavailable. Use the calculateStorageSize() function to determine correct storage sizes so that no
 *    memory will be wasted.
 * <pre>
 *                               +---->>>-------next_free------>>>--------+    +->-next_free---> nullptr
 *                               |                                        v    |
 *   +--------+-------------+--------+-------------+--------+-------------+--------+-------------+---------+
 *   | Header |  Block      | Header |  Block      | Header |  Block      | Header |  Block      |.(unav.).|
 *   +--------+-------------+--------+-------------+--------+-------------+--------+-------------+---------+
 *   ^        ^             ^                               ^
 *   |        p1            |                               p3
 *  m_storage.ptr          m_firstFree
 * </pre>
 *
 * The following picutre illustrates how padding is used to fulfill alignment requirements. Here the allocation
 * p3 had an alignment requirement that caused padding bytes to be inserted between the header and p3.
 *  - Padding does not grow the chunk. If the requested alignment is stricter than the natural alignment of the
 *    Header, the maximum available allocation size shrinks accordingly.
 *  - Unlike Ring and Stack allocation strategies, the padding is inserted between Header and the pointer given
 *    back to the user. That way, all headers have deterministic start addresses.
 *
 * <pre>
 *   +--------+-------------+--------+-------------+--------+-----+-------+--------+-------------+---------+
 *   | Header |  Block      | Header |  Block      | Header | Pad | Block | Header |  Block      |.(unav.).|
 *   +--------+-------------+--------+-------------+--------+-----+-------+--------+-------------+---------+
 *   ^        ^                      ^                            ^                ^
 *   |        p1                     p2                           p3               p4
 *  m_storage.ptr
 * </pre>
 *
 * Upon deallocation:
 *  - The corresponding header is found by going to the closest header start address to the left of the
 *    pointer. For weak alignments, this is always `p - sizeof(Header)`, while for alginments stricter than
 *    the natural alignment of Header, additional padding bytes might have to be skipped.
 *  - Once found, the header is prepended to the list of free Headers and the m_firstFree will be pointed to it.
 *
 */
template<typename Debug_T = Allocator::DebugPolicy::AllocateDeallocateCounter>
class Pool : private Debug_T
{
public:
    /** Header used for internal bookkeeping of allocations.
     * Each block of memory returned by allocate() is preceded by a header.
     */
    class Header {
    private:
        /** Packed data field.
         * The Header needs to store two pieces of information: A pointer to the next free header and a flag
         * indicating whether the header was freed. As a space optimization, the flag is packed into the
         * least significant bit of the pointer, as that one is always 0 due to the Header' own alignment
         * requirements.
         */
        std::uintptr_t m_data;
    public:
        explicit Header(Header* next_free_header)
        {
            static_assert(sizeof(Header*) == sizeof(std::uintptr_t));
            static_assert(alignof(Header) >= 2);
            std::memcpy(&m_data, &next_free_header, sizeof(std::uintptr_t));
            m_data |= 0x01;
        }

        Header* nextFreeHeader() const
        {
            GHULBUS_PRECONDITION_DBG_MESSAGE(m_data != 0, "Cannot retrieve next header from an occupied header.");
            std::uintptr_t tmp;
            tmp = m_data & ~(std::uintptr_t(0x01));
            Header* ret;
            std::memcpy(&ret, &tmp, sizeof(std::uintptr_t));
            return ret;
        }

        void setNextFreeHeader(Header* next_free_header)
        {
            GHULBUS_PRECONDITION_DBG_MESSAGE(m_data == 0, "Cannot change next header on a free header.");
            std::uintptr_t tmp;
            std::memcpy(&tmp, &next_free_header, sizeof(std::uintptr_t));
            m_data = tmp | 0x01;
        }

        void markOccupied()
        {
            GHULBUS_PRECONDITION_DBG_MESSAGE(m_data != 0, "Cannot mark an occupied header as occupied.");
            m_data = 0;
        }

        bool isFree() const
        {
            return ((m_data & 0x01) != 0);
        }
    };
private:
    StorageView m_storage;
    std::size_t m_chunkSize;            ///< Size of a memory chunk handed out by a call to allocate() in bytes.
    Header* m_firstFree;                ///< Pointer to the header of the first free chunk, nullptr if none available.
public:
    /** @tparam Storage_T A Storage type that can be used as an argument to makeStorageView().
     * @param[in] storage Storage.
     * @param[in] chunk_size Size of the chunks given out by each allocate call in bytes.
     */
    template<typename Storage_T>
    Pool(Storage_T& storage, std::size_t chunk_size)
        :m_storage(makeStorageView(storage)), m_chunkSize(chunk_size)
    {
        void* ptr = reinterpret_cast<void*>(m_storage.ptr);
        if(!std::align(alignof(Header), sizeof(Header), ptr, m_storage.size)) {
            throw std::bad_alloc();
        }
        m_storage.ptr = reinterpret_cast<std::byte*>(ptr);
        writeHeaders();
    }

    /** Helper function for calculating ideal storage sizes for use with the Pool allocator.
     * Returns the storage size suitable for storing `number_of_chunks` chunks of `chunk_size` bytes each.
     * Note that storage must be aligned on `alignof(Header)` boundaries for the returned size to meet those
     * requirements.
     */
    static constexpr std::size_t calculateStorageSize(std::size_t chunk_size, std::size_t number_of_chunks)
    {
        return (chunk_size + sizeof(Header)) * number_of_chunks;
    }

    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        if(!m_firstFree) {
            throw std::bad_alloc();
        }

        Header* header = m_firstFree;
        std::size_t free_space = m_chunkSize;
        void* ptr = reinterpret_cast<void*>(header + 1);
        if(!std::align(alignment, n, ptr, free_space)) {
            throw std::bad_alloc();
        }
        std::byte* const ret = reinterpret_cast<std::byte*>(ptr);

        m_firstFree = header->nextFreeHeader();
        header->markOccupied();

        this->onAllocate(n, alignment, ret);
        return ret;
    }

    void deallocate(std::byte* p, std::size_t n)
    {
        this->onDeallocate(p, n);
        // since alignment might require inserting an arbitrary number
        // of padding bytes between the header and p, we calculate the
        // index of the chunk that p points into
        std::size_t const size = m_chunkSize + sizeof(Header);
        std::size_t const chunk_index = (p - m_storage.ptr) / size;
        Header* header = reinterpret_cast<Header*>(m_storage.ptr + chunk_index * size);
        header->setNextFreeHeader(m_firstFree);
        m_firstFree = header;
    }

    /** The size of a chunk in bytes.
     * Each call to allocate always gives out a complete chunk, no matter how much memory was requested.
     */
    std::size_t getChunkSize() const
    {
        return m_chunkSize;
    }

    /** Retrieve the number of chunks that are currently available for allocation.
     */
    std::size_t getNumberOfFreeChunks() const
    {
        std::size_t ret = 0;
        for(Header const* it = m_firstFree; it != nullptr; it = it->nextFreeHeader()) {
            ++ret;
        }
        return ret;
    }

    /** Resets the allocator, restoring the initial order of the free chunk list.
     * This function must only be called after all previously allocated blocks have been deallocated.
     */
    void reset()
    {
        this->onReset();
        for(Header* it = m_firstFree; it != nullptr; it = it->nextFreeHeader()) {
            it->~Header();
        }
        writeHeaders();
    }
private:
    void writeHeaders()
    {
        std::size_t num_chunks = m_storage.size / (m_chunkSize + sizeof(Header));
        Header* next_header = nullptr;
        for(std::size_t i = 0; i < num_chunks; ++i)
        {
            // we iterate blocks from the end: [(num_chunks-1)..0], so that the resulting list
            // is ordered by ascending addresses
            std::size_t const block_index = num_chunks - i - 1;
            std::byte* const block = m_storage.ptr + (block_index * (m_chunkSize + sizeof(Header)));
            next_header = new (block) Header(next_header);
        }
        m_firstFree = next_header;
    }
};
}
}
}
#endif
