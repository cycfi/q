/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOISE_GATE_HPP_JANUARY_17_2021)
#define CYCFI_Q_NOISE_GATE_HPP_JANUARY_17_2021

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/differentiator.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Noise_gate with an onset_threshold, a release_threshold.
   //
   // Constructor examples :
   //                onset_threshold   release_threshold
   // noise_gate     _gate{ -33_dB,    -45_dB };
   // noise_gate     _gate{            -45_dB };
   //
   // To process new sample s : calculate envelope, then calculate gate
   //
   // auto envelope = _peak_envelope_follower(std::abs(s));
   // auto gate = _gate(envelope);
   //
   // _gate(envelope) returns true (if it's open) or false (if it's closed).
   //
   // On onsets, the noise_gate opens up if the signal envelope goes above
   // the specified onset threshold (defaults to +12dB above
   // release_threshold).
   //
   // On note release, the noise_gate turns off if the signal goes below a
   // specified absolute release threshold.
   ////////////////////////////////////////////////////////////////////////////
   struct noise_gate
   {
                  noise_gate(decibel onset_threshold, decibel release_threshold);
                  noise_gate(decibel release_threshold);

      bool        operator()(float env);
      bool        operator()() const;

      void        onset_threshold(decibel onset_threshold);
      void        onset_threshold(float onset_threshold);
      void        release_threshold(decibel release_threshold);
      void        release_threshold(float release_threshold);

      float       onset_threshold() const;
      float       release_threshold() const;

   protected:

      bool        _state = 0;
      float       _onset_threshold;
      float       _release_threshold;
   };

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
   inline noise_gate::noise_gate(
      decibel onset_threshold
    , decibel release_threshold
   )
    : _release_threshold{lin_float(release_threshold)}
    , _onset_threshold{lin_float(onset_threshold)}
   {
   }

   inline noise_gate::noise_gate(decibel release_threshold)
    : _release_threshold{lin_float(release_threshold)}
    , _onset_threshold{lin_float(release_threshold + 12_dB)}
   {}

   inline bool noise_gate::operator()(float env)
   {
      if (!_state && env > _onset_threshold)
         _state = 1;
      else if (_state && env < _release_threshold)
         _state = 0;
      return _state;
   }

   inline bool noise_gate::operator()() const
   {
      return _state;
   }

   inline void noise_gate::onset_threshold(decibel onset_threshold)
   {
      _onset_threshold = lin_float(onset_threshold);
   }

   inline void noise_gate::release_threshold(decibel release_threshold)
   {
      _release_threshold = lin_float(release_threshold);
   }

   inline void noise_gate::onset_threshold(float onset_threshold)
   {
      _onset_threshold = onset_threshold;
   }

   inline void noise_gate::release_threshold(float release_threshold)
   {
      _release_threshold = release_threshold;
   }

   inline float noise_gate::onset_threshold() const
   {
      return _onset_threshold;
   }

   inline float noise_gate::release_threshold() const
   {
      return _release_threshold;
   }

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
