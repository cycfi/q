/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DC_BLOCKER_HPP_DECEMBER_24_2015)
#define CYCFI_Q_DC_BLOCKER_HPP_DECEMBER_24_2015

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // DC blocker based on Julius O. Smith's document
   //
   // A smaller _pole value allows faster tracking of "wandering dc levels",
   // but at the cost of greater low-frequency attenuation.
   ////////////////////////////////////////////////////////////////////////////
   struct dc_block
   {
                  dc_block(frequency f, float sps);

      float       operator()(float s);
      float       operator()() const;
      dc_block&   operator=(float y_);
      void        cutoff(frequency f, float sps);

      float _pole;      // pole
      float x = 0.0f;   // delayed input sample
      float y = 0.0f;   // current value
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline dc_block::dc_block(frequency f, float sps)
      : _pole(1.0f - (2_pi * as_double(f) / sps))
   {}

   inline float dc_block::operator()(float s)
   {
      y = s - x + _pole * y;
      x = s;
      return y;
   }

   inline float dc_block::operator()() const
   {
      return y;
   }

   inline dc_block& dc_block::operator=(float y_)
   {
      y = y_;
      return *this;
   }

   inline void dc_block::cutoff(frequency f, float sps)
   {
      _pole = 1.0f - (2_pi * as_double(f) / sps);
   }

}

#endif
