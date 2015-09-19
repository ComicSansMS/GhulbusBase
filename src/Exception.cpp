#include <gbBase/Exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

namespace GHULBUS_BASE_NAMESPACE
{
namespace Exceptions
{
namespace impl
{
char const* ExceptionImpl::what() const noexcept
{
    return boost::diagnostic_information_what(*this);
}
}
}
}
