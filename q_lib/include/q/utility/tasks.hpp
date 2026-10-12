/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TASKS_OCTOBER_12_2026)
#define CYCFI_Q_TASKS_OCTOBER_12_2026

#include <q/utility/ring_allocator.hpp>
#include <q/support/duration.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace cycfi::q
{
   namespace concepts
   {
      // What a clock must give basic_tasks: the time now, and a duration
      // in the same ticks, both callable from any thread.
      template <typename C>
      concept Clock = requires(C const& c, duration d)
      {
         { c.now() } -> std::same_as<std::int64_t>;
         { c.ticks(d) } -> std::same_as<std::int64_t>;
      };
   }

   namespace detail
   {
      // A callable basic_tasks can hold: copying or destroying it can
      // never allocate or free.
      template <typename F>
      constexpr bool is_task =
         std::is_invocable_v<F&> &&
         std::is_trivially_copyable_v<F> &&
         std::is_trivially_destructible_v<F> &&
         alignof(F) <= alignof(std::max_align_t);

      // Wall time, in nanoseconds.
      struct steady_ticks
      {
         std::int64_t now() const
         {
            using namespace std::chrono;
            auto t = steady_clock::now().time_since_epoch();
            return duration_cast<nanoseconds>(t).count();
         }

         std::int64_t ticks(duration d) const
         {
            return std::llround(as_double(d) * 1e9);
         }
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // basic_tasks: work posted from one thread, the poster, to be run on
   // another, the owner, without a lock. The owner runs what is due with
   // poll, once per cycle (an audio thread's block, say). Neither side
   // blocks or allocates, so either may be a real-time thread.
   //
   // A task is any callable that is trivially copyable and trivially
   // destructible, posted to run at the next poll, or after a delay on
   // the Clock. The tasks wait in a ring_allocator; those not yet due are
   // moved to a second one, which only the owner touches. See note 1.
   //
   // Clock may be a reference, to a clock held elsewhere: an audio_clock
   // the audio thread advances, say.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   class basic_tasks
   {
   public:

      explicit          basic_tasks(std::size_t bytes)
                         requires std::is_default_constructible_v<Clock>;
                        basic_tasks(Clock clock, std::size_t bytes);

      // Poster thread.
                        template <typename F>
                        requires detail::is_task<std::decay_t<F>>
      bool              post(F&& f);

                        template <typename F>
                        requires detail::is_task<std::decay_t<F>>
      bool              post(duration delay, F&& f);

      // Owner thread.
      void              poll();

      // Any thread. The earliest due time of the tasks poll has seen and
      // not yet run, on the clock; the largest value if there are none.
      std::int64_t      next_due() const;

   private:

      using limits = std::numeric_limits<std::int64_t>;
      static constexpr auto never = limits::max();
      static constexpr auto immediate = limits::min();

      // Ahead of each task in its block.
      struct alignas(std::max_align_t) header
      {
         void           (*run)(void*);
         std::int64_t   due;
      };

                        template <typename F>
      bool              post_at(std::int64_t due, F&& f);
      bool              defer(void* p, std::size_t size, std::int64_t& next);

      using due_time = std::atomic<std::int64_t>;

      Clock             _clock;
      ring_allocator    _posted;
      ring_allocator    _waiting;
      due_time          _next_due = never;
   };

   // Wall time, for an owner that is not the audio thread.
   using tasks = basic_tasks<detail::steady_ticks>;

   ////////////////////////////////////////////////////////////////////////////
   // Notes
   //
   // 1. poll runs the tasks that are due and moves the rest, as bytes, to
   //    the back of the waiting ring, which a trivially copyable task
   //    allows. The waiting ring has twice the space, so moving its tasks
   //    along always has room while they fill half of it. Beyond that, a
   //    task that cannot move stays where it is, and post fails once the
   //    posted ring fills: nothing is lost. Tasks that come due at the
   //    same poll run in the order posted, unless a ring stopped early.
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   inline basic_tasks<Clock>::basic_tasks(std::size_t bytes)
    requires std::is_default_constructible_v<Clock>
    : _posted(bytes)
    , _waiting(2 * bytes)
   {}

   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   inline basic_tasks<Clock>::basic_tasks(Clock clock, std::size_t bytes)
    : _clock(clock)
    , _posted(bytes)
    , _waiting(2 * bytes)
   {}

   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   template <typename F>
   requires detail::is_task<std::decay_t<F>>
   inline bool basic_tasks<Clock>::post(F&& f)
   {
      return post_at(immediate, std::forward<F>(f));
   }

   // The due time is taken now, on the poster thread, so the delay counts
   // from the post.
   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   template <typename F>
   requires detail::is_task<std::decay_t<F>>
   inline bool basic_tasks<Clock>::post(duration delay, F&& f)
   {
      return post_at(_clock.now() + _clock.ticks(delay), std::forward<F>(f));
   }

   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   template <typename F>
   inline bool basic_tasks<Clock>::post_at(std::int64_t due, F&& f)
   {
      using task = std::decay_t<F>;

      auto p = _posted.allocate(sizeof(header) + sizeof(task));
      if (!p)
         return false;

      auto h = ::new (p) header{
         [](void* t) { (*static_cast<task*>(t))(); }, due};
      ::new (static_cast<void*>(h + 1)) task(std::forward<F>(f));
      _posted.publish();
      return true;
   }

   // A task not yet due goes to the back of the waiting ring. False if
   // there is no room.
   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   inline bool basic_tasks<Clock>::defer(
      void* p, std::size_t size, std::int64_t& next)
   {
      auto q = _waiting.allocate(size);
      if (!q)
         return false;
      std::memcpy(q, p, size);
      next = std::min(next, static_cast<header*>(p)->due);
      return true;
   }

   // The waiting tasks were posted before those still in the posted ring,
   // so they go first. A ring that stops early keeps what is left for the
   // next poll, which is then due at once.
   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   inline void basic_tasks<Clock>::poll()
   {
      auto const now = _clock.now();
      auto next = never;
      bool stopped = false;

      auto run_or_defer =
         [&](void* p, std::size_t size)
         {
            auto h = static_cast<header*>(p);
            if (h->due <= now)
            {
               h->run(h + 1);
               return true;
            }
            if (defer(p, size, next))
               return true;
            stopped = true;
            return false;
         };

      _waiting.consume(run_or_defer);
      _posted.consume(run_or_defer);
      _waiting.publish();

      _next_due.store(stopped? now : next, std::memory_order_relaxed);
   }

   template <typename Clock>
   requires concepts::Clock<std::remove_reference_t<Clock>>
   inline std::int64_t basic_tasks<Clock>::next_due() const
   {
      return _next_due.load(std::memory_order_relaxed);
   }
}

#endif
