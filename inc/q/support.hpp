/*=============================================================================
   Copyright (c) 2014-2017 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SUPPORT_HPP_DECEMBER_20_2015)
#define CYCFI_Q_SUPPORT_HPP_DECEMBER_20_2015

#include <type_traits>
#include <cstdint>
#include <cstring>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // Some macros
   ////////////////////////////////////////////////////////////////////////////
   #if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
   # define Q_SATURATES_FLOAT_TO_INT_CONVERSION (true)
   #else
   # define Q_SATURATES_FLOAT_TO_INT_CONVERSION (false)
   #endif

   ////////////////////////////////////////////////////////////////////////////
   // Metaprogramming utilities
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename... Rest>
   struct is_arithmetic
   {
      static constexpr bool value
         = std::is_arithmetic<T>::value && is_arithmetic<Rest...>::value;
   };

   template <typename T>
   struct is_arithmetic<T>
   {
      static constexpr bool value = std::is_arithmetic<T>::value;
   };

   namespace detail
   {
      template <uint32_t bits>
      struct int_that_fits_impl { using type = void; };

      template <>
      struct int_that_fits_impl<8> { using type = int8_t; };

      template <>
      struct int_that_fits_impl<16> { using type = int16_t; };

      template <>
      struct int_that_fits_impl<32> { using type = int32_t; };

      template <>
      struct int_that_fits_impl<64> { using type = int64_t; };

      template <uint32_t bits>
      struct uint_that_fits_impl { using type = void; };

      template <>
      struct uint_that_fits_impl<8> { using type = uint8_t; };

      template <>
      struct uint_that_fits_impl<16> { using type = uint16_t; };

      template <>
      struct uint_that_fits_impl<32> { using type = uint32_t; };

      template <>
      struct uint_that_fits_impl<64> { using type = uint64_t; };

      constexpr uint32_t size_that_fits_int(uint32_t bits)
      {
         if (bits <= 8)
            return 8;
         else if (bits <= 16)
            return 16;
         else if (bits <= 32)
            return 32;
         return 0;
      }
   }

   template <uint32_t bits>
   struct int_that_fits
     : detail::int_that_fits_impl<detail::size_that_fits_int(bits)>
   {
      static_assert(std::is_same<typename int_that_fits<bits>::type, void>::value,
         "Error: No int type fits specified number of bits."
      );
   };

   template <uint32_t bits>
   struct uint_that_fits
     : detail::uint_that_fits_impl<detail::size_that_fits_int(bits)>
   {
      static_assert(std::is_same<typename uint_that_fits<bits>::type, void>::value,
         "Error: No int type fits specified number of bits."
      );
   };

   ////////////////////////////////////////////////////////////////////////////
   // Constants
	////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct int_traits;

   template <>
   struct int_traits<uint32_t>
   {
      static constexpr uint32_t max = 4294967295;
      static constexpr uint32_t min = 0;
   };

   template <>
   struct int_traits<int32_t>
   {
      static constexpr int32_t max = 2147483647;
      static constexpr int32_t min = -2147483648;
   };

   template <>
   struct int_traits<uint16_t>
   {
      static constexpr uint16_t max = 65535;
      static constexpr uint16_t min = 0;
   };

   template <>
   struct int_traits<int16_t>
   {
      static constexpr int16_t max = 32767;
      static constexpr int16_t min = -32768;
   };

   template <>
   struct int_traits<uint8_t>
   {
      static constexpr uint8_t max = 255;
      static constexpr uint8_t min = 0;
   };

   template <>
   struct int_traits<int8_t>
   {
      static constexpr uint8_t max = 127;
      static constexpr uint8_t min = -128;
   };

   template <typename T>
   constexpr T int_max()
   {
      return int_traits<T>::max;
   }

   template <typename T>
   constexpr T int_min()
   {
      return int_traits<T>::min;
   }

   ////////////////////////////////////////////////////////////////////////////
   // integer and binary functions
   ////////////////////////////////////////////////////////////////////////////
   constexpr int16_t promote(int8_t i)
   {
      return i;
   }

   constexpr uint16_t promote(uint8_t i)
   {
      return i;
   }

   constexpr int32_t promote(int16_t i)
   {
      return i;
   }

   constexpr uint32_t promote(uint16_t i)
   {
      return i;
   }

   constexpr int64_t promote(int32_t i)
   {
      return i;
   }

   constexpr uint64_t promote(uint32_t i)
   {
      return i;
   }

   constexpr float promote(float i)
   {
      return i;
   }

   constexpr double promote(double i)
   {
      return i;
   }

   template <typename T>
   constexpr T pow2(size_t n)
   {
      return (n == 0)? T(1) : T(2) * pow2<T>(n-1);
   }

   // This is needed to force compile-time evaluation
   template <typename T, size_t n>
   struct static_pow2
   {
      constexpr static T val = pow2<T>(n);
   };

   // smallest power of 2 that fits n
   template <typename T>
   constexpr T smallest_pow2(T n, T m = 1)
   {
      return (m < n)? smallest_pow2(n, m << 1) : m;
   }

   template <typename T>
   constexpr bool is_pow2(T n)
   {
      return (n & (n - 1)) == 0;
   }

   ////////////////////////////////////////////////////////////////////////////
   // basic math functions and constants
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // pi
   ////////////////////////////////////////////////////////////////////////////
   constexpr float pi = 3.1415926535897f;
   constexpr float _2pi = pi * 2.0f;

   ////////////////////////////////////////////////////////////////////////////
   // fast tan approximation (from http://www.musicdsp.org/)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_tan(float x)
   {
       float sqr = x * x;
       float r = 2.033e-01f;
       r *= sqr;
       r += 3.1755e-01f;
       r *= sqr;
       r += 1.0f;
       r *= x;
       return r;
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast exp approximation (from http://www.musicdsp.org/)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_exp(float x)
   {
      return
         (362880.f + x *
            (362880.f + x *
               (181440.f + x *
                  (60480.f + x *
                     (15120.f + x *
                        (3024.f + x *
                           (504.f + x *
                              (72.f + x *
                                 (9.f + x)
         )))))))) * 2.75573192e-6f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // linear interpolation: Interpolates a value linearly between y1 and y2
   // given mu. If mu is 0, the result is y1. If mu is 1, then the result is
   // y2.
   ////////////////////////////////////////////////////////////////////////////
   constexpr float linear_interpolate(float y1, float y2, float mu)
   {
      return y1 + mu * (y2 - y1);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast reciprocal. See http://tinyurl.com/lgmnsyg. We want to avoid
   // multiplication. So instead of 1.0f/val, we use this function which
   // is a inaccurate, but fast substitute. It works by negating the exponent
   // which is assumed to be IEEE754.
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_inverse(float val)
   {
      auto x = reinterpret_cast<std::int32_t&>(val);
      x = 0x7EF311C2 - x;
      return reinterpret_cast<float&>(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast division using multiplication and fast_inverse
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_div(float a, float b)
   {
      return a * fast_inverse(b);
   }
}}

#endif
