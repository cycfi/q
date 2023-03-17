/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_MOVING_SUM_DECEMBER_7_2018)
#define CYCFI_Q_EXP_MOVING_SUM_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/frequency.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // moving_sum computes the moving sum of consecutive samples in a window
   // specified by max_size samples or duration d and float sps.
   //
   // moving_sum can be resized as long as the new size does not exceed the
   // original size (at construction time). When resizing with update=true,
   // when downsizing, the oldest elements are subtracted from the sum. When
   // upsizing, the older elements are added to the sum, otherwise, if
   // update=false, the contents are cleared.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_moving_sum
   {
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

      T operator()(T s)
      {
         _sum += s;              // Add the latest sample to the sum
         _sum -= _buff[_size-1]; // Subtract the oldest sample from the sum
         _buff.push(s);          // Push the latest sample, erasing the oldest
         return _sum;
      }

      T operator()() const
      {
         return _sum;            // Return the sum
      }

      T sum() const
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
         size(std::size_t(sps * as_float(d)), update);
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

      void size(std::size_t size_)
      {
         return _size = std::min(size_, _buff.size());
      }

   private:

      using buffer = ring_buffer<T>;
      using accumulator = decltype(promote(T()));

      buffer      _buff = buffer{};
      std::size_t _size;
      accumulator _sum;
   };

   using moving_sum = basic_moving_sum<float>;
}

#endif
