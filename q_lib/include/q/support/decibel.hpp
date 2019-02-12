/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <q/support/base.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   struct decibel
   {
      struct _direct {};
      constexpr static _direct direct = {};

      decibel() : val(0.0f) {}
      decibel(double val);
      constexpr decibel(double val, _direct) : val(val) {}

      explicit operator double() const       { return fast_pow10(val/20.0); }
      explicit operator float() const        { return fast_pow10(val/20.0); }
      constexpr decibel operator-() const    { return { -val, direct }; }

      double val = 0.0f;
   };

   inline decibel::decibel(double val)
    : val(20.0f * fast_log10(val))
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
}}

#endif
