#include <gbBase/Assert.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>

namespace GHULBUS_BASE_NAMESPACE
{
namespace
{
std::atomic<Assert::Handler> g_AssertionHandler = &Assert::failAbort;
std::atomic<void*>           g_AssertionHandlerUserParam = nullptr;

std::ostream& operator<<(std::ostream& os, Assert::HandlerParameters const& handler_param)
{
    os << "Assertion failed " << handler_param.line << "@" << handler_param.file
       << " in function " << handler_param.function << ": " << handler_param.condition;
    if (handler_param.message) {
        os << " - " << handler_param.message;
    }
    return os;
}

std::string toString(Assert::HandlerParameters const& handler_param)
{
    std::stringstream sstr;
    sstr << handler_param;
    return sstr.str();
}
}

namespace Assert
{
void setAssertionHandler(Handler handler) noexcept
{
    g_AssertionHandler.store(handler);
}

Handler getAssertionHandler() noexcept
{
    return g_AssertionHandler;
}

void setHandlerParam(void* user_param) noexcept
{
    g_AssertionHandlerUserParam.store(user_param);
}

void* getHandlerParam() noexcept
{
    return g_AssertionHandlerUserParam;
}

void assertionFailed(HandlerParameters const& param)
{
    getAssertionHandler()(param);
}

void failAbort(HandlerParameters const& param)
{
    // keep the error message in a string for easy inspection by the debugger
    std::string const str = toString(param);
    std::cerr << str << std::endl;
    std::abort();
}

void failHalt(HandlerParameters const& param)
{
    // keep the error message in a string for easy inspection by the debugger
    std::string const str = toString(param);
    std::cerr << str << std::endl;
    for(;;) { std::this_thread::sleep_for(std::chrono::hours(1)); }
}

void failThrow(HandlerParameters const& param)
{
    throw 42;
}
}
}
