#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ANY_INVOCABLE_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ANY_INVOCABLE_HPP

/** @file
*
* @brief An owning type erased wrapper for callable objects.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <functional>
#include <memory>
#include <type_traits>

namespace GHULBUS_BASE_NAMESPACE
{

template<typename Signature>
class AnyInvocable;

template<typename T_Return, typename... T_Args>
class AnyInvocable<T_Return(T_Args...)> {
private:
    class Concept {
    public:
        virtual ~Concept() = default;
        virtual T_Return invoke(T_Args... args) const = 0;
    };

    template<typename F>
    class Model : public Concept {
        mutable F f;
    public:
        Model(F&& nf)
            :f(std::move(nf))
        {}

        ~Model() override = default;

        T_Return invoke(T_Args... args) const override {
            return std::invoke(f, std::forward<T_Args>(args)...);
        }
    };
public:
    template<typename Func>
    AnyInvocable(Func f)
        :m_ptr(std::make_unique<Model<Func>>(std::move(f)))
    {}

    T_Return operator()(T_Args... args) const
    {
        return m_ptr->invoke(std::forward<T_Args>(args)...);
    }

private:
    std::unique_ptr<Concept> m_ptr;
};

}

#endif