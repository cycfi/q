/*=============================================================================
   Copyright (c) 2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FLOAT_CONVERT_HPP_SEPTEMBER_20_2022)
#define CYCFI_Q_FLOAT_CONVERT_HPP_SEPTEMBER_20_2022

#include <cstdint>
#include <infra/assert.hpp>
#include <type_traits>
#include <limits>
#include <algorithm>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Conversion to and from float -1.0..+1.0 for two's complement (signed)
   // and offset-binary (unsigned) sample formats.
   //
   // Behavior:
   // • to_float: no runtime checks; assumes the input integer uses 'bits'
   //   LSBs.
   // • from_float: saturates to the representable range [-1.0, 1.0) before
   //   conversion.
   ////////////////////////////////////////////////////////////////////////////

   template <typename T, int bits>
   constexpr float to_float(T s)
   {
      using U = std::make_unsigned_t<T>;
      static_assert(bits <= int(std::numeric_limits<U>::digits), "bits exceeds width of T");
      constexpr float half = static_cast<float>(U{1} << (bits - 1));

      if constexpr (std::is_signed_v<T>)
         return static_cast<float>(s) / half;
      else
         return (static_cast<float>(s) - half) / half;
   }

   template <typename T, int bits>
   constexpr T from_float(float s)
   {
      using U = std::make_unsigned_t<T>;
      constexpr U half_u = U{1} << (bits - 1);
      constexpr float half = static_cast<float>(half_u);

      float r = s * half;
      r = std::clamp(r, -half, half - 1.0f);     // saturate to [-half, half-1]
      if constexpr (std::is_signed_v<T>)
         return static_cast<T>(r);
      else
         return static_cast<T>(r + half_u);
   }

   template <typename T, int bits>
   struct to_float_converter
   {
      constexpr float operator()(T s) const
      {
         return to_float<T, bits>(s);
      }
   };

   template <typename T, int bits>
   struct from_float_converter
   {
      constexpr T operator()(float s) const
      {
         return from_float<T, bits>(s);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Conversion to and from unsigned float 0.0..1.0 for offset-binary
   // samples.
   //
   // Behavior:
   // • to_unsigned_float: no runtime checks; assumes the input integer uses
   //   'bits' LSBs.
   // • from_unsigned_float: clamps s to [0.0, 1.0] before conversion.
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      template <typename T, int bits>
      constexpr std::make_unsigned_t<T> get_max_uint()
      {
         using U = std::make_unsigned_t<T>;
         static_assert(bits >= 1, "bits must be >= 1");
         static_assert(bits <= int(std::numeric_limits<U>::digits), "bits exceeds width of T");
         return (bits == int(std::numeric_limits<U>::digits))
            ? ~U{0}
            : (U{1} << bits) - U{1};
      }
   }

   template <typename T, int bits>
   constexpr float to_unsigned_float(T s)
   {
      static_assert(std::is_unsigned_v<T>, "to/from_unsigned_float: T must be unsigned");
      using U = std::make_unsigned_t<T>;
      constexpr U max_u = detail::get_max_uint<T, bits>();
      constexpr float inv_max = 1.0f / static_cast<float>(max_u);
      return static_cast<float>(static_cast<U>(s)) * inv_max;
   }

   template <typename T, int bits>
   constexpr T from_unsigned_float(float s)
   {
      static_assert(std::is_unsigned_v<T>, "to/from_unsigned_float: T must be unsigned");
      using U = std::make_unsigned_t<T>;
      constexpr U max = detail::get_max_uint<T, bits>();

      // Clamp s to [0,1] then scale to [0, max]
      float u = std::clamp(s, 0.0f, 1.0f) * static_cast<float>(max);
      return static_cast<T>(static_cast<U>(u));
   }

   template <typename T, int bits>
   struct to_unsigned_float_converter
   {
      constexpr float operator()(T s) const
      {
         return to_unsigned_float<T, bits>(s);
      }
   };

   template <typename T, int bits>
   struct from_unsigned_float_converter
   {
      constexpr T operator()(float s) const
      {
         return from_unsigned_float<T, bits>(s);
      }
   };

}

#endif
