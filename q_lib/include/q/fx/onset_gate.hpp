/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ONSET_GATE_HPP_JANUARY_17_2021)
#define CYCFI_Q_ONSET_GATE_HPP_JANUARY_17_2021

#include <q/fx/noise_gate.hpp>
#include <q/fx/differentiator.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // onset_gate is a specialization of noise_gate with provision for gating
   // out slow attacks.
   //
   // The constructor parameters `attack_width` and `sps` specify the slope's
   // delta time. The envelope's slope (see differentiator.hpp) must exceed
   // the required onset threshold. This prevents slow moving signals to pass
   // as valid onsets.
   ////////////////////////////////////////////////////////////////////////////
   struct onset_gate : noise_gate
   {
                  onset_gate(
                     decibel onset_threshold
                   , decibel release_threshold
                   , duration attack_width
                   , float sps
                  );

      using noise_gate::operator();

      bool        operator()(float env);

   private:

      slope       _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline onset_gate::onset_gate(
      decibel onset_threshold
    , decibel release_threshold
    , duration attack_width
    , float sps
   )
    : noise_gate{onset_threshold, release_threshold}
    , _slope{attack_width, sps}
   {
   }

   inline bool onset_gate::operator()(float env)
   {
      if (!_state && _slope(env) > _onset_threshold)
         _state = 1;
      else if (_state && env < _release_threshold)
         _state = 0;
      return _state;
   }
}

#endif
