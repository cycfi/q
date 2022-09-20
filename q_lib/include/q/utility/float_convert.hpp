/*=============================================================================
   Copyright (c) 2022 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FLOAT_CONVERT_HPP_SEPTEMBER_20_2022)
#define CYCFI_Q_FLOAT_CONVERT_HPP_SEPTEMBER_20_2022

#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Conversion to and from float -1.0 to 1.0 range, from offset binary
   // samples coming from the ADC and going to DACs.
   ////////////////////////////////////////////////////////////////////////////

   template <typename T, int resolution>
   constexpr float to_float(T s)
   {
      constexpr float half_resolution = resolution / 2;
      return (s / half_resolution) - 1.0f;
   }

   template <typename T, int resolution>
   constexpr T from_float(float s)
   {
      constexpr T half_resolution = resolution / 2;
      return (s * (half_resolution-1)) + half_resolution;
   }

   template <typename T, int resolution>
   struct to_float_converter
   {
      constexpr float operator()(T s) const
      {
         return to_float<T, resolution>(s);
      }
   };

   template <typename T, int resolution>
   struct from_float_converter
   {
      constexpr T operator()(float s) const
      {
         return from_float<T, resolution>(s);
      }
   };
}

#endif
