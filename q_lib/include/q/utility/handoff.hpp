/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_HANDOFF_OCTOBER_12_2026)
#define CYCFI_Q_HANDOFF_OCTOBER_12_2026

#include <atomic>
#include <memory>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // handoff: hands objects that need allocation (a sample buffer, a
   // wavetable, an image) from one thread, the owner, to another, the
   // reader, without a lock. The transfer is one way, and the latest
   // object wins.
   //
   // The owner allocates every object and is the only thread that frees
   // one. send hands a new object over; the reader takes the latest with
   // receive, once per cycle (an audio thread's block, say), and the
   // pointer stays valid for that cycle only. receive never blocks,
   // allocates or frees. send frees the older objects first, once receive
   // has returned the newest one, so the object before the current one
   // stays alive until the next send.
   //
   // The reader must be stopped before the handoff is destroyed.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   class handoff
   {
   public:

      using value_type = T;
      using pointer = std::unique_ptr<T>;

                        handoff() = default;
                        handoff(handoff const&) = delete;
      handoff&          operator=(handoff const&) = delete;

      // Owner thread.
      void              send(pointer p);

      // Reader thread.
      T const*          receive();

   private:

      using owned_list = std::vector<pointer>;
      using slot = std::atomic<T*>;

      void              reclaim();

      owned_list        _owned;
      slot              _current = nullptr;
      slot              _seen = nullptr;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////

   // Hand a new object to the reader, freeing first what the reader can
   // no longer be using.
   template <typename T>
   inline void handoff<T>::send(pointer p)
   {
      reclaim();
      _current.store(p.get(), std::memory_order_release);
      _owned.push_back(std::move(p));
   }

   // Free every object but the current one, once receive has returned the
   // current one. A cycle that started after that cannot hold an older one.
   template <typename T>
   inline void handoff<T>::reclaim()
   {
      auto cur = _current.load(std::memory_order_relaxed);
      if (_seen.load(std::memory_order_acquire) == cur)
      {
         std::erase_if(_owned,
            [cur](auto const& p) { return p.get() != cur; });
      }
   }

   // The latest object, or nullptr before the first send. Valid until the
   // next call; call once at the start of each cycle.
   template <typename T>
   inline T const* handoff<T>::receive()
   {
      auto p = _current.load(std::memory_order_acquire);
      _seen.store(p, std::memory_order_release);
      return p;
   }
}

#endif
