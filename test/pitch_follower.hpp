/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/pitch_detector.hpp>
#include <q/sfx.hpp>
#include <q/notes.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   struct pitch_follower
   {
      struct config
      {
                              config(
                                 frequency lowest_freq
                               , frequency highest_freq);

         // Pitch detector
         frequency            lowest_freq;
         frequency            highest_freq;
         float                threshold            = 0.001;

         // Envelope follower
         duration             release              = 1_s;

         // Compressor
         float                comp_threshold       = 0.5f;
         float                comp_slope           = 1.0f/20;

         // Noise-gate
         float                gate_on_threshold    = -36_dB;
         float                gate_off_threshold   = -60_dB;
      };

                              pitch_follower(config const& conf, std::uint32_t sps);

      bool                    operator()(float s);
      float                   audio() const;
      float                   frequency() const;
      float                   predict_frequency() const;
      bool                    gate() const;
      bool                    is_note_onset() const;

      pitch_detector<>        _pd;
      peak_envelope_follower  _env;
      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
      compressor_expander     _comp;
      window_comparator       _gate;
      float                   _makeup_gain;
      float                   _val = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   inline pitch_follower::config::config(
      q::frequency lowest_freq, q::frequency highest_freq
   )
    : lowest_freq(lowest_freq)
    , highest_freq(highest_freq)
   {}

   inline pitch_follower::pitch_follower(config const& conf, std::uint32_t sps)
    : _pd(conf.lowest_freq, conf.highest_freq, sps, conf.threshold)
    , _env(conf.release, sps)
    , _lp1(conf.highest_freq, sps)
    , _lp2(conf.lowest_freq, sps)
    , _comp(conf.comp_threshold, conf.comp_slope)
    , _gate(conf.gate_off_threshold, conf.gate_on_threshold)
    , _makeup_gain(1.0f/conf.comp_slope)
   {}

   inline bool pitch_follower::operator()(float s)
   {
      // Bandpass filter
      s = _lp1(s);
      s -= _lp2(s);

      // Envelope
      auto env = _env(std::abs(s));

      // Noise gate
      if (_gate(env))
      {
         // Compressor + makeup-gain + hard clip
         constexpr clip _clip;
         _val = _clip(_comp(s, env) * _makeup_gain);
      }
      else
      {
         _val = 0.0f;
      }

      // Pitch Detect
      return _pd(_val);
   }

   inline float pitch_follower::audio() const
   {
      return _val;
   }

   inline float pitch_follower::frequency() const
   {
      return _pd.frequency();
   }

   inline float pitch_follower::predict_frequency() const
   {
      return _pd.predict_frequency();
   }

   inline bool pitch_follower::gate() const
   {
      return _gate();
   }

   inline bool pitch_follower::is_note_onset() const
   {
      return _pd.is_note_onset();
   }
}}

