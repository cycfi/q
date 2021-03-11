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
   // On onsets, the noise_gate opens up if the signal envelope goes above
   // the specified onset threshold (defaults to +12dB above
   // release_threshold).
   ////////////////////////////////////////////////////////////////////////////
   class noise_gate
   {
   public:

      static constexpr auto default_release_threshold = -36_dB;

      noise_gate(decibel onset_threshold, decibel release_threshold, std::uint32_t sps)
       : _release_threshold{float(release_threshold)}
       , _onset_threshold{float(onset_threshold)}
      {
      }

      noise_gate(decibel release_threshold, std::uint32_t sps)
       : _release_threshold{float(release_threshold)}
       , _onset_threshold{float(release_threshold + 12_dB)}
      {}

      noise_gate(std::uint32_t sps)
       : noise_gate{default_release_threshold, sps}
      {}

      bool operator()(float s, float env)
      {
         if (!_state && env > _onset_threshold)
            _state = 1;
         else if (_state && env < _release_threshold)
            _state = 0;

         return _state;
      }

      bool operator()() const
      {
         return _state;
      }

      void onset_threshold(decibel onset_threshold)
      {
         _onset_threshold = float(onset_threshold);
      }

      void release_threshold(decibel release_threshold)
      {
         _release_threshold = float(release_threshold);
      }

      float onset_threshold() const    { return _onset_threshold; }
      float release_threshold() const  { return _release_threshold; }

   private:

      bool                    _state = 0;
      float                   _onset_threshold;
      float                   _release_threshold;
   };
}

#endif
