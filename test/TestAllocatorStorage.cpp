#include <gbBase/Allocator/Storage.hpp>

#include <catch.hpp>

namespace {
}

TEST_CASE("Static Storage")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    {
        Allocator::Storage::Static<10, 1> storage;
        CHECK(storage.get() == reinterpret_cast<std::byte*>(&storage));
        CHECK(storage.size() == 10);
        CHECK(sizeof(storage) == 10);
    }
    {
        Allocator::Storage::Static<128, 1> storage;
        CHECK(storage.get() == reinterpret_cast<std::byte*>(&storage));
        CHECK(storage.size() == 128);
        CHECK(sizeof(storage) == 128);
    }
    {
        Allocator::Storage::Static<1, alignof(double)> storage;
        CHECK(storage.get() == reinterpret_cast<std::byte*>(&storage));
        CHECK(storage.size() == 1);
        CHECK(sizeof(storage) == sizeof(double));
    }
    {
        Allocator::Storage::Static<sizeof(double), alignof(double)> storage;
        CHECK(storage.get() == reinterpret_cast<std::byte*>(&storage));
        CHECK(storage.size() == sizeof(double));
        CHECK(sizeof(storage) == sizeof(double));
    }
}

TEST_CASE("Dynamic Storage")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    {
        Allocator::Storage::Dynamic storage(128);

        CHECK(storage.get() != nullptr);
        CHECK(storage.size() == 128);
    }
    {
        Allocator::Storage::Dynamic storage(1024*1024);

        CHECK(storage.get() != nullptr);
        CHECK(storage.size() == (1 << 20));
    }
}
