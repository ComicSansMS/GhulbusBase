#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ASSERT_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_ASSERT_HPP

/** @file
*
* @brief Assertions.
* @author Andreas Weis (der_ghulbus@ghulbus-inc.de)
*/

#include <gbBase/config.hpp>

#include <boost/predef/compiler.h>

#if defined GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG && defined GHULBUS_CONFIG_ASSERT_LEVEL_PRODUCTION
#   error Assert level debug and production cannot be set at the same time.
#endif

/** Default-level assert.
 * If the asserted condition evaluates to false at runtime, Ghulbus::Assert::assertionFailed() is called.
 * Each macro has a normal and a `_MESSAGE` form, where the latter takes a user supplied string as an additional
 * argument that will be passed on to the assertion handler.
 * The amount of assertions that get compiled into the code depends on the configured assertion level:
 *  * Default level checks will be compiled unless `GHULBUS_CONFIG_ASSERT_LEVEL_PRODUCTION` is defined.
 *  * Debug level checks will only be compiled if `GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG` is defined.
 *  * Production level checks will always be compiled.
 */
#define GHULBUS_ASSERT(x)     GHULBUS_ASSERT_MESSAGE(x, nullptr)
/** Debug-level assert.
 * @copydetails GHULBUS_ASSERT
 */
#define GHULBUS_ASSERT_DBG(x) GHULBUS_ASSERT_DBG_MESSAGE(x, nullptr)
/** Production-level assert.
 * @copydetails GHULBUS_ASSERT
 */
#define GHULBUS_ASSERT_PRD(x) GHULBUS_ASSERT_PRD_MESSAGE(x, nullptr)

/** Default-level precondition check.
 * @copydetails GHULBUS_ASSERT
 */
#define GHULBUS_PRECONDITION(x)     GHULBUS_PRECONDITION_MESSAGE(x, nullptr)
/** Debug-level precondition check.
 * @copydetails GHULBUS_ASSERT
 */
#define GHULBUS_PRECONDITION_DBG(x) GHULBUS_PRECONDITION_DBG_MESSAGE(x, nullptr)
/** Production-level precondition check.
 * @copydetails GHULBUS_ASSERT
 */
#define GHULBUS_PRECONDITION_PRD(x) GHULBUS_PRECONDITION_PRD_MESSAGE(x, nullptr)

#if defined GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG
#   define GHULBUS_ASSERT_MESSAGE(x, msg)     GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)
#   define GHULBUS_ASSERT_DBG_MESSAGE(x, msg) GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)
#   define GHULBUS_ASSERT_PRD_MESSAGE(x, msg) GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)

#   define GHULBUS_PRECONDITION_MESSAGE(x, msg)     GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg)
#   define GHULBUS_PRECONDITION_DBG_MESSAGE(x, msg) GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg)
#   define GHULBUS_PRECONDITION_PRD_MESSAGE(x, msg) GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg)
#elif defined GHULBUS_CONFIG_ASSERT_LEVEL_PRODUCTION
#   define GHULBUS_ASSERT_MESSAGE(x, msg)
#   define GHULBUS_ASSERT_DBG_MESSAGE(x, msg)
#   define GHULBUS_ASSERT_PRD_MESSAGE(x, msg) GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)

#   define GHULBUS_PRECONDITION_MESSAGE(x, msg)
#   define GHULBUS_PRECONDITION_DBG_MESSAGE(x, msg)
#   define GHULBUS_PRECONDITION_PRD_MESSAGE(x, msg) GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg)
#else
/** Default-level assert with custom message.
 * @copydetails GHULBUS_ASSERT
 */
#   define GHULBUS_ASSERT_MESSAGE(x, msg)     GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)
/** Debug-level assert with custom message.
 * @copydetails GHULBUS_ASSERT
 */
#   define GHULBUS_ASSERT_DBG_MESSAGE(x, msg)
/** Production-level assert with custom message.
 * @copydetails GHULBUS_ASSERT
 */
#   define GHULBUS_ASSERT_PRD_MESSAGE(x, msg) GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)

/** Default-level precondition check with custom message.
 * @copydetails GHULBUS_ASSERT
 */
#   define GHULBUS_PRECONDITION_MESSAGE(x, msg)     GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg)
/** Debug-level precondition check with custom message.
 * @copydetails GHULBUS_ASSERT
 */
#   define GHULBUS_PRECONDITION_DBG_MESSAGE(x, msg)
/** Production-level precondition check with custom message.
 * @copydetails GHULBUS_ASSERT
 */
#   define GHULBUS_PRECONDITION_PRD_MESSAGE(x, msg) GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg)
#endif

/** @cond
 */
#if defined BOOST_COMP_MSVC
#   define GHULBUS_INTERNAL_HELPER_FUNCTION __FUNCSIG__
#elif defined BOOST_COMP_GNUC
#   define GHULBUS_INTERNAL_HELPER_FUNCTION __PRETTY_FUNCTION__
#elif defined BOOST_COMP_CLANG
#   define GHULBUS_INTERNAL_HELPER_FUNCTION __PRETTY_FUNCTION__
#else
#   define GHULBUS_INTERNAL_HELPER_FUNCTION __func__
#endif

#define GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)                                                                         \
    do {                                                                                                             \
        if(!(x)) {                                                                                                   \
            GHULBUS_BASE_NAMESPACE::Assert::assertionFailed(                                                         \
                GHULBUS_BASE_NAMESPACE::Assert::HandlerParameters{                                                   \
                                                            __FILE__,                                                \
                                                            __LINE__,                                                \
                                                            GHULBUS_INTERNAL_HELPER_FUNCTION,                        \
                                                            #x,                                                      \
                                                            msg,                                                     \
                                                            GHULBUS_BASE_NAMESPACE::Assert::getHandlerParam() } );   \
        }                                                                                                            \
    } while(false)

#define GHULBUS_INTERNAL_PRECONDITION_IMPL(x, msg) GHULBUS_INTERNAL_ASSERT_IMPL(x, msg)
/** @endcond
 */

namespace GHULBUS_BASE_NAMESPACE
{
    /** Assertion handling.
     */
    namespace Assert {
        /** Parameter passed to an assertion \ref Handler.
         */
        struct HandlerParameters {
            char const* file;               //!< Name of the source file that triggered the failing assertion.
            long        line;               //!< Line in the source file that triggered the failing assertion.
            char const* function;           //!< Name of the function that contains the failing assertion.
            char const* condition;          //!< Textual representation of the condition that failed the assertion.
            char const* message;            //!< Optional user-provided description. NULL if none provided.
            void*       user_param;         //!< User-provided parameter. The default handlers ignore this, but a user
                                            //!< can use this to pass additional state to a custom assertion handler.
        };

        /** Assertion Handler signature.
         * Functions of this type can be registered with setAssertionHandler() to be invoked by assertionFailed().
         */
        typedef void (*Handler)(HandlerParameters const&);

        /** Determine the behavior in case of failing assertions.
         * The default assertion handler is failAbort().
         * @param[in] handler Function to be invoked by assertionFailed() in case of a failing assertion.
         * @note This function is thread-safe.
         */
        GHULBUS_BASE_API void setAssertionHandler(Handler handler) noexcept;

        /** Retrieve the current assertion handler.
         * @note This function is thread-safe.
         */
        GHULBUS_BASE_API Handler getAssertionHandler() noexcept;

        /** Specify the user parameter to be passed to the assertion handler.
         * The assert macros will set the user_param field of HandlerParameters to this value. The default parameters
         * ignore this parameter, but a user can use this to pass additional state to a custom assertion handler.
         * The initial value for the user parameter is NULL.
         * @note This function is thread-safe.
         */
        GHULBUS_BASE_API void setHandlerParam(void* user_param) noexcept;

        /** Specify the user parameter to be passed to the assertion handler.
        * The assert macros will set the user_param field of HandlerParameters to this value. The default parameters
        * ignore this parameter, but a user can use this to pass additional state to a custom assertion handler.
        * The initial value for the user parameter is NULL.
        * @note This function is thread-safe.
        */
        GHULBUS_BASE_API void* getHandlerParam() noexcept;

        /** Invoke the assertion handler.
         * This is called by the assert macros if an assertion fails. The active assertion handler can be changed
         * with setAssertionHandler().
         * @param[in] param Description of the failing assertion.
         * @note This function is thread-safe.
         */
        GHULBUS_BASE_API void assertionFailed(HandlerParameters const& param);

        /** Assertion handler that calls std::abort().
         * This is the default assertion handler. It writes an error message to cerr and calls abort.
         */
        GHULBUS_BASE_API void failAbort(HandlerParameters const& param);

        /** Assertion handler that halts the program to allow a debugger to attach.
         * This handler prints an error message to cerr and enters an infinite loop.
         * The invoking thread will be put to sleep, so it will not burn CPU time while halted.
         */
        [[noreturn]] GHULBUS_BASE_API void failHalt(HandlerParameters const& param);

        /** Assertion handler that throws an exception.
         * @todo Not implemented.
         */
        GHULBUS_BASE_API void failThrow(HandlerParameters const& param);
    };
}

#endif
