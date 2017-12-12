#include <gbBase/Allocator.hpp>

#include <catch.hpp>

#include <algorithm>
#include <vector>


TEST_CASE("Allocator integration test")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    Allocator::Storage::Dynamic storage(1024);
    Allocator::LinearMemoryResource<Allocator::Storage::Dynamic> resource(storage);
    Allocator::StatefulAllocator<double, decltype(resource)> allocator(resource);

    std::vector<double, decltype(allocator)> my_vector(allocator);
    my_vector.resize(100, 42.5);
    CHECK(resource.getFreeMemory() <= 1024 - (100 * sizeof(double)));
    CHECK(reinterpret_cast<std::byte*>(my_vector.data()) - storage.get() < static_cast<int>(resource.getFreeMemory()));
    CHECK(std::all_of(begin(my_vector), end(my_vector), [](double d) { return d == 42.5; }));
}
