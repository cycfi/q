/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_FREQUENCY_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_FREQUENCY_HPP_FEBRUARY_21_2018

#include <cstdint>
#include <cmath>
#include <q/support.hpp>
#include <q/value.hpp>

namespace cycfi { namespace q
{
   struct duration;
   struct frequency;
   struct period;

   ////////////////////////////////////////////////////////////////////////////
   struct frequency : value<double, frequency>
   {
      using base_type = value<double, frequency>;
      using base_type::base_type;

      constexpr frequency(double val) : base_type(val) {}
      constexpr frequency(duration d);

      explicit constexpr operator double() const   { return val; }
      explicit constexpr operator float() const    { return val; }
      constexpr q::period period() const;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct duration : value<double, duration>
   {
      using base_type = value<double, duration>;
      using base_type::base_type;

      constexpr duration(double val) : base_type(val) {}

      explicit constexpr operator double() const   { return val; }
      explicit constexpr operator float() const    { return val; }
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
    : base_type(1.0 / d.val) {}

   constexpr q::period frequency::period() const
   {
      return 1.0 / val;
   }
}}

#endif
