/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/handoff.hpp>

#include <array>
#include <atomic>
#include <thread>

namespace q = cycfi::q;

namespace
{
   // Counts live objects, and marks each one dead on destruction so a
   // read after the destructor shows up as a bad tag.
   struct counted
   {
      static constexpr std::uint32_t live_tag = 0x600DF00D;
      static constexpr std::uint32_t dead_tag = 0xDEADBEEF;
      static inline std::atomic<int> live = 0;

      explicit counted(std::uint32_t id_)
       : id(id_)
      {
         data.fill(id_);
         ++live;
      }

      ~counted()
      {
         tag = dead_tag;
         data.fill(dead_tag);
         --live;
      }

      bool valid() const
      {
         if (tag != live_tag)
            return false;
         for (auto d : data)
         {
            if (d != id)
               return false;
         }
         return true;
      }

      std::uint32_t                 tag = live_tag;
      std::uint32_t                 id;
      std::array<std::uint32_t, 256> data;
   };

   auto make(std::uint32_t id)
   {
      return std::make_unique<counted>(id);
   }
}

TEST_CASE("handoff: receive is null before any send")
{
   q::handoff<counted> ho;
   CHECK(ho.receive() == nullptr);
}

TEST_CASE("handoff: receive returns the latest")
{
   q::handoff<counted> ho;

   ho.send(make(1));
   CHECK(ho.receive()->id == 1);

   ho.send(make(2));
   ho.send(make(3));
   CHECK(ho.receive()->id == 3);
}

TEST_CASE("handoff: old objects live until receive has seen the newer one")
{
   counted::live = 0;
   {
      q::handoff<counted> ho;

      // Never read: nothing is freed, since 1 may be in use.
      ho.send(make(1));
      ho.send(make(2));
      ho.send(make(3));
      CHECK(counted::live == 3);

      // receive has seen 3, so the next send frees 1 and 2.
      CHECK(ho.receive()->id == 3);
      ho.send(make(4));
      CHECK(counted::live == 2);
      CHECK(ho.receive()->id == 4);
   }
   CHECK(counted::live == 0);
}

TEST_CASE("handoff: two-thread stress")
{
   constexpr std::uint32_t num_sends = 20000;

   counted::live = 0;
   std::size_t peak = 0;
   {
      q::handoff<counted> ho;
      std::atomic<bool> done = false;
      std::atomic<int> bad = 0;
      std::atomic<int> reads = 0;
      std::atomic<bool> backwards = false;
      std::atomic<std::uint32_t> last_read = 0;

      std::thread reader{
         [&]
         {
            std::uint32_t last = 0;
            while (!done.load(std::memory_order_acquire))
            {
               if (auto p = ho.receive())
               {
                  if (!p->valid())
                     ++bad;
                  if (p->id < last)
                     backwards = true;
                  last = p->id;
                  last_read.store(last, std::memory_order_release);
                  ++reads;
               }
            }
         }
      };

      // Every 64 sends, wait for the reader to catch up, so the bound
      // below holds however little time the reader gets.
      for (std::uint32_t i = 1; i <= num_sends; ++i)
      {
         ho.send(make(i));
         if (auto n = std::size_t(counted::live.load()); n > peak)
            peak = n;
         if (i % 64 == 0)
         {
            while (last_read.load(std::memory_order_acquire) != i)
               ;
         }
      }

      // Let the reader see the last one; the next send frees the rest.
      while (last_read.load(std::memory_order_acquire) != num_sends)
         ;
      done.store(true, std::memory_order_release);
      reader.join();
      ho.send(make(num_sends + 1));

      CHECK(bad == 0);
      CHECK(!backwards);
      CHECK(reads > 0);
      CHECK(counted::live == 2);
   }
   CHECK(counted::live == 0);

   // Once the reader has caught up, the next send frees all but the
   // newest: at most the 64 sent since, and the one before them.
   CHECK(peak <= 64 + 1);
}
