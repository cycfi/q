/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LITERALS_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_LITERALS_HPP_FEBRUARY_21_2018

#include <cstdint>
#include <cmath>
#include <q/support.hpp>

namespace cycfi { namespace q
{
   struct duration;
   struct frequency;
   struct period;

   ////////////////////////////////////////////////////////////////////////////
   struct frequency
   {
      constexpr frequency(double val) : val(val) {}
      constexpr frequency(duration d);
      explicit constexpr operator double() const { return val; }
      constexpr q::period period() const;
      double val = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct duration
   {
      constexpr duration(double val) : val(val) {}
      explicit constexpr operator double() const { return val; }
      double val = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct period : duration
   {
      using duration::duration;
      constexpr period(duration d) : duration(d) {}
      constexpr period(frequency f) : duration(1.0 / f.val) {}
   };

   ////////////////////////////////////////////////////////////////////////////
   constexpr frequency::frequency(duration d)
    : val(1.0 / d.val) {}

   constexpr q::period frequency::period() const
   {
      return 1.0 / val;
   }

   ////////////////////////////////////////////////////////////////////////////
   namespace literals
   {
      constexpr frequency operator "" _Hz(long double val)
      {
         return {double(val)};
      }

      constexpr frequency operator "" _Hz(unsigned long long int val)
      {
         return {double(val)};
      }

      constexpr frequency operator "" _KHz(long double val)
      {
         return {double(val * 1e3)};
      }

      constexpr frequency operator "" _KHz(unsigned long long int val)
      {
         return {double(val * 1e3)};
      }

      constexpr frequency operator "" _kHz(long double val)
      {
         return {double(val * 1e3)};
      }

      constexpr frequency operator "" _kHz(unsigned long long int val)
      {
         return {double(val * 1e3)};
      }

      constexpr frequency operator "" _MHz(long double val)
      {
         return {double(val * 1e6)};
      }

      constexpr frequency operator "" _MHz(unsigned long long int val)
      {
         return {double(val * 1e6)};
      }

      constexpr duration operator "" _s(long double val)
      {
         return {double(val)};
      }

      constexpr duration operator "" _s(unsigned long long int val)
      {
         return {double(val)};
      }

      constexpr duration operator "" _ms(long double val)
      {
         return {double(val * 1e-3)};
      }

      constexpr duration operator "" _ms(unsigned long long int val)
      {
         return {double(val * 1e-3)};
      }

      constexpr duration operator "" _us(long double val)
      {
         return {double(val * 1e-6)};
      }

      constexpr duration operator "" _us(unsigned long long int val)
      {
         return {double(val * 1e-6)};
      }

      float operator "" _dB(long double val)
      {
         return std::pow(10.0, val/20.0);
      }

      constexpr long double operator "" _pi(long double val)
      {
         return val * pi;
      }

      constexpr long double operator "" _pi(unsigned long long int val)
      {
         return val * pi;
      }
   }
}}

#endif
