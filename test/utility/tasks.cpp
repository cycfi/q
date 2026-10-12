/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/tasks.hpp>
#include <q/utility/audio_clock.hpp>
#include <q/support/literals.hpp>

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   template <typename F>
   constexpr bool can_post = requires(q::tasks& t, F f) { t.post(f); };

   struct counter
   {
      int n = 0;
   };

   // A clock the test sets by hand, in milliseconds.
   struct test_clock
   {
      std::int64_t now() const { return t.load(); }
      std::int64_t ticks(q::duration d) const
      {
         return std::llround(q::as_double(d) * 1000);
      }

      std::atomic<std::int64_t> t = 0;
   };

   using clocked_tasks = q::basic_tasks<test_clock&>;

   // A task carrying k values, which it checks when it runs.
   template <std::size_t k>
   auto payload_task(counter* c, int id)
   {
      std::array<int, k> values;
      for (std::size_t i = 0; i != k; ++i)
         values[i] = id + int(i);
      return [c, id, values]
      {
         for (std::size_t i = 0; i != k; ++i)
         {
            if (values[i] != id + int(i))
               return;
         }
         ++c->n;
      };
   }
}

TEST_CASE("tasks: poll runs the posted tasks in order")
{
   q::tasks t{1024};
   std::vector<int> ran;
   auto* p = &ran;

   CHECK(t.post([p] { p->push_back(1); }));
   CHECK(t.post([p] { p->push_back(2); }));
   CHECK(t.post([p] { p->push_back(3); }));
   CHECK(ran.empty());

   t.poll();
   CHECK(ran == std::vector<int>{1, 2, 3});

   // Nothing is waiting: poll does nothing.
   t.poll();
   CHECK(ran.size() == 3);
}

TEST_CASE("tasks: post fails when full, and poll frees the space")
{
   q::tasks t{256};
   counter c;
   auto* p = &c;

   int posted = 0;
   while (t.post([p] { ++p->n; }))
      ++posted;
   CHECK(posted > 0);

   t.poll();
   CHECK(c.n == posted);
   CHECK(t.post([p] { ++p->n; }));
   t.poll();
   CHECK(c.n == posted + 1);
}

TEST_CASE("tasks: tasks of any size up to the buffer")
{
   q::tasks t{512};
   counter c;

   CHECK(t.post(payload_task<1>(&c, 1)));
   CHECK(t.post(payload_task<40>(&c, 2)));
   CHECK(t.post(payload_task<3>(&c, 3)));
   t.poll();
   CHECK(c.n == 3);

   // Larger than the whole buffer: never fits.
   CHECK(!t.post(payload_task<200>(&c, 4)));
   t.poll();
   CHECK(c.n == 3);
}

TEST_CASE("tasks: a task that does not fit before the end wraps")
{
   q::tasks t{256};
   counter c;

   // Fill most of the buffer and run it, so the write position sits near
   // the end. Then a task larger than what is left before the end goes
   // to the start, intact.
   for (int round = 0; round != 8; ++round)
   {
      CHECK(t.post(payload_task<20>(&c, round)));
      CHECK(t.post(payload_task<7>(&c, round)));
      t.poll();
   }
   CHECK(c.n == 16);
}

TEST_CASE("tasks: a task posted during poll waits for the next poll")
{
   q::tasks t{1024};
   counter c;
   auto* p = &c;
   auto* tp = &t;

   t.post([p, tp] { ++p->n; tp->post([p] { p->n += 10; }); });
   t.poll();
   CHECK(c.n == 1);
   t.poll();
   CHECK(c.n == 11);
}

TEST_CASE("tasks: only trivially copyable callables are accepted")
{
   int i = 0;
   auto small = [&i] { ++i; };
   auto large = [a = std::array<char, 256>{}] { (void) a; };
   auto owning = [s = std::make_shared<int>(0)] { ++*s; };
   std::function<void()> erased = [] {};

   static_assert(can_post<decltype(small)>);
   static_assert(can_post<decltype(large)>);
   static_assert(!can_post<decltype(owning)>);
   static_assert(!can_post<decltype(erased)>);
   SUCCEED();
}

TEST_CASE("tasks: two-thread stress, tasks of mixed sizes")
{
   constexpr int num_tasks = 200000;

   q::tasks t{1024};

   // Touched only by the owner, inside the tasks.
   struct state
   {
      int next = 0;
      int ran = 0;
      bool in_order = true;
      bool intact = true;
   } s;
   auto* sp = &s;

   auto sized =
      [sp]<std::size_t k>(int id, std::integral_constant<std::size_t, k>)
      {
         std::array<int, k> values;
         values.fill(id);
         return [sp, id, values]
         {
            for (auto v : values)
            {
               if (v != id)
                  sp->intact = false;
            }
            if (id != sp->next)
               sp->in_order = false;
            sp->next = id + 1;
            ++sp->ran;
         };
      };

   std::thread poster{
      [&]
      {
         for (int i = 0; i != num_tasks; ++i)
         {
            auto post =
               [&](auto task)
               {
                  while (!t.post(task))
                     ;
               };
            switch (i % 3)
            {
               case 0:
                  post(sized(i, std::integral_constant<std::size_t, 1>{}));
                  break;
               case 1:
                  post(sized(i, std::integral_constant<std::size_t, 13>{}));
                  break;
               default:
                  post(sized(i, std::integral_constant<std::size_t, 40>{}));
                  break;
            }
         }
      }
   };

   while (s.ran != num_tasks)
      t.poll();
   poster.join();

   CHECK(s.ran == num_tasks);
   CHECK(s.in_order);
   CHECK(s.intact);
}

TEST_CASE("tasks: a delayed task runs at the first poll at its time")
{
   test_clock clock;
   clocked_tasks t{clock, 1024};
   counter c;
   auto* p = &c;

   clock.t = 5;
   CHECK(t.post(10_ms, [p] { ++p->n; }));      // due at 15

   t.poll();
   CHECK(c.n == 0);
   CHECK(t.next_due() == 15);

   clock.t = 14;
   t.poll();
   CHECK(c.n == 0);

   clock.t = 20;
   t.poll();
   CHECK(c.n == 1);
   CHECK(t.next_due() == std::numeric_limits<std::int64_t>::max());

   // Run once only.
   t.poll();
   CHECK(c.n == 1);
}

TEST_CASE("tasks: a delayed task does not hold up the tasks behind it")
{
   test_clock clock;
   clocked_tasks t{clock, 1024};
   std::vector<int> ran;
   auto* p = &ran;

   t.post([p] { p->push_back(1); });
   t.post(1_s, [p] { p->push_back(2); });
   t.post([p] { p->push_back(3); });
   t.post(10_ms, [p] { p->push_back(4); });
   t.poll();
   CHECK(ran == std::vector<int>{1, 3});
   CHECK(t.next_due() == 10);

   clock.t = 10;
   t.post([p] { p->push_back(5); });
   t.poll();
   CHECK(ran == std::vector<int>{1, 3, 4, 5});
   CHECK(t.next_due() == 1000);

   clock.t = 1000;
   t.poll();
   CHECK(ran == std::vector<int>{1, 3, 4, 5, 2});
}

TEST_CASE("tasks: tasks due at the same poll run in the order posted")
{
   test_clock clock;
   clocked_tasks t{clock, 1024};
   std::vector<int> ran;
   auto* p = &ran;

   t.post(30_ms, [p] { p->push_back(1); });
   t.post(10_ms, [p] { p->push_back(2); });
   t.poll();
   t.post(20_ms, [p] { p->push_back(3); });
   t.poll();
   CHECK(ran.empty());

   clock.t = 100;
   t.poll();
   CHECK(ran == std::vector<int>{1, 2, 3});
}

TEST_CASE("tasks: many delayed tasks: none lost when the space runs short")
{
   test_clock clock;
   clocked_tasks t{clock, 256};
   counter c;
   auto* p = &c;

   // Post and poll until the space is used up by tasks not yet due.
   int posted = 0;
   for (int i = 0; i != 1000; ++i)
   {
      if (t.post(1_s, [p] { ++p->n; }))
         ++posted;
      t.poll();
   }
   CHECK(posted > 0);
   CHECK(posted < 1000);
   CHECK(c.n == 0);

   // Then all of them come due, and every one runs.
   clock.t = 1000;
   for (int i = 0; i != 10 && c.n != posted; ++i)
      t.poll();
   CHECK(c.n == posted);
}

TEST_CASE("tasks: on an audio_clock, two-thread stress with delays")
{
   constexpr int num_tasks = 50000;

   q::audio_clock clock{48000};
   q::basic_tasks<q::audio_clock&> t{clock, 4096};

   // Touched only by the owner, inside the tasks.
   struct state
   {
      int ran = 0;
      int next_immediate = 0;
      bool in_order = true;
      bool on_time = true;
   } s;
   auto* sp = &s;
   auto* cp = &clock;

   std::thread poster{
      [&]
      {
         for (int i = 0; i != num_tasks; ++i)
         {
            if (i % 2 == 0)
            {
               // Immediate: run in the order posted.
               auto task =
                  [sp, i]
                  {
                     if (i / 2 != sp->next_immediate)
                        sp->in_order = false;
                     ++sp->next_immediate;
                     ++sp->ran;
                  };
               while (!t.post(task))
                  ;
            }
            else
            {
               // Delayed by 0 to 5 ms: never early.
               auto delay = q::duration{(i % 6) * 0.001};
               auto earliest = clock.now() + clock.ticks(delay);
               auto task =
                  [sp, cp, earliest]
                  {
                     if (cp->now() < earliest)
                        sp->on_time = false;
                     ++sp->ran;
                  };
               while (!t.post(delay, task))
                  ;
            }
         }
      }
   };

   // The owner: an audio thread, advancing its clock a block at a time.
   while (s.ran != num_tasks)
   {
      clock.advance(64);
      t.poll();
   }
   poster.join();

   CHECK(s.ran == num_tasks);
   CHECK(s.in_order);
   CHECK(s.on_time);
}
