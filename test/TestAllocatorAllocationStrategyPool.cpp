#include <gbBase/Allocator/AllocationStrategyPool.hpp>

#include <gbBase/Allocator/Storage.hpp>

#include <MockDebugPolicy.hpp>
#include <MockStorage.hpp>

#include <catch.hpp>

#include <type_traits>

TEST_CASE("Pool Allocation Strategy")
{
    using namespace GHULBUS_BASE_NAMESPACE;

    using testing::MockDebugPolicy;
    using testing::MockStorage;

    MockStorage storage;

    using DebugPol = Allocator::DebugPolicy::CombinedPolicy<Allocator::DebugPolicy::DebugHeap, MockDebugPolicy>;
    using Alloc = Allocator::AllocationStrategy::Pool<DebugPol>;
    using Header = Alloc::Header;
    MockDebugPolicy::number_on_allocate_calls = 0;
    MockDebugPolicy::number_on_deallocate_calls = 0;
    MockDebugPolicy::number_on_reset_calls = 0;


    SECTION("Header")
    {
        Header h1(nullptr);
        CHECK(h1.isFree());
        CHECK(h1.nextFreeHeader() == nullptr);

        Header h2(&h1);
        CHECK(h2.isFree());
        CHECK(h2.nextFreeHeader() == &h1);

        h2.markOccupied();
        CHECK(!h2.isFree());

        Header h3(nullptr);
        h2.setNextFreeHeader(&h3);
        CHECK(h2.isFree());
        CHECK(h2.nextFreeHeader() == &h3);
    }

    SECTION("Construction")
    {
        std::size_t constexpr chunk_size = 1024;
        std::size_t constexpr storage_size = Alloc::calculateStorageSize(chunk_size, 10);
        std::aligned_storage_t<storage_size, alignof(Header)> s;
        storage.memory_ptr = reinterpret_cast<std::byte*>(&s);
        storage.memory_size = storage_size;

        Alloc pool(storage, 1024);
        CHECK(pool.getChunkSize() == chunk_size);
        CHECK(pool.getNumberOfFreeChunks() == 10);

        Header* it_header = reinterpret_cast<Header*>(storage.memory_ptr);

        int count = 0;
        for(; it_header; it_header = it_header->nextFreeHeader(), ++count) {
            CHECK(it_header->isFree());
            CHECK(reinterpret_cast<std::byte*>(it_header) - storage.memory_ptr == count*(chunk_size + sizeof(Header)));
        }
        CHECK(count == 10);
    }

    SECTION("Allocation & Deallocation")
    {
        std::size_t constexpr chunk_size = 1024;
        std::size_t constexpr storage_size = Alloc::calculateStorageSize(chunk_size, 10);
        std::aligned_storage_t<storage_size, alignof(Header)> s;
        std::byte* const ptr = reinterpret_cast<std::byte*>(&s);
        storage.memory_ptr = ptr;
        storage.memory_size = storage_size;

        Alloc pool(storage, 1024);

        CHECK(MockDebugPolicy::number_on_allocate_calls == 0);
        CHECK(pool.getNumberOfFreeChunks() == 10);
        auto const p1 = pool.allocate(120, 1);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 1);
        CHECK(pool.getNumberOfFreeChunks() == 9);
        CHECK(p1 == ptr + sizeof(Header));

        auto const p2 = pool.allocate(1024, 1);
        CHECK(MockDebugPolicy::number_on_allocate_calls == 2);
        CHECK(pool.getNumberOfFreeChunks() == 8);
        CHECK(p2 == ptr + 1024 + 2*sizeof(Header));

        auto const p3 = pool.allocate(0, 1);
        CHECK(p3 == ptr + 2*1024 + 3*sizeof(Header));

        auto const p4 = pool.allocate(55, 1);
        CHECK(p4 == ptr + 3*1024 + 4*sizeof(Header));

        auto const p5 = pool.allocate(512, 1);
        CHECK(p5 == ptr + 4*1024 + 5*sizeof(Header));

        // cannot request blocks bigger than chunk size
        CHECK_THROWS_AS(pool.allocate(1025, 1), std::bad_alloc);

        auto const p6 = pool.allocate(512, 16);
        CHECK(p6 == ptr + 5*1024 + 6*sizeof(Header));

        // maximum chunk size may shrink due to alignment
        CHECK_THROWS_AS(pool.allocate(1017, 16), std::bad_alloc);
        // aligned pointers have additional offset
        auto const p7 = pool.allocate(1016, 16);
        CHECK(p7 == ptr + 6*1024 + 7*sizeof(Header) + alignof(Header));

        auto const p8 = pool.allocate(1, 1);
        CHECK(p8 == ptr + 7*1024 + 8*sizeof(Header));

        auto const p9 = pool.allocate(1, 1);
        CHECK(p9 == ptr + 8*1024 + 9*sizeof(Header));

        auto const p10 = pool.allocate(1024, 1);
        CHECK(p10 == ptr + 9*1024 + 10*sizeof(Header));

        // no more chunks left
        CHECK(pool.getNumberOfFreeChunks() == 0);
        CHECK_THROWS_AS(pool.allocate(0, 1), std::bad_alloc);

        // deallocating chunks frees recycles them
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 0);
        pool.deallocate(p2, 1024);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 1);
        CHECK(pool.getNumberOfFreeChunks() == 1);
        auto const p11 = pool.allocate(1024, 1);
        CHECK(p11 == p2);

        // chunks are recycled in a fifo manner
        pool.deallocate(p4, 55);
        pool.deallocate(p5, 512);
        pool.deallocate(p3, 0);
        pool.deallocate(p9, 1);
        CHECK(MockDebugPolicy::number_on_deallocate_calls == 5);
        CHECK(pool.getNumberOfFreeChunks() == 4);

        auto const p12 = pool.allocate(1024, 1);
        CHECK(p12 == p9);
        auto const p13 = pool.allocate(1024, 1);
        CHECK(p13 == p3);

        // recycling aligned memory gets rid of the alignment due to padding
        pool.deallocate(p7, 1016);
        auto const p14 = pool.allocate(1024, 1);
        CHECK(p14 == p7 - alignof(Header));

        pool.deallocate(p1, 120);
        pool.deallocate(p6, 512);
        pool.deallocate(p8, 1);
        pool.deallocate(p10, 1024);
        pool.deallocate(p11, 1024);
        pool.deallocate(p12, 1024);
        pool.deallocate(p13, 1024);
        pool.deallocate(p14, 1024);
    }

    SECTION("Reset")
    {
        std::size_t constexpr chunk_size = 1024;
        std::size_t constexpr storage_size = Alloc::calculateStorageSize(chunk_size, 10);
        std::aligned_storage_t<storage_size, alignof(Header)> s;
        std::byte* const ptr = reinterpret_cast<std::byte*>(&s);
        storage.memory_ptr = ptr;
        storage.memory_size = storage_size;

        Alloc pool(storage, 1024);

        std::byte* ps[10];
        for(int i=0; i<10; ++i) {
            ps[i] = pool.allocate(1024, 8);
        }
        for(int i=0; i<10; ++i) {
            pool.deallocate(ps[i], 1024);
        }
        CHECK(pool.getNumberOfFreeChunks() == 10);

        // free list is reversed, pointers are now handed out back-to-front:
        auto const p10 = pool.allocate(1024, 8);
        CHECK(p10 == ps[9]);
        auto const p11 = pool.allocate(1024, 8);
        CHECK(p11 == ps[8]);
        CHECK(p11 < p10);
        pool.deallocate(p11, 1024);
        pool.deallocate(p10, 1024);

        // reset restores original order:
        CHECK(MockDebugPolicy::number_on_reset_calls == 0);
        pool.reset();
        CHECK(MockDebugPolicy::number_on_reset_calls == 1);
        for(int i=0; i<10; ++i) {
            CHECK(pool.allocate(1024, 8) == ps[i]);
        }
        for(int i=0; i<10; ++i) {
            pool.deallocate(ps[i], 1024);
        }
    }

    SECTION("Storage alignment")
    {
        std::size_t constexpr chunk_size = 1024;
        std::size_t constexpr storage_size = Alloc::calculateStorageSize(chunk_size, 10);
        std::aligned_storage_t<storage_size, alignof(Header)> s;
        std::byte* const ptr = reinterpret_cast<std::byte*>(&s);
        storage.memory_ptr = ptr + 1;
        storage.memory_size = storage_size;

        {
            Alloc pool(storage, 1024);
            CHECK(pool.getNumberOfFreeChunks() == 9);
            auto const p1 = pool.allocate(1024, 1);
            CHECK(p1 == ptr + alignof(Header) + sizeof(Header));
            pool.deallocate(p1, 1024);
        }

        {
            storage.memory_size = 1024;
            Alloc pool(storage, 1024 - sizeof(Header));
            CHECK(pool.getNumberOfFreeChunks() == 0);
        }
        {
            storage.memory_ptr = ptr;
            Alloc pool(storage, 1024 - sizeof(Header));
            CHECK(pool.getNumberOfFreeChunks() == 1);
            auto const p1 = pool.allocate(1024 - sizeof(Header), 1);
            CHECK(p1 == ptr + sizeof(Header));
            pool.deallocate(p1, 1024 - sizeof(Header));
        }

        {
            storage.memory_ptr = ptr + 1;
            storage.memory_size = sizeof(Header) + alignof(Header) - 2;
            CHECK_THROWS_AS(Alloc(storage, 1), std::bad_alloc);
            storage.memory_size = sizeof(Header) + alignof(Header) - 1;
            {
                Alloc pool(storage, 1);
                CHECK(pool.getNumberOfFreeChunks() == 0);
            }
            storage.memory_size = sizeof(Header) + alignof(Header);
            {
                Alloc pool(storage, 1);
                CHECK(pool.getNumberOfFreeChunks() == 1);
                auto const p1 = pool.allocate(1, 1);
                CHECK_THROWS_AS(pool.allocate(1, 1), std::bad_alloc);
                pool.deallocate(p1, 1);
            }
        }
    }
}
