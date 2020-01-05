/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LITERALS_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_LITERALS_HPP_FEBRUARY_21_2018

#include <cstdint>
#include <cmath>
#include <q/support/base.hpp>
#include <q/support/frequency.hpp>
#include <q/support/decibel.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   inline namespace literals
   {
      constexpr frequency operator "" _Hz(long double val)
      {
         return { double(val) };
      }

      constexpr frequency operator "" _Hz(unsigned long long int val)
      {
         return { double(val) };
      }

      constexpr frequency operator "" _KHz(long double val)
      {
         return { double(val * 1e3) };
      }

      constexpr frequency operator "" _KHz(unsigned long long int val)
      {
         return { double(val * 1e3) };
      }

      constexpr frequency operator "" _kHz(long double val)
      {
         return { double(val * 1e3) };
      }

      constexpr frequency operator "" _kHz(unsigned long long int val)
      {
         return { double(val * 1e3) };
      }

      constexpr frequency operator "" _MHz(long double val)
      {
         return { double(val * 1e6) };
      }

      constexpr frequency operator "" _MHz(unsigned long long int val)
      {
         return { double(val * 1e6) };
      }

      constexpr duration operator "" _s(long double val)
      {
         return { double(val) };
      }

      constexpr duration operator "" _s(unsigned long long int val)
      {
         return { double(val) };
      }

      constexpr duration operator "" _ms(long double val)
      {
         return { double(val * 1e-3) };
      }

      constexpr duration operator "" _ms(unsigned long long int val)
      {
         return { double(val * 1e-3) };
      }

      constexpr duration operator "" _us(long double val)
      {
         return { double(val * 1e-6) };
      }

      constexpr duration operator "" _us(unsigned long long int val)
      {
         return { double(val * 1e-6) };
      }

      constexpr decibel operator "" _dB(unsigned long long int val)
      {
         return { double(val), decibel::direct };
      }

      constexpr decibel operator "" _dB(long double val)
      {
         return { double(val), decibel::direct };
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
}

#endif
