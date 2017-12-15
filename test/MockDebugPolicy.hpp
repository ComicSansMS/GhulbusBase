#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_TEST_MOCK_DEBUG_POLICY_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_TEST_MOCK_DEBUG_POLICY_HPP

#include <gbBase/config.hpp>

namespace GHULBUS_BASE_NAMESPACE
{
namespace testing
{
struct MockDebugPolicy {
    static inline std::size_t number_on_allocate_calls = 0;
    static inline std::size_t number_on_deallocate_calls = 0;
    static inline std::size_t number_on_reset_calls = 0;

    void onAllocate(std::size_t, std::size_t, std::byte*) { ++number_on_allocate_calls; }
    void onDeallocate(std::byte*, std::size_t) { ++number_on_deallocate_calls; }
    void onReset() { ++number_on_reset_calls; }
};
}
}
#endif
