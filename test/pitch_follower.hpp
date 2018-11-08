/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/pitch_detector.hpp>
#include <q/sfx.hpp>
#include <q/envelope.hpp>
#include <q/notes.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   struct pitch_follower
   {
                              pitch_follower(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , float threshold = 0.001
                              );

      float                   operator()(
                                 float s
                               , envelope_tracker& env_tracker
                               , envelope& env_gen
                              );

      pitch_detector<>        _pd;
      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
   };

   ////////////////////////////////////////////////////////////////////////////
   inline pitch_follower::pitch_follower(
      frequency lowest_freq
    , frequency highest_freq
    , std::uint32_t sps
    , float threshold
   )
    : _pd(lowest_freq, highest_freq, sps, threshold)
    , _lp1(highest_freq, sps)
    , _lp2(lowest_freq, sps)
   {}

   inline float pitch_follower::operator()(
      float s
    , envelope_tracker& env_tracker
    , envelope& env_gen
   )
   {
      // Track envelope
      s = env_tracker(s, env_gen);

      // Bandpass filter
      s = _lp1(s);
      s -= _lp2(s);

//      if (env_gen.state() == envelope::attack_state)
//         _pd.reset();

      // Detect Pitch
      _pd(s);

      // Legato
      if (_pd.is_note_onset())
         env_gen.legato();

      return s;
   }
}}

