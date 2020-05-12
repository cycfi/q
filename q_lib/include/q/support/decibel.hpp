/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <q/detail/db_table.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   struct decibel
   {
      struct _direct {};
      constexpr static _direct direct = {};

      constexpr decibel() : val(0.0f) {}
      constexpr decibel(double val);
      constexpr decibel(double val, _direct) : val(val) {}

      constexpr explicit operator double() const   { return detail::db2a(val); }
      constexpr explicit operator float() const    { return detail::db2a(val); }
      constexpr decibel operator-() const          { return { -val, direct }; }

      constexpr decibel& operator+=(decibel b)     { val += b.val; return *this; }
      constexpr decibel& operator-=(decibel b)     { val -= b.val; return *this; }
      constexpr decibel& operator*=(decibel b)     { val *= b.val; return *this; }
      constexpr decibel& operator/=(decibel b)     { val /= b.val; return *this; }

      double val = 0.0f;
   };

   constexpr decibel::decibel(double val)
    : val(detail::a2db(val))
   {}

   constexpr decibel operator-(decibel a, decibel b)
   {
      return decibel{ a.val - b.val, decibel::direct };
   }

   constexpr decibel operator+(decibel a, decibel b)
   {
      return decibel{ a.val + b.val, decibel::direct };
   }

   constexpr decibel operator*(decibel a, decibel b)
   {
      return decibel{ a.val * b.val, decibel::direct };
   }

   constexpr decibel operator*(decibel a, double b)
   {
      return decibel{ a.val * b, decibel::direct };
   }

   constexpr decibel operator*(decibel a, float b)
   {
      return decibel{ a.val * b, decibel::direct };
   }

   constexpr decibel operator*(double a, decibel b)
   {
      return decibel{ a * b.val, decibel::direct };
   }

   constexpr decibel operator*(float a, decibel b)
   {
      return decibel{ a * b.val, decibel::direct };
   }

   inline decibel operator/(decibel a, decibel b)
   {
      return decibel{ fast_div(a.val, b.val), decibel::direct };
   }

   inline decibel operator/(decibel a, double b)
   {
      return decibel{ fast_div(a.val, b), decibel::direct };
   }

   inline decibel operator/(decibel a, float b)
   {
      return decibel{ fast_div(a.val, b), decibel::direct };
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
