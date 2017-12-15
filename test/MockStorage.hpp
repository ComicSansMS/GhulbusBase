#ifndef GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_TEST_MOCK_STORAGE_HPP
#define GHULBUS_LIBRARY_INCLUDE_GUARD_BASE_TEST_MOCK_STORAGE_HPP

#include <gbBase/config.hpp>

namespace GHULBUS_BASE_NAMESPACE
{
namespace testing
{
struct MockStorage {
    std::byte* memory_ptr = nullptr;
    std::size_t memory_size = 0;

    std::byte* get() noexcept {
        return memory_ptr;
    }

    std::size_t size() const noexcept {
        return memory_size;
    }
};
}
}
#endif
