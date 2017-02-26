#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_FINALLY_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_FINALLY_HPP

/** @file
*
* @brief Scope finalizers.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <type_traits>

namespace GHULBUS_BASE_NAMESPACE
{

/** Helper class for finally().
 * The sole purpose of the Finalizer is to invoke a function upon destruction.
 * Finalizers are move-only types.
 */
template<class F>
class Finalizer
{
    static_assert(std::is_nothrow_move_constructible<F>::value, "Callable type must be nothrow-move-constructible.");
public:
    explicit Finalizer(F f) noexcept : m_finalizer(std::move(f)), m_invoke(true) {}

    Finalizer(Finalizer&& rhs) noexcept : m_finalizer(std::move(rhs.m_finalizer)), m_invoke(rhs.m_invoke) { rhs.m_invoke = false; }

    ~Finalizer() noexcept { if(m_invoke) { std::invoke(m_finalizer); } }

    Finalizer(Finalizer const&) = delete;
    Finalizer& operator=(Finalizer const&) = delete;

    /** Defusing a Finalizer will prevent it from invoking the finalize function upon destruction.
     */
    void defuse() noexcept { m_invoke = false; }
private:
    F m_finalizer;
    bool m_invoke;
};

/** Constructs a Finalizer to invoke code at the end of scope.
 * @param[in] finalizer A Callable object. The returned Finalizer will invoke this upon destruction.
 */
template<class F>
inline Finalizer<F> finally(F&& finalizer) noexcept
{
    return Finalizer<F>(std::forward<F>(finalizer));
}
}

#endif
