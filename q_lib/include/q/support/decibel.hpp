/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <q/detail/db_table.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // decibel is a highly optimized class for dealing with decibels. The
   // class provides fast conversion from linear to decibel and back. The
   // decibel value is non-linear. It operates on the logarithmic domain. The
   // decibel class is perfectly suitable for dynamics processing (e.g.
   // compressors and limiters and envelopes). Q provides fast decibel
   // computations using lookup tables and fast math computations for
   // converting to and from scalars.
   ////////////////////////////////////////////////////////////////////////////
   struct decibel
   {
      struct _direct {};
      constexpr static _direct direct = {};

      constexpr            decibel() : val(0.0f) {}
      explicit             decibel(double val);
      constexpr            decibel(double val, _direct) : val(val) {}

      [[deprecated("Use as_double(db) instead of double(db)")]]
      constexpr explicit   operator double() const;

      [[deprecated("Use as_float(db) instead of float(db)")]]
      constexpr explicit   operator float() const;

      constexpr decibel    operator+() const          { return { val, direct }; }
      constexpr decibel    operator-() const          { return { -val, direct }; }

      constexpr decibel&   operator+=(decibel b)      { val += b.val; return *this; }
      constexpr decibel&   operator-=(decibel b)      { val -= b.val; return *this; }
      constexpr decibel&   operator*=(decibel b)      { val *= b.val; return *this; }
      constexpr decibel&   operator/=(decibel b)      { val /= b.val; return *this; }

      double val = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   constexpr double as_double(decibel db)
   {
      return detail::db2a(db.val);
   }

   constexpr float as_float(decibel db)
   {
      return detail::db2a(db.val);
   }

   inline decibel approx_db(float val)
   {
      return decibel{20.0f*faster_log10(val), decibel::direct};
   }

   inline decibel::decibel(double val)
    : val(20.0f*fast_log10(val))
   {}

   constexpr decibel::operator double() const
   {
      return as_double(*this);
   }

   constexpr decibel::operator float() const
   {
      return as_float(*this);
   }

   constexpr decibel operator-(decibel a, decibel b)
   {
      return decibel{a.val - b.val, decibel::direct};
   }

   constexpr decibel operator+(decibel a, decibel b)
   {
      return decibel{a.val + b.val, decibel::direct};
   }

   constexpr decibel operator*(decibel a, decibel b)
   {
      return decibel{a.val * b.val, decibel::direct};
   }

   constexpr decibel operator*(decibel a, double b)
   {
      return decibel{a.val * b, decibel::direct};
   }

   constexpr decibel operator*(decibel a, float b)
   {
      return decibel{a.val * b, decibel::direct};
   }

   constexpr decibel operator*(decibel a, int b)
   {
      return decibel{a.val * b, decibel::direct};
   }

   constexpr decibel operator*(double a, decibel b)
   {
      return decibel{a * b.val, decibel::direct};
   }

   constexpr decibel operator*(float a, decibel b)
   {
      return decibel{a * b.val, decibel::direct};
   }

   constexpr decibel operator*(int a, decibel b)
   {
      return decibel{a * b.val, decibel::direct};
   }

   inline decibel operator/(decibel a, decibel b)
   {
      return decibel{a.val / b.val, decibel::direct};
   }

   inline decibel operator/(decibel a, double b)
   {
      return decibel{a.val / b, decibel::direct};
   }

   inline decibel operator/(decibel a, float b)
   {
      return decibel{a.val / b, decibel::direct};
   }

   inline decibel operator/(decibel a, int b)
   {
      return decibel{a.val / b, decibel::direct};
   }

   constexpr bool operator==(decibel a, decibel b)
   {
      return a.val == b.val;
   }

   constexpr bool operator!=(decibel a, decibel b)
   {
      return a.val != b.val;
   }

   constexpr bool operator<(decibel a, decibel b)
   {
      return a.val < b.val;
   }

   constexpr bool operator<=(decibel a, decibel b)
   {
      return a.val <= b.val;
   }

   constexpr bool operator>(decibel a, decibel b)
   {
      return a.val > b.val;
   }

   constexpr bool operator>=(decibel a, decibel b)
   {
      return a.val >= b.val;
   }
}

#endif
