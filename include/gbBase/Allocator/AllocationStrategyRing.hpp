#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_RING_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_ALLOCATION_STRATEGY_RING_HPP

/** @file
*
* @brief Ring allocation strategy.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <gbBase/Allocator/DebugPolicy.hpp>
#include <gbBase/Allocator/Storage.hpp>

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
 */
template<typename Storage_T, typename Debug_T = Allocator::DebugPolicy::AllocateDeallocateCounter>
class Ring : private Debug_T {
public:
    /** Header used for internal bookkeeping of allocations.
     * Each block of memory returned by allocate() is preceded by a header.
     */
    class Header {
    private:
        std::uintptr_t m_data[2];
    public:
        Header(Header* previous_header)
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
    Storage_T* m_storage;
    Header* m_topHeader;
    Header* m_bottomHeader;
    std::size_t m_freeMemoryOffset;

public:
    Ring(Storage_T& storage) noexcept
        :m_storage(&storage), m_topHeader(nullptr), m_bottomHeader(nullptr), m_freeMemoryOffset(0)
    {}

    std::byte* allocate(std::size_t n, std::size_t alignment)
    {
        // we have to leave room for the header before the pointer that we return
        std::size_t free_space = getFreeMemoryContiguous(m_freeMemoryOffset);
        bool const out_of_memory = (free_space < sizeof(Header));
        free_space -= (out_of_memory) ? 0 : sizeof(Header);
        void* ptr = reinterpret_cast<void*>(m_storage->get() + getFreeMemoryOffset() + sizeof(Header));
        // the alignment has to be at least alignof(Header) to guarantee that the header is
        // stored at its natural alignment.
        // As usual, this assumes that all alignments are powers of two.
        if(out_of_memory || (!std::align(std::max(alignment, alignof(Header)), n, ptr, free_space))) {
            // we are out of memory, so try wrap around the ring
            if(isWrappedAround() || ((free_space = getFreeMemoryContiguous(0)) < sizeof(Header))) {
                // already wrapped, or not enough space even after wrapping
                throw std::bad_alloc();
            }
            free_space -= sizeof(Header);
            ptr = reinterpret_cast<void*>(m_storage->get() + sizeof(Header));
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
        m_freeMemoryOffset = (ret - m_storage->get()) + n;

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
                m_freeMemoryOffset = (reinterpret_cast<std::byte*>(header_start) - m_storage->get());
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

    bool isWrappedAround() const
    {
        // we are wrapped iff the current offset is left of the bottom header
        return (m_storage->get() + getFreeMemoryOffset()) <= reinterpret_cast<std::byte*>(m_bottomHeader);
    }

    /** Get the biggest contiguous block starting from offs.
     * @pre offs must not point into allocated memory.
     */
    std::size_t getFreeMemoryContiguous(std::size_t offs) const noexcept
    {
        std::byte* offs_ptr = (m_storage->get() + offs);
        std::byte* bottom_ptr = reinterpret_cast<std::byte*>(m_bottomHeader);
        if(bottom_ptr < offs_ptr) {
            // linear case: free space from offset to end of storage
            return m_storage->size() - offs;
        } else {
            // wrap-around case: free space from offset to bottom header
            return bottom_ptr - offs_ptr;
        }
    }
};
}
}
}
#endif
