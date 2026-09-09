/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_io/detail/event_queue.hpp>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace q = cycfi::q;

namespace
{
   struct item
   {
      std::uint32_t  value;
      std::size_t    time;
   };

   using queue_type = q::detail::event_queue<item, 8>;
}

TEST_CASE("A new queue is empty")
{
   queue_type q_;
   item out;

   CHECK(q_.empty());
   CHECK(q_.size() == 0);
   CHECK_FALSE(q_.pop(out));
   CHECK(q_.drops() == 0);
}

TEST_CASE("What is pushed comes back")
{
   queue_type q_;
   item out;

   CHECK(q_.push({0x903C40, 128}));
   CHECK_FALSE(q_.empty());
   CHECK(q_.size() == 1);

   REQUIRE(q_.pop(out));
   CHECK(out.value == 0x903C40);
   CHECK(out.time == 128);
   CHECK(q_.empty());
}

TEST_CASE("Order is preserved")
{
   queue_type q_;
   item out;

   for (std::uint32_t i = 0; i != 4; ++i)
      REQUIRE(q_.push({i, i * 10}));

   for (std::uint32_t i = 0; i != 4; ++i)
   {
      REQUIRE(q_.pop(out));
      CHECK(out.value == i);
      CHECK(out.time == i * 10);
   }
   CHECK(q_.empty());
}

TEST_CASE("The buffer wraps")
{
   // Push and pop one at a time, many more times around than the queue holds,
   // so every index is reused several times.
   queue_type q_;
   item out;

   for (std::uint32_t i = 0; i != 100; ++i)
   {
      REQUIRE(q_.push({i, i}));
      REQUIRE(q_.pop(out));
      CHECK(out.value == i);
   }
   CHECK(q_.empty());
   CHECK(q_.drops() == 0);
}

TEST_CASE("A full queue drops the newest event and counts it")
{
   // The capacity is one less than the size: the queue is full when the write
   // index is one behind the read index, which is what keeps the two indices
   // unambiguous.
   queue_type q_;
   item out;

   for (std::uint32_t i = 0; i != queue_type::capacity; ++i)
      REQUIRE(q_.push({i, i}));

   CHECK(q_.size() == queue_type::capacity);
   CHECK_FALSE(q_.push({999, 999}));
   CHECK(q_.drops() == 1);

   CHECK_FALSE(q_.push({998, 998}));
   CHECK(q_.drops() == 2);

   // The events already in the queue are the ones that survive.
   for (std::uint32_t i = 0; i != queue_type::capacity; ++i)
   {
      REQUIRE(q_.pop(out));
      CHECK(out.value == i);
   }
   CHECK(q_.empty());
}

TEST_CASE("A drained queue accepts events again")
{
   queue_type q_;
   item out;

   for (std::uint32_t i = 0; i != queue_type::capacity; ++i)
      REQUIRE(q_.push({i, i}));
   CHECK_FALSE(q_.push({999, 999}));

   while (q_.pop(out))
      ;

   CHECK(q_.empty());
   CHECK(q_.push({42, 42}));
   REQUIRE(q_.pop(out));
   CHECK(out.value == 42);

   // The drop count is cumulative; draining does not clear it.
   CHECK(q_.drops() == 1);
}

TEST_CASE("One writer and one reader on separate threads lose nothing")
{
   // This is the real arrangement: the device callback writes, the process
   // loop reads, and neither takes a lock. The writer retries on a full
   // queue so the test asserts that nothing is lost or reordered, rather
   // than how deep the queue is.
   constexpr std::uint32_t count = 100000;
   q::detail::event_queue<item, 64> q_;
   std::atomic<bool> done{false};

   std::thread writer{
      [&]
      {
         for (std::uint32_t i = 0; i != count; ++i)
            while (!q_.push({i, i}))
               std::this_thread::yield();
         done = true;
      }
   };

   std::uint32_t expected = 0;
   item out;
   while (expected != count)
   {
      if (q_.pop(out))
      {
         REQUIRE(out.value == expected);
         REQUIRE(out.time == expected);
         ++expected;
      }
      else if (done && q_.empty())
      {
         break;
      }
   }

   writer.join();
   CHECK(expected == count);
   CHECK(q_.empty());
}
