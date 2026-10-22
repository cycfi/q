/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DB_TABLE_HPP_SEPTEMBER_21_2019)
#define CYCFI_Q_DB_TABLE_HPP_SEPTEMBER_21_2019

#include <cstdint>
#include <cstring>
#include <cmath>
#include <type_traits>
#include <q/support/base.hpp>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // The dB <-> linear conversions split the number into a power of two
   // and a mantissa, which is what a float is made of:
   //
   //    10^(db/20) = 2^(db log2(10) / 20) = 2^i * 2^f,   0 <= f < 1
   //
   // The integer part i indexes a table of powers of two, the fraction f
   // a 256-entry table of 2^f read with linear interpolation. The other
   // way, log2(x) is the float's exponent field plus log2(1 + m) with m
   // the mantissa, from a 256-entry table of log2(1 + j/256) read the same
   // way; 20 log10(2) scales it to dB.
   //
   // Both are exact to about 1e-5 dB over the whole float range, and
   // the split is faster than a bit-trick exp or log on the serial
   // per-sample path. The tables are built at compile time, 3 KB in all.
   ////////////////////////////////////////////////////////////////////////////
   constexpr int db_table_bits = 8;
   constexpr int db_table_n = 1 << db_table_bits;

   struct db_tables
   {
      float    pow2_frac[db_table_n + 1];  // 2^(j/256)
      float    pow2_int[256];              // 2^(e-127), e the exponent field
      float    log2_frac[db_table_n + 1];  // log2(1 + j/256)

      constexpr db_tables()
       : pow2_frac{}, pow2_int{}, log2_frac{}
      {
         // 2^(1/256) and its powers, accumulated in double
         constexpr double root = 1.0027112750502024854;
         double v = 1.0;
         for (int j = 0; j != db_table_n; ++j)
         {
            pow2_frac[j] = float(v);
            v *= root;
         }
         pow2_frac[db_table_n] = 2.0f;

         // 2^(e-127), held at the ends to the normal range (no denormal,
         // no infinity) so a clamped index stays finite
         double p = 1.0;
         for (int k = 0; k != 126; ++k)
            p /= 2.0;
         pow2_int[0] = float(p);                  // 2^-126, for e = 0 too
         for (int e = 1; e != 255; ++e)
         {
            pow2_int[e] = float(p);               // 2^(e-127)
            p *= 2.0;
         }
         pow2_int[255] = pow2_int[254];           // 2^127, for e = 255 too

         // log2(1 + t) = 2 atanh(t / (2 + t)) / ln 2, the series in z =
         // t / (2 + t) <= 1/3 converging in a few terms
         constexpr double ln2 = 0.69314718055994530942;
         for (int j = 0; j <= db_table_n; ++j)
         {
            double t = double(j) / db_table_n;
            double z = t / (2.0 + t);
            double z2 = z * z;
            double term = z;
            double sum = z;
            for (int k = 1; k != 24; ++k)
            {
               term *= z2;
               sum += term / (2 * k + 1);
            }
            log2_frac[j] = float(2.0 * sum / ln2);
         }
      }
   };

   inline constexpr db_tables db_tbl{};

   ////////////////////////////////////////////////////////////////////////////
   // 10^(db/20) for constant evaluation: exp by series, in double.
   ////////////////////////////////////////////////////////////////////////////
   constexpr double constexpr_db2a(double db)
   {
      constexpr double ln2 = 0.69314718055994530942;
      double x = db * 0.11512925464970228420;      // ln(10) / 20
      int n = int(x / ln2);
      if (x < 0 && n * ln2 > x)
         --n;
      double r = x - n * ln2;                      // 0 <= r < ln 2
      double term = 1.0, sum = 1.0;
      for (int k = 1; k != 30; ++k)
      {
         term *= r / k;
         sum += term;
      }
      if (n > 0)
         for (int k = 0; k != n; ++k)
            sum *= 2.0;
      else
         for (int k = 0; k != -n; ++k)
            sum /= 2.0;
      return sum;
   }

   ////////////////////////////////////////////////////////////////////////////
   // db2a: dB to linear. Any float in; the result is held to the normal
   // float range (about -758 to +765 dB).
   ////////////////////////////////////////////////////////////////////////////
   constexpr float db2a(float db)
   {
      if (std::is_constant_evaluated())
         return float(constexpr_db2a(db));

      // y = db log2(10)/20 scaled by 256: its floor holds the power of two
      // in the high bits and the table index in the low eight; what is
      // left is the interpolation fraction.
      constexpr float k = 0.16609640474436811f * db_table_n;
      constexpr float lo = -127.0f * db_table_n;    // e = 0: 2^-126
      constexpr float hi = 127.0f * db_table_n - 1; // e = 253, m < 2: 2^127
      auto y = std::fmin(std::fmax(db * k, lo), hi);  // NaN lands on lo
      auto fl = std::floor(y);
      auto frac = y - fl;
      auto iy = int(fl);
      auto j = iy & (db_table_n - 1);
      auto e = (iy >> db_table_bits) + 127;
      auto v1 = db_tbl.pow2_frac[j];
      auto v2 = db_tbl.pow2_frac[j + 1];
      auto m = v1 + (v2 - v1) * frac;
      return m * db_tbl.pow2_int[e];
   }

   ////////////////////////////////////////////////////////////////////////////
   // a2db: linear to dB, of the magnitude. Zero reads about -765 dB;
   // denormals read as 2^-127 times their mantissa.
   ////////////////////////////////////////////////////////////////////////////
   inline float a2db(float x)
   {
      std::uint32_t b;
      std::memcpy(&b, &x, 4);
      b &= 0x7FFFFFFFu;
      auto e = int(b >> 23) - 127;
      auto j = (b >> (23 - db_table_bits)) & (db_table_n - 1);
      auto frac = float(b & 0x7FFFu) * (1.0f / 32768.0f);
      auto v1 = db_tbl.log2_frac[j];
      auto v2 = db_tbl.log2_frac[j + 1];
      auto l = v1 + (v2 - v1) * frac;
      return (float(e) + l) * 6.02059991327962f;   // 20 log10(2)
   }
}

#endif
