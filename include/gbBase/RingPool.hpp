#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_RING_POOL_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_RING_POOL_HPP

/** @file
*
* @brief Ring pool allocator.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <gbBase/Assert.hpp>

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
    std::vector<std::size_t> m_freeList;
public:
    inline RingPool_T(std::size_t poolCapacity);

    RingPool_T(RingPool_T const&) = delete;
    RingPool_T& operator=(RingPool_T const&) = delete;

    void* allocate(std::size_t requested_size);

    void free(void* ptr);

private:
    inline void* allocate_block_at(std::size_t offset, std::size_t size);
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
                    return Policies_T::FallbackPolicy::allocate_failed();
                } else {
                    // allocate from the beginning
                    if(m_rightPtr.compare_exchange_weak(expected_right, requested_size)) {
                        // we wrapped around the ring; set the padding pointer so that we can skip the padding
                        // area when this block is freed again
                        std::size_t expected_padding = 0;
                        auto const res = m_paddingPtr.compare_exchange_strong(expected_padding, expected_right);
                        GHULBUS_ASSERT(res);
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
            // check if the leftPtr element is in the free list;
            // since freeing elements at leftPtr does not check the freelist, we won't notice if leftPtr moves to an
            // element on the free list
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
            } else {
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

using RingPool = RingPool_T<RingPoolPolicies::DefaultPolicies>;
}
}

#endif
