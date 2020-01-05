/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FREQUENCY_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_FREQUENCY_HPP_FEBRUARY_21_2018

#include <cstdint>
#include <cmath>
#include <q/support/base.hpp>
#include <q/support/value.hpp>

#if !defined(Q_DONT_USE_THREADS)
#include <chrono>
#include <thread>
#endif

namespace cycfi::q
{
   struct duration;
   struct frequency;
   struct period;

   ////////////////////////////////////////////////////////////////////////////
   struct frequency : value<double, frequency>
   {
      using base_type = value<double, frequency>;
      using base_type::base_type;

      constexpr                     frequency(double val) : base_type(val) {}
      constexpr                     frequency(duration d);

      constexpr explicit operator   double() const   { return rep; }
      constexpr explicit operator   float() const    { return rep; }
      constexpr q::period           period() const;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct duration : value<double, duration>
   {
      using base_type = value<double, duration>;
      using base_type::base_type;

      constexpr                     duration(double val) : base_type(val) {}

      constexpr explicit operator   double() const   { return rep; }
      constexpr explicit operator   float() const    { return rep; }
   };

   ////////////////////////////////////////////////////////////////////////////
   struct period : duration
   {
      using duration::duration;

      constexpr                     period(duration d) : duration(d) {}
      constexpr                     period(frequency f) : duration(1.0 / f.rep) {}
   };

   ////////////////////////////////////////////////////////////////////////////
   constexpr frequency::frequency(duration d)
    : base_type(1.0 / d.rep)
   {}

   constexpr q::period frequency::period() const
   {
      return 1.0 / rep;
   }

   ////////////////////////////////////////////////////////////////////////////
#if !defined(Q_DONT_USE_THREADS)
   inline void sleep(duration t)
   {
      std::this_thread::sleep_for(std::chrono::duration<double>(double(t)));
   }
#endif

}

#endif
