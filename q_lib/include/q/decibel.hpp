/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <q/support.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   struct decibel
   {
      constexpr decibel(double val) : val(val) {}

      operator double() const                { return fast_pow10(val/20.0); }
      constexpr decibel operator-() const    { return { -val }; }

      double val = 0.0f;
   };

   inline decibel to_db(double val)
   {
      return 20.0f * fast_log10(val);
   }

   constexpr decibel operator-(decibel a, decibel b)
   {
      return { a.val - b.val };
   }

   constexpr decibel operator+(decibel a, decibel b)
   {
      return { a.val + b.val };
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

   // $$$ TODO: Use fast approximate versions of std::log10 and std::pow $$$
}}

#endif
