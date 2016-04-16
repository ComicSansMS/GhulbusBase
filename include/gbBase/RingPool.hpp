#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_RING_POOL_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_RING_POOL_HPP

/** @file
*
* @brief Ring pool allocator.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <gbBase/Assert.hpp>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Memory
{
namespace RingPoolPolicies
{
    namespace FallbackPolicies
    {
        struct ReturnNullptr
        {
            static void* allocate_failed()
            {
                return nullptr;
            }
        };

        struct Assert
        {
            static void* allocate_failed()
            {
                GHULBUS_ASSERT_MESSAGE(false, "Allocator capacity exceeded.");
                return nullptr;
            }
        };

        template<typename Exception_T = std::bad_alloc>
        struct Throw
        {
            static void* allocate_failed()
            {
                throw Exception_T();
            }
        };

        // @todo: fallback allocator
    }

    template<typename FallbackPolicy_T>
    struct Policies {
        typedef FallbackPolicy_T FallbackPolicy;
    };

    using DefaultPolicies = Policies<FallbackPolicies::ReturnNullptr>;
}

/**
 * - freeing never blocks allocations
 * - allocations never block each other
 * - freeing blocks if order is different from allocation order
 * Freeing elements out-of-order pushed them onto the free list instead. The free list is maintained lazily.
 * An element will stay on the free list even if the elements that blocked it from being freed are no longer allocated.
 * The free list will be cleaned from elements that are eligible for being freed only when
 *  - A free() call cannot free its argument and has to put it on the free list. The rationale here is that since we
 *    anyway need to acquire the lock to put a new element on the free list, we might as well clean it up. The lazy
 *    cleanup approach has the advantage that there is no overhead at all for the case where elements are freed in the
 *    order of allocation. Note that the cleanup might result in all elements being freed, including the one that
 *    triggered the cleanup in the first place.
 *  - An allocate() call cannot find a suitably big memory block. This is not ideal, as allocations are supposed to be
 *    fast, but it is sometimes unavoidable. For instance, if the element allocated first is freed last, all other
 *    elements will stay on the free list. That way, large chunks of memory can be occupied by elements on the
 *    free list and it might be hard to get another allocation to succeed due to lack of free memory.
 *    However, if we cannot get an allocation through, we also won't get another free call, which might effectively
 *    lock us out of the pool. We thus also attempt a free list clean on failing allocations. Since failing allocations
 *    should be an exceptional case, this seems acceptable.
 *  - The user calls cleanPendingElementsFromFreeList(). Consider doing this if you know that a particular free call
 *    makes a large portion of the free list eligible for cleanup and you'd rather pay the cost for the cleanup now
 *    than on the next free() or allocate().
 */
template<class Policies_T>
class RingPool_T
{
private:
    std::atomic<std::size_t> m_rightPtr;    // offset to the beginning of the unallocated memory region
    std::atomic<std::size_t> m_leftPtr;     // offset to the beginning of the allocated memory region
    std::size_t const m_poolCapacity;
    std::unique_ptr<char[]> m_storage;
    std::atomic<std::size_t> m_paddingPtr;   // in case the last allocation in the buffer does not exactly hit
                                             // the ring boundary, we have to leave some empty space at the end.
                                             // if this happens, this offset points to the start of the empty area,
                                             // that extends until m_poolCapacity. will be 0 if the end of the buffer
                                             // is currently in the unallocated region (between rightPtr and leftPtr).
    std::mutex m_freeListMutex;
    std::vector<std::size_t> m_freeList;     // elements can only be freed in the order in which they were allocated;
                                             // elements that are freed out-of-order will be moved to the free list
                                             // instead and await their turn.
public:
    inline RingPool_T(std::size_t poolCapacity);

    RingPool_T(RingPool_T const&) = delete;
    RingPool_T& operator=(RingPool_T const&) = delete;

    void* allocate(std::size_t requested_size);

    void free(void* ptr);

    bool cleanPendingElementsFromFreeList();

private:
    inline void* allocate_block_at(std::size_t offset, std::size_t size);

    /** Private helper containing common parts of free() and cleanPendingElementsFromFreeList().
     * @attention Only safe to call while lock to m_freeListMutex is being held.
     */
    inline bool tryToFreeNextElementInFreeList(std::size_t& leftPtr, std::size_t const& paddingPtr);
};

template<class Policies_T>
RingPool_T<Policies_T>::RingPool_T(std::size_t poolCapacity)
    :m_rightPtr(0), m_leftPtr(0), m_poolCapacity(poolCapacity),
     m_storage(std::make_unique<char[]>(poolCapacity)), m_paddingPtr(0)
{
}

template<class Policies_T>
void* RingPool_T<Policies_T>::allocate(std::size_t requested_size)
{
    requested_size += sizeof(size_t);
    for(;;) {
        auto expected_right = m_rightPtr.load();
        auto const left = m_leftPtr.load();
        if(left <= expected_right) {
            // rightPtr is right of leftPtr; allocated section does not span ring boundaries
            if(expected_right + requested_size >= m_poolCapacity) {
                // no room to the right, attempt wrap-around and allocate at the beginning
                if(requested_size >= left) {
                    // requested data too big; does not fit empty space
                    if(cleanPendingElementsFromFreeList()) { continue; }
                    return Policies_T::FallbackPolicy::allocate_failed();
                } else {
                    // allocate from the beginning
                    if(m_rightPtr.compare_exchange_weak(expected_right, requested_size)) {
                        // we wrapped around the ring; set the padding pointer so that we can skip the padding
                        // area when this block is freed again
                        std::size_t expected_padding = 0;
                        auto const res = m_paddingPtr.compare_exchange_strong(expected_padding, expected_right);
                        GHULBUS_ASSERT_DBG(res);
                        return allocate_block_at(0u, requested_size);
                    }
                }
            } else {
                // allocate at rightPtr
                if(m_rightPtr.compare_exchange_weak(expected_right, expected_right + requested_size)) {
                    return allocate_block_at(expected_right, requested_size);
                }
            }
        } else {
            // rightPtr is left of leftPtr; allocated section does span ring boundaries
            if(expected_right + requested_size >= left) {
                // requested data too big; does not fit empty space
                if(cleanPendingElementsFromFreeList()) { continue; }
                return Policies_T::FallbackPolicy::allocate_failed();
            } else {
                // allocate at rightPtr
                if(m_rightPtr.compare_exchange_weak(expected_right, expected_right + requested_size)) {
                    return allocate_block_at(expected_right, requested_size);
                }
            }
        }
    }
}

template<class Policies_T>
void* RingPool_T<Policies_T>::allocate_block_at(std::size_t offset, std::size_t size)
{
    auto const basePtr = m_storage.get() + offset;
    std::memcpy(basePtr, &size, sizeof(std::size_t));
    return basePtr + sizeof(size_t);
}

template<class Policies_T>
void RingPool_T<Policies_T>::free(void* ptr)
{
    if(!ptr) {
        return;
    }
    char* baseptr = static_cast<char*>(ptr) - sizeof(std::size_t);
    std::size_t elementSize;
    std::memcpy(&elementSize, baseptr, sizeof(std::size_t));
    std::size_t const element_index = baseptr - m_storage.get();
    if(element_index == m_leftPtr)
    {
        // ptr is the element at left; free it immediately
        m_leftPtr.fetch_add(elementSize);
        if (m_leftPtr == m_paddingPtr)
        {
            // we freed the last element at the end of the ring buffer; wrap around and clear padding
            m_leftPtr.store(0);
            m_paddingPtr.store(0);
        }
    } else
    {
        std::unique_lock<std::mutex> lk(m_freeListMutex);
        auto const leftPtr_old = m_leftPtr.load();
        auto leftPtr = leftPtr_old;
        auto const paddingPtr = m_paddingPtr.load();
        bool elementWasFreed = false;
        for(;;) {
            if(!tryToFreeNextElementInFreeList(leftPtr, paddingPtr)) {
                // no element on the free list hit; but maybe the original element became free in an earlier iteration
                if(leftPtr == element_index) {
                    leftPtr += elementSize;
                    if(leftPtr == paddingPtr) {
                        leftPtr = 0;
                    }
                    elementWasFreed = true;
                } else {
                    // no more elements left to be freed
                    break;
                }
            }
        }
        if(leftPtr != leftPtr_old) {
            m_leftPtr.store(leftPtr);
            if(leftPtr < leftPtr_old) {
                m_paddingPtr.store(0);
            }
        }
        if(!elementWasFreed) {
            m_freeList.push_back(element_index);
        }
    }
}

template<class Policies_T>
bool RingPool_T<Policies_T>::cleanPendingElementsFromFreeList()
{
    std::unique_lock<std::mutex> lk(m_freeListMutex);
    auto const leftPtr_old = m_leftPtr.load();
    auto leftPtr = leftPtr_old;
    auto const paddingPtr = m_paddingPtr.load();
    while(tryToFreeNextElementInFreeList(leftPtr, paddingPtr))
        ;
    if(leftPtr != leftPtr_old) {
        m_leftPtr.store(leftPtr);
        if(leftPtr < leftPtr_old) {
            m_paddingPtr.store(0);
        }
        return true;
    } else {
        return false;
    }
}

template<class Policies_T>
bool RingPool_T<Policies_T>::tryToFreeNextElementInFreeList(std::size_t& leftPtr, std::size_t const& paddingPtr)
{
    auto const it = std::find(begin(m_freeList), end(m_freeList), leftPtr);
    if(it != end(m_freeList)) {
        // we found an element to be freed
        std::size_t it_element_size;
        std::memcpy(&it_element_size, m_storage.get() + (*it), sizeof(std::size_t));
        leftPtr += it_element_size;
        if(leftPtr == paddingPtr) {
            leftPtr = 0;
        }
        m_freeList.erase(it);
        return true;
    } else {
        return false;
    }
}

using RingPool = RingPool_T<RingPoolPolicies::DefaultPolicies>;
}
}

#endif
