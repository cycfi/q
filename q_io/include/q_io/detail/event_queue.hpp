/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_EVENT_QUEUE_SEPTEMBER_9_2026)
#define CYCFI_Q_EVENT_QUEUE_SEPTEMBER_9_2026

#include <array>
#include <atomic>
#include <cstddef>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // event_queue: a fixed size queue from one writer to one reader, with no
   // lock on either side.
   //
   // A MIDI device hands events to a callback we do not own, on a thread we
   // do not own, while the program reads them from its own loop. Neither may
   // block the other: the callback cannot wait for the reader, and the reader
   // cannot wait for the device.
   //
   // Size must be a power of two. The queue holds Size-1 events, because a
   // write index one behind the read index is what marks it full; were it
   // allowed to catch up, full and empty would look the same.
   //
   // A full queue drops the event offered to it and counts the drop. The
   // alternative, dropping the oldest, would discard a note-on and keep the
   // note-off that ends it.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, std::size_t Size>
   class event_queue
   {
   public:

      static_assert((Size & (Size-1)) == 0, "Size must be a power of two.");
      static_assert(Size >= 2, "Size must be at least two.");

      using value_type = T;

      static constexpr std::size_t size_ = Size;
      static constexpr std::size_t capacity = Size-1;

      bool                    push(T const& val);
      bool                    pop(T& val);

      bool                    empty() const;
      std::size_t             size() const;
      std::size_t             drops() const;

   private:

      static constexpr std::size_t mask = Size-1;

      std::array<T, Size>     _buffer = {};
      std::atomic<std::size_t> _write{0};
      std::atomic<std::size_t> _read{0};
      std::atomic<std::size_t> _drops{0};
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, std::size_t Size>
   inline bool event_queue<T, Size>::push(T const& val)
   {
      auto const write = _write.load(std::memory_order_relaxed);
      auto const next = (write+1) & mask;

      // Acquire: the read index tells us the reader is done with this slot,
      // so we must not see it move before the reader's pop is complete.
      if (next == _read.load(std::memory_order_acquire))
      {
         _drops.fetch_add(1, std::memory_order_relaxed);
         return false;
      }

      _buffer[write] = val;

      // Release: the value is written before the reader can see the index
      // that publishes it.
      _write.store(next, std::memory_order_release);
      return true;
   }

   template <typename T, std::size_t Size>
   inline bool event_queue<T, Size>::pop(T& val)
   {
      auto const read = _read.load(std::memory_order_relaxed);
      if (read == _write.load(std::memory_order_acquire))
         return false;

      val = _buffer[read];
      _read.store((read+1) & mask, std::memory_order_release);
      return true;
   }

   template <typename T, std::size_t Size>
   inline bool event_queue<T, Size>::empty() const
   {
      return _read.load(std::memory_order_acquire)
         == _write.load(std::memory_order_acquire);
   }

   template <typename T, std::size_t Size>
   inline std::size_t event_queue<T, Size>::size() const
   {
      auto const write = _write.load(std::memory_order_acquire);
      auto const read = _read.load(std::memory_order_acquire);
      return (write-read) & mask;
   }

   template <typename T, std::size_t Size>
   inline std::size_t event_queue<T, Size>::drops() const
   {
      return _drops.load(std::memory_order_relaxed);
   }
}

#endif
