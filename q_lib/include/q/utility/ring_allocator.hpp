/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_RING_ALLOCATOR_OCTOBER_12_2026)
#define CYCFI_Q_RING_ALLOCATOR_OCTOBER_12_2026

#include <infra/support.hpp>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // ring_allocator: blocks of any size, allocated by one thread, the
   // producer, and freed by another, the consumer, in the order they were
   // allocated, without a lock. Neither side blocks, and nothing is
   // allocated after construction, so either may be a real-time thread.
   //
   // The blocks come from one buffer, allocated when the ring_allocator is
   // made, its size rounded up to a power of 2 and fixed from then on.
   // Each block is aligned for any type, and has a small header ahead of
   // it. allocate returns null when a block does not fit in the space
   // left; once the consumer has freed everything, any block up to half
   // the buffer, less its header, fits.
   //
   // A block is invisible to the consumer until publish. consume passes
   // each published block to the consumer, oldest first, then frees them.
   // A consumer that returns bool can stop early: returning false leaves
   // that block, and the ones after it, for a later consume.
   ////////////////////////////////////////////////////////////////////////////
   class ring_allocator
   {
   public:

      explicit          ring_allocator(std::size_t bytes);
                        ring_allocator(ring_allocator const&) = delete;
      ring_allocator&   operator=(ring_allocator const&) = delete;

      // Producer thread.
      void*             allocate(std::size_t size);
      void              publish();

      // Consumer thread. Calls f(p, size) for each published block, up to
      // the first for which a bool returning f returns false.
                        template <typename F>
      void              consume(F&& f);

   private:

      // Ahead of each block. A skip header marks the space up to the end
      // of the buffer as unused: the next block is at the start.
      struct alignas(std::max_align_t) header
      {
         std::size_t    record;        // header and block, in bytes
         std::size_t    size;          // the block, as asked for
      };

      static constexpr std::size_t skip = ~std::size_t(0);

      // Every record is a whole number of units, so the space left at the
      // end of the buffer always has room for a header.
      static constexpr std::size_t unit = sizeof(header);
      static_assert((unit & (unit - 1)) == 0);

      struct release
      {
         void operator()(std::byte* p) const
         {
            ::operator delete[](
               p, std::align_val_t{alignof(std::max_align_t)});
         }
      };

      using buffer_ptr = std::unique_ptr<std::byte[], release>;
      using position = std::atomic<std::size_t>;

      header*           at(std::size_t i) const;

      std::size_t             _size;
      buffer_ptr              _buffer;
      std::size_t             _alloc = 0;       // the producer's
      alignas(64) position    _read = 0;        // the consumer's
      alignas(64) position    _write = 0;       // the producer's
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline ring_allocator::ring_allocator(std::size_t bytes)
    : _size(smallest_pow2(std::max(bytes, 2 * unit)))
    , _buffer(static_cast<std::byte*>(::operator new[](
         _size, std::align_val_t{alignof(std::max_align_t)})))
   {}

   inline ring_allocator::header* ring_allocator::at(std::size_t i) const
   {
      return reinterpret_cast<header*>(_buffer.get() + (i & (_size - 1)));
   }

   // The block goes at the allocation position or, when it does not fit
   // before the end, at the start, with a skip header over the gap.
   inline void* ring_allocator::allocate(std::size_t size)
   {
      if (size > _size)
         return nullptr;

      auto const record = unit + (size + unit - 1) / unit * unit;
      auto w = _alloc;
      auto const used = w - _read.load(std::memory_order_acquire);
      auto const to_end = _size - (w & (_size - 1));
      auto const gap = to_end < record? to_end : 0;
      if (_size - used < gap + record)
         return nullptr;

      if (gap)
      {
         ::new (static_cast<void*>(at(w))) header{gap, skip};
         w += gap;
      }

      auto h = ::new (static_cast<void*>(at(w))) header{record, size};
      _alloc = w + record;
      return h + 1;
   }

   // Everything allocated so far becomes visible to the consumer, written
   // as the producer left it.
   inline void ring_allocator::publish()
   {
      _write.store(_alloc, std::memory_order_release);
   }

   // Only the blocks published when consume starts are passed on. The
   // space of those consumed is freed in one store, after the last.
   template <typename F>
   inline void ring_allocator::consume(F&& f)
   {
      using result = std::invoke_result_t<F&, void*, std::size_t>;

      auto r = _read.load(std::memory_order_relaxed);
      auto const w = _write.load(std::memory_order_acquire);
      while (r != w)
      {
         auto h = at(r);
         if (h->size != skip)
         {
            if constexpr (std::is_same_v<result, bool>)
            {
               if (!f(static_cast<void*>(h + 1), h->size))
                  break;
            }
            else
            {
               f(static_cast<void*>(h + 1), h->size);
            }
         }
         r += h->record;
      }
      _read.store(r, std::memory_order_release);
   }
}

#endif
