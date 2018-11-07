/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   struct decibel
   {
      constexpr decibel(double val) : val(val) {}

      operator double() const                { return std::pow(10.0, val/20.0); }
      constexpr decibel operator-() const    { return {-val}; }

      double val = 0.0f;
   };

   inline decibel to_db(double val)
   {
      return 20 * std::log10(val);
   }

   // $$$ TODO: Use fast approximate versions of std::log10 and std::pow $$$
}}

#endif
