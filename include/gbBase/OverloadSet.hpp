#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_OVERLOAD_SET_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_OVERLOAD_SET_HPP

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
