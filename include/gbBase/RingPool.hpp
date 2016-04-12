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
    }

    template<typename FallbackPolicy_T>
    struct Policies {
        typedef FallbackPolicy_T FallbackPolicy;
    };

    using DefaultPolicies = Policies<FallbackPolicies::ReturnNullptr>;
}

template<class Policies_T>
class RingPool_T
{
private:
    std::atomic<std::size_t> m_rightPtr;    // offset to the beginning of the unallocated memory
    std::atomic<std::size_t> m_leftPtr;     // offset to the beginning of the allocated memory
    std::size_t const m_poolCapacity;
    std::unique_ptr<char[]> m_storage;
public:
    inline RingPool_T(std::size_t poolCapacity);

    RingPool_T(RingPool_T const&) = delete;
    RingPool_T& operator=(RingPool_T const&) = delete;

    inline void* allocate(std::size_t requested_size);

    inline void free(void* ptr);
};

template<class Policies_T>
RingPool_T<Policies_T>::RingPool_T(std::size_t poolCapacity)
    :m_rightPtr(0), m_leftPtr(0), m_poolCapacity(poolCapacity),
     m_storage(std::make_unique<char[]>(poolCapacity))
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
                        auto const basePtr = m_storage.get();
                        std::memcpy(basePtr, &requested_size, sizeof(std::size_t));
                        return basePtr + sizeof(size_t);
                    }
                }
            } else {
                // allocate at rightPtr
                if(m_rightPtr.compare_exchange_weak(expected_right, expected_right + requested_size)) {
                    auto const basePtr = m_storage.get() + expected_right;
                    std::memcpy(basePtr, &requested_size, sizeof(std::size_t));
                    return basePtr + sizeof(size_t);
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
                    auto const basePtr = m_storage.get() + expected_right;
                    std::memcpy(basePtr, &requested_size, sizeof(std::size_t));
                    return basePtr + sizeof(size_t);
                }
            }
        }
    }
}

template<class Policies_T>
void RingPool_T<Policies_T>::free(void* ptr)
{
    char* baseptr = static_cast<char*>(ptr) - sizeof(std::size_t);
    std::size_t elementSize;
    std::memcpy(&elementSize, baseptr, sizeof(std::size_t));
    GHULBUS_ASSERT_MESSAGE(baseptr == m_storage.get() + m_leftPtr,
                           "RingPool elements must be freed in the order in which they were allocated.");
    m_leftPtr.fetch_add(elementSize);
    // todo: what if we free the last element at the end of the ring buffer?
}

using RingPool = RingPool_T<RingPoolPolicies::DefaultPolicies>;
}
}

#endif
