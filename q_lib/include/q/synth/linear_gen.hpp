/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LINEAR_GEN_HPP_APRIL_29_2023)
#define CYCFI_Q_LINEAR_GEN_HPP_APRIL_29_2023

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Linear upward ramp generator.
   //
   // The linear upward ramp generator generates a linearly increasing
   // amplitude from 0.0 to 1.0 specified by the duration (width) and samples
   // per second (sps) parameters. This is essentially a basic straight ramp
   // from 0.0 to 1.0.
   ////////////////////////////////////////////////////////////////////////////
   struct lin_upward_ramp_gen
   {
      lin_upward_ramp_gen(duration width, float sps)
       : _rate{1.0f / (as_float(width) * sps)}
      {
      }

      float operator()()
      {
         _y += _rate;
         return _y;
      }

      void config(duration width, float sps)
      {
         _rate = 1.0f / (as_float(width) * sps);
      }

      void reset()
      {
         _y = 0;
      }

   private:

      float _rate;
      float _y = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Linear downward ramp generator.
   //
   // The inverse of the linear upward ramp generator. This is essentially a
   // basic straight ramp from 1.0 to 0.0.
   ////////////////////////////////////////////////////////////////////////////
   struct lin_downward_ramp_gen : lin_upward_ramp_gen
   {
      lin_downward_ramp_gen(duration width, float sps)
       : lin_upward_ramp_gen{width, sps}
      {
      }

      float operator()()
      {
         return 1.0f - lin_upward_ramp_gen::operator()();
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Hold line generator.
   //
   // An infinitely wide straight horizontal line at 1.0 (width and sps are
   // unused and ignored).
   ////////////////////////////////////////////////////////////////////////////
   struct hold_line_gen
   {
      hold_line_gen(duration width, float sps)
      {
      }

      float operator()()
      {
         return 1.0f;
      }

      void config(duration width, float sps)
      {
      }

      void reset()
      {
      }
   };
}

#endif
