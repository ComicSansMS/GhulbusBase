#ifndef INCLUDE_GUARD_GHULBUS_LIBRARY_BASE_OVERLOAD_SET_HPP
#define INCLUDE_GUARD_GHULBUS_LIBRARY_BASE_OVERLOAD_SET_HPP

/** @file
 *
 * @brief A helper type for wrapping a number of invocables into a single overload set.
 * @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
 */

#include <gbBase/config.hpp>

namespace GHULBUS_BASE_NAMESPACE
{

template<class... Ts> struct OverloadSet : Ts... { using Ts::operator()...; };
template<class... Ts> OverloadSet(Ts...) -> OverloadSet<Ts...>;

}

#endif
