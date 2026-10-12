/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/ring_allocator.hpp>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <random>
#include <thread>
#include <vector>

namespace q = cycfi::q;

namespace
{
   bool aligned(void* p)
   {
      return reinterpret_cast<std::uintptr_t>(p)
         % alignof(std::max_align_t) == 0;
   }

   // Fill a block with a pattern made from its id, and check it back.
   void fill(void* p, std::size_t size, std::uint32_t id)
   {
      auto b = static_cast<unsigned char*>(p);
      for (std::size_t i = 0; i != size; ++i)
         b[i] = static_cast<unsigned char>(id * 31 + i);
   }

   bool intact(void const* p, std::size_t size, std::uint32_t id)
   {
      auto b = static_cast<unsigned char const*>(p);
      for (std::size_t i = 0; i != size; ++i)
      {
         if (b[i] != static_cast<unsigned char>(id * 31 + i))
            return false;
      }
      return true;
   }

   struct block
   {
      std::size_t       size;
      std::uint32_t     id;
   };

   // Consume everything, checking order, sizes, alignment and contents
   // against the blocks expected, which it removes.
   void consume_all(q::ring_allocator& ra, std::deque<block>& expected)
   {
      ra.consume(
         [&](void* p, std::size_t size)
         {
            REQUIRE(!expected.empty());
            auto b = expected.front();
            expected.pop_front();
            CHECK(size == b.size);
            CHECK(aligned(p));
            CHECK(intact(p, size, b.id));
         });
   }
}

TEST_CASE("ring_allocator: nothing is visible before publish")
{
   q::ring_allocator ra{1024};

   auto p = ra.allocate(10);
   REQUIRE(p);
   fill(p, 10, 1);

   int seen = 0;
   ra.consume([&](void*, std::size_t) { ++seen; });
   CHECK(seen == 0);

   ra.publish();
   std::deque<block> expected{{10, 1}};
   consume_all(ra, expected);
   CHECK(expected.empty());

   // Consumed once only.
   ra.consume([&](void*, std::size_t) { ++seen; });
   CHECK(seen == 0);
}

TEST_CASE("ring_allocator: consume stops where f returns false")
{
   q::ring_allocator ra{1024};
   std::deque<block> expected;

   for (std::uint32_t id = 0; id != 6; ++id)
   {
      auto p = ra.allocate(20);
      REQUIRE(p);
      fill(p, 20, id);
      expected.push_back({20, id});
   }
   ra.publish();

   // Take three, then stop: the rest stay, in order, for the next
   // consume.
   int taken = 0;
   ra.consume(
      [&](void* p, std::size_t size)
      {
         if (taken == 3)
            return false;
         auto b = expected.front();
         expected.pop_front();
         CHECK(intact(p, size, b.id));
         ++taken;
         return true;
      });
   CHECK(taken == 3);
   CHECK(expected.size() == 3);

   consume_all(ra, expected);
   CHECK(expected.empty());

   // The space of all six is free again.
   for (int i = 0; i != 6; ++i)
      CHECK(ra.allocate(20));
}

TEST_CASE("ring_allocator: blocks come out in order, aligned and intact")
{
   q::ring_allocator ra{4096};
   std::deque<block> expected;

   std::uint32_t id = 0;
   for (std::size_t size : {1, 7, 16, 17, 0, 100, 3})
   {
      auto p = ra.allocate(size);
      REQUIRE(p);
      CHECK(aligned(p));
      fill(p, size, id);
      expected.push_back({size, id++});
   }
   ra.publish();
   consume_all(ra, expected);
   CHECK(expected.empty());
}

TEST_CASE("ring_allocator: full, then freed by consume")
{
   q::ring_allocator ra{256};
   std::deque<block> expected;

   // Allocate until it no longer fits.
   std::uint32_t id = 0;
   while (auto p = ra.allocate(24))
   {
      fill(p, 24, id);
      expected.push_back({24, id++});
   }
   CHECK(id > 0);

   ra.publish();
   consume_all(ra, expected);
   CHECK(expected.empty());
   CHECK(ra.allocate(24));
}

TEST_CASE("ring_allocator: a block larger than the buffer never fits")
{
   q::ring_allocator ra{256};
   CHECK(!ra.allocate(256));
   CHECK(!ra.allocate(10000));
   CHECK(!ra.allocate(~std::size_t(0)));
   CHECK(ra.allocate(16));
}

TEST_CASE("ring_allocator: half the buffer fits whenever it is empty")
{
   // From many starting positions: move the write position along, empty
   // the ring, and ask for the largest block it promises. A header of two
   // sizes goes ahead of each block.
   constexpr std::size_t header = 2 * sizeof(std::size_t);
   for (std::size_t shift = 0; shift != 64; ++shift)
   {
      q::ring_allocator ra{512};
      for (std::size_t i = 0; i != shift; ++i)
      {
         REQUIRE(ra.allocate(i % 5));
         ra.publish();
         ra.consume([](void*, std::size_t) {});
      }

      auto p = ra.allocate(512 / 2 - header);
      CHECK(p);
      ra.publish();
      ra.consume([](void*, std::size_t) {});
   }
}

TEST_CASE("ring_allocator: random use, checked against a model")
{
   std::mt19937 rng{42};
   std::uniform_int_distribution<int> op{0, 9};
   std::uniform_int_distribution<std::size_t> size_of{0, 200};

   q::ring_allocator ra{1024};
   std::deque<block> published;
   std::deque<block> pending;
   std::uint32_t id = 0;
   int allocated = 0;
   int refused = 0;

   for (int step = 0; step != 100000; ++step)
   {
      auto o = op(rng);
      if (o < 6)
      {
         auto size = size_of(rng);
         if (auto p = ra.allocate(size))
         {
            CHECK(aligned(p));
            fill(p, size, id);
            pending.push_back({size, id++});
            ++allocated;
         }
         else
         {
            ++refused;
         }
      }
      else if (o < 8)
      {
         ra.publish();
         published.insert(published.end(), pending.begin(), pending.end());
         pending.clear();
      }
      else
      {
         consume_all(ra, published);
         CHECK(published.empty());
      }
   }
   CHECK(allocated > 1000);
   CHECK(refused > 0);
}

TEST_CASE("ring_allocator: two-thread stress")
{
   constexpr std::uint32_t num_blocks = 200000;

   q::ring_allocator ra{2048};
   std::atomic<bool> ok = true;

   std::thread producer{
      [&]
      {
         std::mt19937 rng{7};
         std::uniform_int_distribution<std::size_t> size_of{4, 300};
         for (std::uint32_t id = 0; id != num_blocks; ++id)
         {
            auto size = size_of(rng);
            void* p;
            while (!(p = ra.allocate(size)))
               ;
            fill(p, size, id);
            std::memcpy(p, &id, sizeof(id));
            ra.publish();
         }
      }
   };

   std::uint32_t next = 0;
   while (next != num_blocks)
   {
      ra.consume(
         [&](void* p, std::size_t size)
         {
            std::uint32_t id;
            std::memcpy(&id, p, sizeof(id));
            if (id != next || !aligned(p))
               ok = false;
            // The id overwrote the start of the pattern; check the rest.
            auto b = static_cast<unsigned char const*>(p);
            for (std::size_t i = sizeof(id); i != size; ++i)
            {
               if (b[i] != static_cast<unsigned char>(id * 31 + i))
                  ok = false;
            }
            ++next;
         });
   }
   producer.join();
   CHECK(ok);
   CHECK(next == num_blocks);
}
