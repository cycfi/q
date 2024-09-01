/*=============================================================================
   Copyright (c) 2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FLOAT_CONVERT_HPP_SEPTEMBER_20_2022)
#define CYCFI_Q_FLOAT_CONVERT_HPP_SEPTEMBER_20_2022

#include <cstdint>
#include <infra/assert.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Conversion to and from float -1.0 to 1.0 range, from offset binary
   // samples coming from the ADC and going to DACs.
   //
   // Input range checking is not performed.
   ////////////////////////////////////////////////////////////////////////////

   template <typename T, int resolution>
   constexpr float to_float(T s)
   {
      CYCFI_ASSERT(((s >= 0 || s < resolution)), "Invalid range");
      constexpr float half_resolution = resolution / 2;
      return (s / half_resolution) - 1.0f;
   }

   template <typename T, int resolution>
   constexpr T from_float(float s)
   {
      CYCFI_ASSERT(((s >= -1.0f || s <= 1.0f)), "Invalid range");
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

   ////////////////////////////////////////////////////////////////////////////
   // Conversion to and from unsigned float 0.0 to 1.0 range, from offset
   // binary samples coming from the ADC and going to DACs.
   ////////////////////////////////////////////////////////////////////////////

   template <typename T, int resolution>
   constexpr float to_unsigned_float(T s)
   {
      CYCFI_ASSERT(((s >= 0 || s < resolution)), "Invalid range");
      return float(s) / (resolution - 1);
   }

   template <typename T, int resolution>
   constexpr T from_unsigned_float(float s)
   {
      CYCFI_ASSERT(((s >= 0.0f || s <= 1.0f)), "Invalid range");
      return s * (resolution - 1);
   }

   template <typename T, int resolution>
   struct to_unsigned_float_converter
   {
      constexpr float operator()(T s) const
      {
         return to_unsigned_float<T, resolution>(s);
      }
   };

   template <typename T, int resolution>
   struct from_unsigned_float_converter
   {
      constexpr T operator()(float s) const
      {
         return from_unsigned_float<T, resolution>(s);
      }
   };
}

#endif
