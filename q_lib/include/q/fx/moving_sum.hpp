/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_MOVING_SUM_DECEMBER_7_2018)
#define CYCFI_Q_EXP_MOVING_SUM_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/basic_concepts.hpp>
#include <q/support/frequency.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic_moving_sum computes the moving sum of consecutive samples in a
   // window specified by max_size samples or duration d and float sps.
   //
   // basic_moving_sum can be resized as long as the new size does not exceed
   // the original size (at construction time). When resizing with
   // update=true, when downsizing, the oldest elements are subtracted from
   // the sum. When upsizing, the older elements are added to the sum,
   // otherwise, if update=false, the contents are cleared.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_moving_sum
   {
      using value_type = T;

      basic_moving_sum(std::size_t max_size)
       : _buff(max_size)
       , _size(max_size)
       , _sum{ 0 }
      {
         _buff.clear();
      }

      basic_moving_sum(duration d, float sps)
       : basic_moving_sum(std::size_t(sps * as_float(d)))
      {}

      T operator()(value_type s)
      {
         _sum += s;              // Add the latest sample to the sum
         _sum -= _buff[_size-1]; // Subtract the oldest sample from the sum
         _buff.push(s);          // Push the latest sample, erasing the oldest
         return _sum;
      }

      value_type operator()() const
      {
         return _sum;            // Return the sum
      }

      value_type sum() const
      {
         return _sum;            // Return the sum
      }

      std::size_t size() const
      {
         return _size;
      }

      void resize(std::size_t size, bool update = false)
      {
         // We cannot exceed the original size
         auto new_size = std::min(size, _buff.size());

         if (update)
         {
            if (new_size > _size) // expand
            {
               for (auto i = _size; i != new_size; ++i)
                  _sum += _buff[i];
            }
            else // contract
            {
               for (auto i = new_size; i != _size; ++i)
                  _sum -= _buff[i];
            }
         }
         else
         {
            clear();
         }
         _size = new_size;
      }

      void resize(duration d, float sps, bool update = false)
      {
         resize(std::size_t(sps * as_float(d)), update);
      }

      void clear()
      {
         _buff.clear();
         _sum = 0;
      }

      void fill(T val)
      {
         _buff.fill(val);
         _sum = val * _size;
      }

   private:

      using buffer = ring_buffer<T>;
      using accumulator = decltype(promote(T()));

      buffer      _buff = buffer{};
      std::size_t _size;
      accumulator _sum;
   };

   using moving_sum = basic_moving_sum<float>;

   ////////////////////////////////////////////////////////////////////////////
   // basic_moving_sum_ref computes the moving sum over a history the CALLER
   // owns: basic_moving_sum without the storage. It keeps only the window
   // size and the running sum, so any number of views share one history,
   // each with its own window, for a size and a sum apiece.
   //
   // The history is read BEFORE the caller pushes the current sample, and
   // holds at least `size` samples, `hist[0]` the most recent; any
   // indexable container will do. The caller owns the push, exactly once
   // per sample.
   //
   // The two-value overload takes both ends directly, so the window can
   // hold a function of the raw signal: one history serves an RMS follower
   // and a plain average at once. set() and resize() follow from the same
   // split, taking what only the caller can supply.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_moving_sum_ref
   {
      using value_type = T;
      using accumulator_type = decltype(promote(T()));

      basic_moving_sum_ref(std::size_t size)
       : _size(size)
       , _sum{ 0 }
      {}

      basic_moving_sum_ref(duration d, float sps)
       : basic_moving_sum_ref(std::size_t(sps * as_float(d)))
      {}

      T operator()(value_type s, value_type departing)
      {
         _sum += s;              // Add the latest sample to the sum
         _sum -= departing;      // Subtract the oldest sample from the sum
         return _sum;
      }

      // Same, taking the departing sample from the caller's history.
      template <concepts::IndexableContainer History>
      T operator()(value_type s, History const& hist)
      {
         return (*this)(s, hist[_size-1]);
      }

      value_type operator()() const
      {
         return _sum;            // Return the sum
      }

      value_type sum() const
      {
         return _sum;            // Return the sum
      }

      std::size_t size() const
      {
         return _size;
      }

      // Resize and clear. There is no maximum here: the caller's history
      // bounds the window, so it is the caller that must keep it long enough.
      void resize(std::size_t size)
      {
         _size = size;
         clear();
      }

      void resize(duration d, float sps)
      {
         resize(std::size_t(sps * as_float(d)));
      }

      // Resize, carrying the sum over by walking the caller's history -- what
      // the owning class does against its own buffer when update is true.
      template <concepts::IndexableContainer History>
      void resize(std::size_t size, History const& hist)
      {
         if (size > _size)       // expand
         {
            for (auto i = _size; i != size; ++i)
               _sum += hist[i];
         }
         else                    // contract
         {
            for (auto i = size; i != _size; ++i)
               _sum -= hist[i];
         }
         _size = size;
      }

      void clear()
      {
         _sum = 0;
      }

      // Adopt a sum the caller computed itself. A running sum cannot
      // rebuild itself when that function of the history changes.
      void set(accumulator_type sum)
      {
         _sum = sum;
      }

   private:

      std::size_t _size;
      accumulator_type _sum;
   };

   using moving_sum_ref = basic_moving_sum_ref<float>;
}

#endif
