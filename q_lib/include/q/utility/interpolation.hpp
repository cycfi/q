/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_INFINITY_INTERPOLATION_JULY_20_2014)
#define CYCFI_INFINITY_INTERPOLATION_JULY_20_2014

#include <q/support/base.hpp>
#include <cmath>
#include <cstddef>

namespace cycfi::q
{
   namespace sample_interpolation
   {
      struct none
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            return buffer[std::size_t(index)];
         }
      };

      struct linear
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            auto y1 = buffer[std::size_t(index)];
            auto y2 = buffer[std::size_t(index) + 1];
            auto mu = index - std::floor(index);
            return linear_interpolate(y1, y2, mu);
         }
      };
   }
}

#endif
