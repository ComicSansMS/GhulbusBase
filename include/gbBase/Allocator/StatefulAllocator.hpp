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
 * This allocator implements the Allocator Concept from the C++ standard and can thus be used
 * as a custom allocator for STL containers.
 * @tparam T The type of objects that this allocator will allocate.
 * @tparam State_T The underlying state. This should be one of the types from AllocationStrategy.
 */
template<typename T, typename State_T>
class StatefulAllocator {
public:
    using value_type = T;

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

    T* allocate(std::size_t n) {
        if constexpr(std::is_same_v<T, void>) {
            return reinterpret_cast<T*>(m_state->allocate(n, 1));
        } else {
            return reinterpret_cast<T*>(m_state->allocate(sizeof(T)*n, alignof(T)));
        }
    }

    void deallocate(T* p, std::size_t n) {
        return m_state->deallocate(reinterpret_cast<std::byte*>(p), n);
    }

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
