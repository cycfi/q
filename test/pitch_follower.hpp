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
         float                gate_on_threshold    = 0.005;
         float                gate_off_threshold   = 0.001;
      };

                              pitch_follower(config const& conf, std::uint32_t sps);

      bool                    operator()(float s);
      bool                    operator()(float s, std::size_t& extra);

      pitch_detector<>        _pd;
      peak_envelope_follower  _env;
      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
      compressor_expander     _comp;
      float                   _makeup_gain;
      float const             _gate_on_threshold;
      float const             _gate_off_threshold;
      float                   _gate_threshold;
   };

   ////////////////////////////////////////////////////////////////////////////
   inline pitch_follower::config::config(
      frequency lowest_freq, frequency highest_freq
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
    , _makeup_gain(1.0f/conf.comp_slope)
    , _gate_on_threshold(conf.gate_on_threshold)
    , _gate_off_threshold(conf.gate_off_threshold)
    , _gate_threshold(conf.gate_on_threshold)
   {}

   inline bool pitch_follower::operator()(float s, std::size_t& extra)
   {
      // Bandpass filter
      s = _lp1(s);
      s -= _lp2(s);

      // Envelope
      auto env = _env(std::abs(s));

      // Noise gate, note-on, note-off
      if (env > _gate_threshold)
      {
         // Compressor + makeup-gain + hard clip
         constexpr clip _clip;
         s = _clip(_comp(s, env) * _makeup_gain);
         _gate_threshold = _gate_off_threshold;
      }
      else
      {
         s = 0.0f;
         _gate_threshold = _gate_on_threshold;
      }

      // Pitch Detect
      return _pd(s, extra);
   }

   inline bool pitch_follower::operator()(float s)
   {
      std::size_t extra;
      return (*this)(s, extra);
   }
}}

