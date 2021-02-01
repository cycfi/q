/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOISE_GATE_HPP_JANUARY_17_2021)
#define CYCFI_Q_NOISE_GATE_HPP_JANUARY_17_2021

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/special.hpp>
#include <q/fx/envelope.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // noise_gate gates noise below a specified threshold.
   //
   // On note release, the noise_gate turns off if the signal goes below a
   // specified absolute release threshold.
   //
   // On onsets, the noise_gate opens up if the signal envelope goes 12_dB
   // above the specified release threshold. In addition, the noise_gate may
   // also open up on possibly lower-level but fast moving signals. This
   // configuration makes the noise_gate less susceptible to triggering on
   // low-level slow-moving signals.
   //
   // Fast moving signals are detected by obtaining the 16 point moving sum
   // of the differentated peak envelope, triggering on thresholds above
   // -40_dB, which is chosen to be far above the noise floor.
   ////////////////////////////////////////////////////////////////////////////
   struct noise_gate
   {
      static constexpr auto env_release = 50_ms;
      static constexpr auto diff_threshold = float(-40_dB);
      static constexpr auto default_release_threshold = -36_dB;

      noise_gate(decibel release_threshold, std::uint32_t sps)
       : _env{env_release, sps}
       , _release_threshold{float(release_threshold)}
       , _onset_threshold{float(release_threshold + 12_dB)}
      {}

      noise_gate(std::uint32_t sps)
       : noise_gate{default_release_threshold, sps}
      {}

      bool operator()(float s)
      {
         auto env = _env(std::abs(s));
         auto diff = _diff(env);
         auto sum = _sum(diff);

         if (!_state && (sum > diff_threshold || env > _onset_threshold))
            _state = 1;
         else if (_state && env < _release_threshold)
            _state = 0;

         return _state;
      }

      bool operator()() const
      {
         return _state;
      }

      void set_release_threshold(decibel release_threshold)
      {
         _onset_threshold = float(release_threshold + 12_dB);
         _release_threshold = float(release_threshold);
      }

      float env() const
      {
         return _env();
      }

      peak_envelope_follower  _env;
      differentiator          _diff;
      moving_sum              _sum{16};
      bool                    _state = 0;
      float                   _onset_threshold;
      float                   _release_threshold;
   };
}

#endif
