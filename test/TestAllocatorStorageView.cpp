#include <gbBase/Allocator/StorageView.hpp>

#include <catch.hpp>

#include <MockStorage.hpp>


TEST_CASE("Storage View")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    std::byte x;
    testing::MockStorage mock;
    mock.memory_ptr = &x;
    mock.memory_size = 42;

    Allocator::StorageView const storage_view = Allocator::makeStorageView(mock);
    CHECK(storage_view.ptr == &x);
    CHECK(storage_view.size == 42);
}
