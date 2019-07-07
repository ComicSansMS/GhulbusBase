#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_FIXED_RING_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_FIXED_RING_HPP

/** @file
*
* @brief Fixed-size ring buffer.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>
#include <gbBase/Assert.hpp>

#include <memory>
#include <vector>

namespace GHULBUS_BASE_NAMESPACE
{

template<typename T, class Allocator = std::allocator<T>>
class FixedRing {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = typename std::vector<T>::size_type;
    using reference = value_type&;
    using const_reference = value_type const&;
private:
    std::vector<T, Allocator> m_ring;
    size_type m_push_idx;
    size_type m_n_elements;

    size_type front_index() const
    {
        size_type const end_idx = (m_push_idx < m_n_elements) ? (m_push_idx + m_ring.capacity()) : m_push_idx;
        return end_idx - m_n_elements;
    }

    size_type nth_index(size_type n) const
    {
        auto const offs = m_n_elements - n;
        return (m_push_idx < offs) ? (m_push_idx + m_ring.capacity() - offs) : (m_push_idx - offs);
    }
public:
    explicit FixedRing(size_type capacity, Allocator const& alloc = Allocator())
        :m_ring(alloc), m_push_idx(0), m_n_elements(0)
    {
        GHULBUS_PRECONDITION(capacity > 0);
        m_ring.reserve(capacity);
    }

    ~FixedRing() = default;
    FixedRing(FixedRing const&) = default;
    FixedRing& operator=(FixedRing const&) = default;
    FixedRing(FixedRing&&) = default;
    FixedRing& operator=(FixedRing&&) = default;

    /** Push an element to the back of the ring buffer.
     * \pre !full()
     */
    void push_back(T const& v)
    {
        GHULBUS_PRECONDITION(!full());
        if (m_ring.size() < m_ring.capacity()) {
            m_ring.push_back(v);
            ++m_push_idx;
            ++m_n_elements;
            return;
        }
        if (m_push_idx == m_ring.capacity()) { m_push_idx = 0; }
        m_ring[m_push_idx++] = v;
        ++m_n_elements;
    }

    /** Retrieve an element from the front of the ring buffer.
     * \pre !empty()
     */
    reference pop_front()
    {
        GHULBUS_PRECONDITION(!empty());
        auto const front_idx = front_index();
        --m_n_elements;
        return m_ring[front_idx];
    }

    /** Maximum number of elements that the ring buffer can hold at once.
     */
    size_type capacity() const noexcept
    {
        return m_ring.capacity();
    }

    /** Number of elements currently stored in the ring buffer.
     */
    size_type available() const noexcept
    {
        return m_n_elements;
    }

    /** Number of free slots available for storing additional elements in the ring buffer.
     */
    size_type free() const noexcept
    {
        return m_ring.capacity() - m_n_elements;
    }

    bool empty() const noexcept
    {
        return available() == 0;
    }

    bool full() const noexcept
    {
        return free() == 0;
    }

    reference operator[](size_type index) noexcept
    {
        GHULBUS_PRECONDITION((index >= 0) && (index < m_n_elements));
        return m_ring[nth_index(index)];
    }

    const_reference operator[](size_type index) const noexcept
    {
        GHULBUS_PRECONDITION((index >= 0) && (index < m_n_elements));
        return m_ring[nth_index(index)];
    }

    reference front() noexcept
    {
        return m_ring[front_index()];
    }

    const_reference front() const noexcept
    {
        return m_ring[front_index()];
    }

    reference back() noexcept
    {
        return m_ring[m_push_idx - 1];
    }

    const_reference back() const noexcept
    {
        return m_ring[m_push_idx - 1];
    }

    inline friend bool operator==(FixedRing const& lhs, FixedRing const& rhs) noexcept
    {
        size_type const l_n = lhs.available();
        size_type const r_n = rhs.available();
        if (l_n != r_n) { return false; }
        for (size_type i = 0; i < l_n; ++i) {
            if (lhs.m_ring[lhs.nth_index(i)] != rhs.m_ring[rhs.nth_index(i)]) { return false; }
        }
        return true;
    }

    inline friend bool operator!=(FixedRing const& lhs, FixedRing const& rhs) noexcept
    {
        return !(lhs == rhs);
    }
};

}

#endif
