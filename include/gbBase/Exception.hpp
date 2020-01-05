#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_EXCEPTION_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_EXCEPTION_HPP

/** @file
*
* @brief Exception Base Class.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/
#include <gbBase/config.hpp>

#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <typeinfo>
#include <type_traits>

namespace GHULBUS_BASE_NAMESPACE
{
    /** Decorator type.
     * An exception type derived from `Exception` may have an arbitrary number of decorator types attached to it
     * using `operator<<`. The data attached this way may then be retrieved from the exception object by
     * calling `getErrorInfo()`.
     */
    template<typename Tag_T, typename T>
    class GHULBUS_BASE_API ErrorInfo {
    public:
        using ValueType = T;
        using TagType = Tag_T;
    private:
        T m_data;
    public:
        template<typename... Args>
        ErrorInfo(Args&&... args)
            :m_data(std::forward<Args>(args)...)
        {}

        T const& getData() const & {
            return m_data;
        }

        T&& getData() && {
            return std::move(m_data);
        }
    };

    /** Trait for detecting instantiations of `ErrorInfo`.
     */
    template<typename T>
    struct IsErrorInfo : public std::false_type {};
    template<typename Tag_T, typename T>
    struct IsErrorInfo<ErrorInfo<Tag_T, T>> : public std::true_type {};

    namespace impl {
    /** @cond
     */
    /** Helper functor for converting arbitrary types to string for printing.
     * - if `T` is std::string acts as the identity function
     * - if `T` has an overload of `to_string` found through an unqualified call, uses `to_string`
     * - if `T` has an ostream inserter defined, uses `operator<<` to insert to a `std::stringstream`
     * - otherwise use `typeid(T).name()`.
     */
    template<typename T, typename = void>
    struct toStringHelper;

    using std::to_string;

    template<typename T, typename = void>
    struct hasToString : public std::false_type {};
    template<typename T>
    struct hasToString<T, std::void_t<decltype(to_string(std::declval<T>()))>> : public std::true_type {};

    template<typename T, typename = void>
    struct hasOstreamInserter : public std::false_type {};
    template<typename T>
    struct hasOstreamInserter<T, std::void_t<decltype(std::declval<std::stringstream>() << std::declval<T>())>> : public std::true_type {};

    template<>
    struct toStringHelper<std::string> {
        std::string const& operator()(std::string const& s) {
            return s;
        }
    };

    template<typename T>
    struct toStringHelper<T, std::enable_if_t<!std::is_same_v<T, std::string> && hasToString<T>::value>> {
        std::string operator()(T const& v) {
            return to_string(v);
        }
    };

    template<typename T>
    struct toStringHelper<T, std::enable_if_t<!std::is_same_v<T, std::string> &&
                                              !hasToString<T>::value &&
                                              hasOstreamInserter<T>::value>>
    {
        std::string operator()(T const& v) {
            std::stringstream sstr;
            sstr << v;
            return sstr.str();
        }
    };

    template<typename T>
    struct toStringHelper<T, std::enable_if_t<!std::is_same_v<T, std::string> &&
                                              !hasToString<T>::value &&
                                              !hasOstreamInserter<T>::value>>
    {
        std::string operator()(T const& v) {
            return typeid(T).name();
        }
    };
    /** @endcond
    */
    }

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
            struct GHULBUS_BASE_API location { };
            struct GHULBUS_BASE_API description { };
            struct GHULBUS_BASE_API filename { };
        }
        /** Decorator record types.
         */
        namespace Records
        {
            struct location {
                char const* file;
                char const* function;
                long line;

                location(char const* nfile, char const* nfunc, long nline)
                    :file(nfile), function(nfunc), line(nline)
                {}
            };
        }

        /** @name Decorators
         * @{
         */
        /** The source code location where the exception originated.
         */
        using location = ErrorInfo<Tags::location, Records::location>;
        /** A user-provided string describing the error.
         */
        using description = ErrorInfo<Tags::description, std::string>;
        /** A filename for errors occurring in the context of a file operation.
         */
        using filename = ErrorInfo<Tags::filename, std::string>;
        /// @}
    }

    /** Base class for all Ghulbus exceptions.
     * Any exception can be decorated with additional info. Instantiate an Info object from the
     * Exception_Info namespace and use `operator<<` to assign it to an exception.
     * All exceptions thrown through \ref GHULBUS_THROW are decorated with Exception_Info::description and a
     * decorator for the location of the throw site of the exception, see `Exception_Info::Records::location`.
     * Individual decorators can be retrieved from an exception object with `getErrorInfo()`.
     * To obtain a textual representation of all the information for an exception,
     * use `getDiagnosticMessage`.
     *
     */
    class GHULBUS_BASE_API Exception : public virtual std::exception {
    private:
        /** Type-erased storage for ErrorInfo (base class).
         */
        class ErrorInfoConcept {
        private:
            std::unique_ptr<ErrorInfoConcept> m_next;
        public:
            ErrorInfoConcept() = default;

            explicit ErrorInfoConcept(std::unique_ptr<ErrorInfoConcept>&& next)
                :m_next(std::move(next))
            {}

            virtual ~ErrorInfoConcept() noexcept = default;
            virtual std::unique_ptr<ErrorInfoConcept> clone() const = 0;
            virtual std::type_info const& getTypeInfo() const = 0;
            virtual std::string dataString() const = 0;

            ErrorInfoConcept const* next() const {
                return m_next.get();
            }

            ErrorInfoConcept* next() {
                return m_next.get();
            }
        };

        /** Type-erased storage for ErrorInfo.
        */
        template<typename ErrorInfo_T>
        class ErrorInfoModel : public ErrorInfoConcept {
        public:
            static_assert(IsErrorInfo<ErrorInfo_T>::value,
                          "Only ErrorInfo types can be used to decorate Exception.");
            using ValueType = typename ErrorInfo_T::ValueType;
            using TagType = typename ErrorInfo_T::TagType;
        private:
            ValueType m_data;
        public:
            ErrorInfoModel(std::unique_ptr<ErrorInfoConcept>&& next, ErrorInfo_T data)
                :ErrorInfoConcept(std::move(next)), m_data(std::move(data).getData())
            {}

            ~ErrorInfoModel() noexcept override = default;

            std::unique_ptr<ErrorInfoConcept> clone() const override {
                ErrorInfoConcept const* next_ei = next();
                return std::make_unique<ErrorInfoModel>((next_ei == nullptr) ? nullptr : next_ei->clone(), m_data);
            }

            std::type_info const& getTypeInfo() const override {
                return typeid(TagType);
            }

            ValueType const& data() const {
                return m_data;
            }

            std::string dataString() const override {
                return impl::toStringHelper<decltype(data())>{}(data());
            }
        };

        std::unique_ptr<ErrorInfoConcept> mutable m_errorInfos;
        mutable Exception_Info::Records::location m_location = Exception_Info::Records::location(nullptr, nullptr, -1);
        mutable std::string m_description;
        mutable std::string m_diagnosticMessageCached;
    protected:
        Exception() = default;
        virtual ~Exception() = default;

        Exception(Exception&&) noexcept = default;
        Exception& operator=(Exception&&) noexcept  = default;

        Exception(Exception const& rhs)
            :m_errorInfos(rhs.m_errorInfos ? rhs.m_errorInfos->clone() : nullptr),
             m_location(rhs.m_location), m_description(rhs.m_description)
        {}

        Exception& operator=(Exception const& rhs) {
            if (&rhs != this) {
                m_errorInfos = rhs.m_errorInfos->clone();
                m_location = rhs.m_location;
                m_description = rhs.m_description;
            }
            return *this;
        }

    public:
        /** Redeclare std::exception::what() as pure virtual.
         * We do this here so that Exception becomes abstract and to force inheriting classes to give
         * the noexcept guarantee.
         */
        virtual char const* what() const noexcept = 0;

    private:
        template<typename ErrorInfo_T>
        std::enable_if_t<IsErrorInfo<std::decay_t<ErrorInfo_T>>::value, void>
        addErrorInfo(ErrorInfo_T&& error_info) const {
            m_errorInfos = std::make_unique<
                ErrorInfoModel<std::decay_t<ErrorInfo_T>>>(std::move(m_errorInfos),
                                                           std::forward<ErrorInfo_T>(error_info));
        }

        void setExceptionLocation(Exception_Info::Records::location const& location) const {
            m_location = location;
        }

        Exception_Info::Records::location const& getLocation() const {
            return m_location;
        }

        void setDescription(std::string description) const {
            m_description = std::move(description);
        }

        std::string const& getDescription() const {
            return m_description;
        }

        template<typename ErrorInfo_T>
        typename ErrorInfo_T::ValueType const* getErrorInfo() const {
            using TagType = typename ErrorInfo_T::TagType;
            for (ErrorInfoConcept const* it = m_errorInfos.get(); it != nullptr; it = it->next()) {
                if (it->getTypeInfo() == typeid(TagType)) {
                    return &dynamic_cast<ErrorInfoModel<ErrorInfo_T> const&>(*it).data();
                }
            }
            return nullptr;
        }

        char const* getDiagnosticMessage() const {
            /*
            <file>(<line>): Throw in function <func>
            Dynamic exception type: bla
            [<TagType #1>] = <data #1>
             ...
            [<TagType #n>] = <data #n>
            */
            m_diagnosticMessageCached =
                std::string((m_location.file) ? m_location.file : "<unknown file>") +
                '(' + std::to_string(m_location.line) + "): Throw in function " +
                (m_location.function ? m_location.function : "<unknown function>") +
                "\nDynamic exception type: " + typeid(*this).name() +
                "\n" + m_description;
            for (ErrorInfoConcept const* it = m_errorInfos.get(); it != nullptr; it = it->next()) {
                m_diagnosticMessageCached += std::string("\n[") + it->getTypeInfo().name() + "] = " +
                                             it->dataString();
            }
            return m_diagnosticMessageCached.c_str();
        }

        friend char const* getDiagnosticMessage(Exception const& e);
        template<typename ErrorInfo_T, typename Exception_T>
        friend typename ErrorInfo_T::ValueType const* getErrorInfo(Exception_T const& e);
        template<typename Exception_T, typename ErrorInfo_T>
        friend std::enable_if_t<IsErrorInfo<std::decay_t<ErrorInfo_T>>::value &&
                                std::is_base_of_v<Exception, Exception_T>, Exception_T> const&
        operator<<(Exception_T const& e, ErrorInfo_T&& error_info);
    };


    /** Decorate an exception with an ErrorInfo.
     */
    template<typename Exception_T, typename ErrorInfo_T>
    inline std::enable_if_t<IsErrorInfo<std::decay_t<ErrorInfo_T>>::value &&
                            std::is_base_of_v<Exception, Exception_T>, Exception_T> const&
    operator<<(Exception_T const& e, ErrorInfo_T&& error_info) {
        if constexpr (std::is_same_v<std::decay_t<ErrorInfo_T>, Exception_Info::location>) {
            e.setExceptionLocation(error_info.getData());
        } else if constexpr (std::is_same_v<std::decay_t<ErrorInfo_T>, Exception_Info::description>) {
            e.setDescription(std::forward<ErrorInfo_T>(error_info).getData());
        } else {
            e.addErrorInfo(std::forward<ErrorInfo_T>(error_info));
        }
        return e;
    }

    /** Attempts to retrieve the error decoration of type `ErrorInfo_T` from the exception `e`.
     * @return A pointer to the decorator record if it could be retrieved; `nullptr` otherwise.
     *         Retrieval may fail if `Exception_T` is not a class derived from `Exception` or
     *         if the exception does not contain a decorator of the requested type.
     */
    template<typename ErrorInfo_T, typename Exception_T>
    inline typename ErrorInfo_T::ValueType const* getErrorInfo(Exception_T const& e) {
        if (Exception const* exc = dynamic_cast<Exception const*>(&e); exc != nullptr) {
            if constexpr (std::is_same_v<std::decay_t<ErrorInfo_T>, Exception_Info::location>) {
                return &(exc->getLocation());
            } else if constexpr (std::is_same_v<std::decay_t<ErrorInfo_T>, Exception_Info::description>) {
                return &(exc->getDescription());
            } else {
                return exc->getErrorInfo<ErrorInfo_T>();
            }
        }
        return nullptr;
    }

    /** Helper function for retrieving a diagnostic information string about the exception.
     */
    inline char const* getDiagnosticMessage(Exception const& e) {
        return e.getDiagnosticMessage();
    }

    /** Helper function applying a variadic number of decorators to an exception.
     * @param e An exception object.
     * @param args The decorators that will be applied to the exception object e.
     * @return A reference to the exception object e.
     */
    template<typename Exception_T, typename... ExceptionInfo_Ts>
    inline auto decorate_exception(Exception_T const& e, ExceptionInfo_Ts const&... args)
    {
        return (e << ... << args);
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
            class GHULBUS_BASE_API ExceptionImpl : public virtual ::GHULBUS_BASE_NAMESPACE::Exception
            {
            public:
                char const* what() const noexcept override {
                    return ::GHULBUS_BASE_NAMESPACE::getDiagnosticMessage(*this);
                }
            };
        }

        /** Thrown by Assert::failThrow in case of a failing exception.
         */
        class GHULBUS_BASE_API AssertFailed : public impl::ExceptionImpl
        {
        };

        /** Thrown by interfaces that have not yet been implemented.
         */
        class GHULBUS_BASE_API NotImplemented : public impl::ExceptionImpl
        {
        };

        /** Thrown when an I/O operation fails.
         */
        class GHULBUS_BASE_API IOError : public impl::ExceptionImpl
        {
        };

        /** Thrown when an invalid argument was passed to a function.
         * This is used if an argument does not violate a function's precondition but is nonetheless invalid in the
         * given context. Precondition violations are undefined behavior and may only be signaled using GHULBUS_ASSERT.
         */
        class GHULBUS_BASE_API InvalidArgument : public impl::ExceptionImpl
        {
        };

        /** Thrown when a function call violates protocol.
         * This is used if a function call does not violate a function's precondition but is nonetheless invalid in the
         * given context. Precondition violations are undefined behavior and may only be signaled using GHULBUS_ASSERT.
         * @note Use this exception with care. Most of the time, a protocol violation is a precondition violation.
         */
        class GHULBUS_BASE_API ProtocolViolation : public impl::ExceptionImpl
        {
        };
    }
}

/** @cond
 */
#if defined _MSC_VER
#   define GHULBUS_INTERNAL_HELPER_FUNCTION_2 __FUNCSIG__
#elif defined __clang__
#   define GHULBUS_INTERNAL_HELPER_FUNCTION_2 __PRETTY_FUNCTION__
#elif defined __GNUC__
#   define GHULBUS_INTERNAL_HELPER_FUNCTION_2 __PRETTY_FUNCTION__
#else
#   define GHULBUS_INTERNAL_HELPER_FUNCTION_2 __func__
#endif
/** @endcond
 */

/** Throw a Ghulbus::Exception.
 * All exceptions thrown with this macro will be decorated with the supplied string description and information
 * about the source code location that triggered the throw.
 * @param exc An exception object inheriting from Ghulbus::Exception.
 * @param str A std::string or null-terminated C-string to attach to the exception as description.
 */
#define GHULBUS_THROW(exc, str) \
    throw ( (exc) <<                                                                            \
        ::GHULBUS_BASE_NAMESPACE::Exception_Info::location(__FILE__,                            \
                                                           GHULBUS_INTERNAL_HELPER_FUNCTION_2,  \
                                                           __LINE__) <<                         \
        ::GHULBUS_BASE_NAMESPACE::Exception_Info::description(str) )

#endif
