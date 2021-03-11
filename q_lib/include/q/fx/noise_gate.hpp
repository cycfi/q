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
                              noise_gate(
                                 decibel onset_threshold
                               , decibel release_threshold
                               , std::uint32_t sps
                              );

                              noise_gate(
                                 decibel release_threshold
                               , std::uint32_t sps);

      bool                    operator()(float s, float env);
      bool                    operator()() const;

      void                    onset_threshold(decibel onset_threshold);
      void                    onset_threshold(float onset_threshold);
      void                    release_threshold(decibel release_threshold);
      void                    release_threshold(float release_threshold);

      float                   onset_threshold() const;
      float                   release_threshold() const;

   private:

      bool                    _state = 0;
      float                   _onset_threshold;
      float                   _release_threshold;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline noise_gate::noise_gate(
      decibel onset_threshold
    , decibel release_threshold
    , std::uint32_t sps
   )
    : _release_threshold{float(release_threshold)}
    , _onset_threshold{float(onset_threshold)}
   {
   }

   inline noise_gate::noise_gate(
      decibel release_threshold
    , std::uint32_t sps
   )
    : _release_threshold{float(release_threshold)}
    , _onset_threshold{float(release_threshold + 12_dB)}
   {}

   inline bool noise_gate::operator()(float s, float env)
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
      _onset_threshold = float(onset_threshold);
   }

   inline void noise_gate::release_threshold(decibel release_threshold)
   {
      _release_threshold = float(release_threshold);
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
}

#endif
