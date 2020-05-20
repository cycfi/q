/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_MOVING_MAXIMUM_NOVEMBER_6_2019)
#define CYCFI_Q_EXP_MOVING_MAXIMUM_NOVEMBER_6_2019

#include <q/support/base.hpp>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // moving_maximum: an efficient sliding maximum algorithm that has cost
   // that is O(log2(L)).
   //
   // Brookes: "Algorithms for Max and Min Filters with Improved Worst-Case
   // Performance" IEEE TRANSACTIONS ON CIRCUITS AND SYSTEMS—II: ANALOG AND
   // DIGITAL SIGNAL PROCESSING, VOL. 47, NO. 9, SEPTEMBER 2000
   //
   // Many thanks to Robert Bristow-Johnson.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct moving_maximum
   {
      moving_maximum(duration d, std::size_t sps)
       : moving_maximum(std::size_t(float(d) * sps))
      {}

      moving_maximum(std::size_t size)
       : _size(size)
       , _input_index(0)
      {
         constexpr T very_large_number = 3.40e38;
         std::size_t capacity = smallest_pow2(size);
         _data.resize(capacity * 2, T{});
         std::fill(_data.begin(), _data.end(), -very_large_number);
      }

      T operator()(T value)
      {
         T* array = &_data[0];

         // our main buffer is in the latter half of the array.
         std::size_t index = (_data.size()/2) + _input_index;

         while (index > 1)
         {
            array[index] = value;

            // toggle LSB, the upper bits of the sibling address are the same.
            T sibling_value = array[index ^ 1];

            if (value < sibling_value)
            {
               // use maximum of the two values
               value = sibling_value;
            }
            // parent address is index/2 (drop remainder or "sibling bit")
            index /= 2;
         }

         if (++_input_index >= _size)
            _input_index = 0;

         return value;
      }

   private:

      std::size_t    _size;         // window size
      std::size_t    _input_index;  // the actual sample placement is at (array_size + input_index);
      std::vector<T> _data;         // the big array (twice array_size);
   };
}

#endif
