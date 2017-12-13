#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_STORAGE_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_STORAGE_HPP

/** @file
*
* @brief Storage for memory resources.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Allocator
{
namespace Storage
{
/** Static storage.
 * Provides room for `N` bytes of memory with an alignment of `Align`.
 * The storage lives inside the class, so `sizeof(Static<N>)` increases with growing `N`.
 * @tparam N Size of the static storage in bytes.
 * @tparam Align Memory alignment of the storage. Defaults to `alignof(std::max_align_t)`.
 */
template<std::size_t N, std::size_t Align = alignof(std::max_align_t)>
class Static {
private:
    std::aligned_storage_t<N, Align> m_storage;
public:
    constexpr std::byte* get() noexcept {
        return reinterpret_cast<std::byte*>(&m_storage);
    }

    constexpr std::size_t size() const noexcept {
        return N;
    }
};

/** Dynamic storage.
 * Allocates a storage of a certain size dynamically from the heap (using global `new`).
 */
class Dynamic {
private:
    std::unique_ptr<std::byte[]> m_storage;
    std::size_t m_size;
public:
    Dynamic(std::size_t n)
        :m_storage(std::make_unique<std::byte[]>(n)), m_size(n)
    {}

    std::byte* get() noexcept {
        return m_storage.get();
    }

    std::size_t size() const noexcept {
        return m_size;
    }
};
}
}
}
#endif
