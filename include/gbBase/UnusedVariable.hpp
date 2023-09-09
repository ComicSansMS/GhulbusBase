#ifndef INCLUDE_GUARD_GHULBUS_LIBRARY_BASE_UNUSED_VARIABLE_HPP
#define INCLUDE_GUARD_GHULBUS_LIBRARY_BASE_UNUSED_VARIABLE_HPP

/** @file
 *
 * @brief Macro for marking unused variables.
 * @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
 */

/** Use this to mark unused variables in code.
 * This should suppress any unused variable warnings from the compiler.
 */
#define GHULBUS_UNUSED_VARIABLE(x) (void)x

#endif
