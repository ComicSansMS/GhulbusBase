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
        Header(Header* next_free_header)
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
    std::size_t m_chunkSize;
    Header* m_firstFree;
public:
    /** @tparam Storage_T A Storage type that can be used as an argument to makeStorageView().
     */
    template<typename Storage_T>
    explicit Pool(Storage_T& storage, std::size_t chunk_size) noexcept
        :m_storage(makeStorageView(storage)), m_chunkSize(chunk_size)
    {
        writeHeaders();
    }

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

    std::size_t getChunkSize() const
    {
        return m_chunkSize;
    }

    std::size_t getNumberOfFreeChunks() const
    {
        std::size_t ret = 0;
        for(Header const* it = m_firstFree; it != nullptr; it = it->nextFreeHeader()) {
            ++ret;
        }
        return ret;
    }

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
            // we iterate blocks from the end: [(num_chunks-1)..0]
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
