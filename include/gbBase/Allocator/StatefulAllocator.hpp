#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_STATEFUL_ALLOCATOR_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_STATEFUL_ALLOCATOR_HPP

/** @file
*
* @brief Standard-compliant stateful allocator.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <cstddef>
#include <type_traits>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Allocator
{
/** Stateful allocator.
 * This allocator implements the [Allocator Concept](http://en.cppreference.com/w/cpp/concept/Allocator) from
 * the C++ standard and can thus be used as a custom allocator for STL containers.
 * @tparam T The type of objects that this allocator will allocate.
 * @tparam State_T The underlying state. This should be one of the types from AllocationStrategy.
 */
template<typename T, typename State_T>
class StatefulAllocator {
public:
    using value_type = T;

    /** This is used for converting construction upon rebind. */
    template<typename, typename>
    friend class StatefulAllocator;
private:
    State_T* m_state;

public:
    StatefulAllocator(State_T& state) noexcept
        :m_state(&state)
    {}

    ~StatefulAllocator() = default;
    StatefulAllocator(StatefulAllocator const&) = default;
    StatefulAllocator& operator=(StatefulAllocator const&) = default;

    template<typename U>
    StatefulAllocator(StatefulAllocator<U, State_T> const& rhs)
        :m_state(rhs.m_state)
    {}

    /** Allocates a storage suitable for n objects of type T.
     */
    T* allocate(std::size_t n) {
        if constexpr(std::is_same_v<T, void>) {
            return reinterpret_cast<T*>(m_state->allocate(n, 1));
        } else {
            return reinterpret_cast<T*>(m_state->allocate(sizeof(T)*n, alignof(T)));
        }
    }

    /** Deallocates storage pointed to by p.
     * @pre p must be a value returned by a previous call to allocate()
     *      that has not yet been deallocated.
     */
    void deallocate(T* p, std::size_t n) {
        return m_state->deallocate(reinterpret_cast<std::byte*>(p), sizeof(T)*n);
    }

    /** Retrieve a pointer to the underlying AllocationStrategy.
     */
    State_T const* getState() const {
        return m_state;
    }
};

template<typename T, typename U, typename State_T>
bool operator==(StatefulAllocator<T, State_T> const& lhs, StatefulAllocator<U, State_T> const& rhs) {
    return lhs.getState() == rhs.getState();
}

template<typename T, typename U, typename State_T>
bool operator!=(StatefulAllocator<T, State_T> const& lhs, StatefulAllocator<U, State_T> const& rhs) {
    return !(lhs == rhs);
}
}
}
#endif
