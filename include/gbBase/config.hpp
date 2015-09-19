#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_GBBASE_EXPORT_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_GBBASE_EXPORT_HPP

/** @file
 *
 * @brief General configuration for Base.
 * @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
 */

#include <gbBase/gbBase_Export.hpp>

/** Specifies the API for a function declaration.
 * When building as a dynamic library, this is used to mark the functions that will be exported by the library.
 */
#define GHULBUS_BASE_API GHULBUS_LIBRARY_GBBASE_EXPORT

/** Namespace for GhulbusBase library.
 * The implementation internally always uses this macro to refer to the namespace. When building GhulbusBase yourself,
 * you can globally redefine this macro to move to use a different namespace.
 */
#ifndef GHULBUS_BASE_NAMESPACE
#   define GHULBUS_BASE_NAMESPACE Ghulbus
#endif

#endif
