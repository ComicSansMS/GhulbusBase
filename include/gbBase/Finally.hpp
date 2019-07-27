#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_FINALLY_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_FINALLY_HPP

/** @file
*
* @brief Scope finalizers.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace GHULBUS_BASE_NAMESPACE
{

/** Helper class for finally().
 * The sole purpose of the Finalizer is to invoke a function upon destruction.
 * Finalizers are move-only types.
 */
template<class F>
class Finalizer
{
public:
    explicit Finalizer(F f) noexcept
        : m_finalizer(std::move(f)), m_invoke(true) {}

    Finalizer(Finalizer&& rhs) noexcept
        : m_finalizer(std::move(rhs.m_finalizer)), m_invoke(rhs.m_invoke) { rhs.m_invoke = false; }

    ~Finalizer() noexcept { if(m_invoke) { m_finalizer(); } }

    Finalizer(Finalizer const&) = delete;
    Finalizer& operator=(Finalizer const&) = delete;
    /* Even though move assignment could technically be implemented here, it does not make sense
     * for the intended use cases of finally(), so we delete it as well.
     */
    Finalizer& operator=(Finalizer&&) = delete;

    /** Defusing a Finalizer will prevent it from invoking the finalize function upon destruction.
     */
    void defuse() noexcept { m_invoke = false; }
private:
    F m_finalizer;
    bool m_invoke;
};

/** Type-erased wrapper for Finalizer types.
 */
class AnyFinalizer {
public:
    /** Constructs an empty wrapper.
     */
    AnyFinalizer() noexcept = default;

    /** Constructs a wrapper containing Finalizer f.
     */
    template<class F>
    AnyFinalizer(Finalizer<F>&& f)
        :m_finalizer(std::make_unique<Model<F>>(std::move(f)))
    {}

    ~AnyFinalizer() noexcept = default;

    AnyFinalizer(AnyFinalizer&& rhs) = default;
    AnyFinalizer& operator=(AnyFinalizer&& rhs) = default;

    AnyFinalizer(AnyFinalizer const&) = delete;
    AnyFinalizer& operator=(AnyFinalizer const&) = delete;

    /** Checks if the wrapper is currently empty.
     */
    operator bool() const noexcept {
        return m_finalizer.operator bool();
    }

    /** Invokes defuse on the contained Finalizer.
     * \pre The wrapper must not be empty.
     */
    void defuse() noexcept
    {
        m_finalizer->defuse();
    }
private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void defuse() noexcept = 0;
    };

    template<class F>
    struct Model : Concept {
        Finalizer<F> finalizer_;

        Model(Finalizer<F>&& f)
            :finalizer_(std::move(f))
        {}

        ~Model() override = default;

        void defuse() noexcept override {
            finalizer_.defuse();
        }
    };

    std::unique_ptr<Concept> m_finalizer;
};

/** Constructs a Finalizer to invoke code at the end of scope.
 * @param[in] finalizer A Callable object. The returned Finalizer will invoke this upon destruction.
 */
template<class F>
inline auto finally(F&& finalizer) noexcept
{
    using RemRef = typename std::remove_reference<F>::type;
    using RetParam = typename std::conditional<
        std::is_lvalue_reference<F>::value && !std::is_function<RemRef>::value,
        RemRef, F>::type;
    return Finalizer<RetParam>(std::forward<F>(finalizer));
}
}

#endif
