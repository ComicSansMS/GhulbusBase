#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_GBBASE_CONFIG_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_GBBASE_CONFIG_HPP

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

/** \namespace Ghulbus Base namespace for the Ghulbus library.
 * The implementation internally always uses this macro `GHULBUS_BASE_NAMESPACE` to refer to the namespace.
 * When building GhulbusBase yourself, you can globally redefine this macro to move to a different namespace.
 */
#ifndef GHULBUS_BASE_NAMESPACE
#   define GHULBUS_BASE_NAMESPACE Ghulbus
#endif

#endif
