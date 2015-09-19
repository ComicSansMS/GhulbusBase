#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_EXCEPTION_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_EXCEPTION_HPP

/** @file
*
* @brief Exception Base Class.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <boost/exception/all.hpp>

#include <exception>
#include <string>

namespace GHULBUS_BASE_NAMESPACE
{
    /** Base class for all Ghulbus exceptions.
     * We use Boost.Exception to do the heavy lifting in the exception implementation.
     * Any exception can be decorated with additional info. Instantiate an Info object from the
     * Exception_Info namespace and use `operator<<` to assign it to an exception.
     * All exceptions thrown through \ref GHULBUS_THROW are decorated with Exception_Info::description and the
     * locational decorators from Boost: `boost::throw_function`, `boost::throw_file` and `boost::throw_line`.
     * You can retrieve individual decorators from an exception object with `boost::get_error_info()`.
     * To obtain a textual representation of all the information for an exception,
     * use `boost::diagnostic_information()`.
     *
     */
    class Exception : public virtual std::exception, public virtual boost::exception {
    public:
        virtual ~Exception() {};
        /** Redeclare std::exception::what() as pure virtual.
         * We do this here so that Exception becomes abstract and to force inheriting classes to give
         * the noexcept guarantee.
         */
        virtual char const* what() const noexcept = 0;
    };

    /** Exception decorators.
     * Place all exception decorators in this namespace.
     * A decorator is an instance of the boost::error_info template with a unique (empty) tag type and a record type
     * that provides storage for the information.
     * For non-trivial record-types, provide a std::ostream inserter to allow for nice automatic error messages.
     */
    namespace Exception_Info {
        /** Decorator Tags.
         * Tags are empty types used to uniquely identify a decoratory type.
         */
        namespace Tags
        {
            struct description { };
        }
        /** Decorator record types.
         */
        namespace Records
        {
        }

        /** @name Decorators
         * @{
         */
        /** A user-provided string describing the error.
         */
        typedef boost::error_info<Tags::description, std::string> description;
        /// @}
    }

    /** Concrete exception objects.
     */
    namespace Exceptions
    {
        namespace impl
        {
            /** Mixin class for implementing Ghulbus::Exceptions.
            * This class provides a default implementation for what() that gives a detailed error message.
            */
            class ExceptionImpl : public virtual ::GHULBUS_BASE_NAMESPACE::Exception
            {
                GHULBUS_BASE_API char const* what() const noexcept override;
            };
        }

        /** Thrown by Assert::failThrow in case of a failing exception.
        */
        class AssertFailed : public impl::ExceptionImpl
        {
        };

        /** Thrown by interfaces that have not yet been implemented.
        */
        class NotImplemented : public impl::ExceptionImpl
        {
        };
    }
}

/** Throw a Ghulbus::Exception.
 * All exceptions thrown with this macro will be decorated with the supplied string description and information
 * about the source code location that triggered the throw.
 * @param exc An exception object inheriting from Ghulbus::Exception.
 * @param str A null-terminated C-string to attach to the exception as description.
 */
#define GHULBUS_THROW(exc, str) \
    BOOST_THROW_EXCEPTION(( (exc) << ::GHULBUS_BASE_NAMESPACE::Exception_Info::description(str) ))

#endif
