#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_STORAGE_VIEW_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ALLOCATOR_STORAGE_VIEW_HPP

/** @file
*
* @brief Storage view.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <cstddef>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Allocator
{
/** Non-owning view on a region of memory used by an allocation strategy.
 * This class is used for interop between different storages to allocator strategies.
 * Each allocation strategy accepts any kind of storage as a constructor argument and
 * then attempts to obtain a view on it by calling makeStorageView().
 */
struct StorageView
{
    std::byte* ptr;         ///< Pointer to a region of memory.
    std::size_t size;       ///< Size in bytes of the region pointed to by ptr.
};

/** Build a StorageView from a storage type.
 * An allocation strategy makes an unqualified call to makeStorageView to obtain
 * a view on the storage it is passed in its constructor. When using a custom
 * storage type, an overload of this function can be found through ADL.
 */
template<typename Storage_T>
inline StorageView makeStorageView(Storage_T& storage) noexcept
{
    return StorageView{ storage.get(), storage.size() };
}
}
}
#endif
