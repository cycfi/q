/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_FOLLOWER_HPP_NOVEMBER_18_2018)
#define CYCFI_Q_PITCH_FOLLOWER_HPP_NOVEMBER_18_2018

#include <q/support/literals.hpp>
#include <q/pitch_detector.hpp>
#include <q/fx.hpp>

namespace cycfi { namespace q
{
   using namespace q::literals;

   ////////////////////////////////////////////////////////////////////////////
   // pitch_follower
   ////////////////////////////////////////////////////////////////////////////
   class pitch_follower
   {
   public:

      static constexpr float hysteresis = 0.0001; // -80dB

      struct config
      {
         // Envelope Follower
         duration             env_hold             = 10_ms;

         // Compressor
         duration             comp_release         = 30_ms;
         decibel              comp_threshold       = -18_dB;
         decibel              comp_width           = 3_dB;
         double               comp_slope           = 1.0/4;
         double               comp_gain            = 4;

         // Gate
         decibel              gate_on_threshold    = -36_dB;
         decibel              gate_off_threshold   = -60_dB;

         // Attack / Decay
         duration             attack               = 100_ms;
         duration             decay                = 300_ms;
         duration             release              = 100_ms;
         decibel              release_threshold    = -40_dB;
      };

                              pitch_follower(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , float threshold = 0.001
                              );

                              pitch_follower(
                                 config const& conf
                               , frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , float threshold = 0.001
                              );

      float                   operator()(float s);
      float                   envelope() const     { return _synth_env_val; }
      bool                    is_playing() const   { return _is_playing; }
      float                   frequency() const    { return _freq; }

   private:

      peak_envelope_follower  _env;
      fast_envelope_follower  _fast_env;
      envelope_shaper         _synth_env;
      soft_knee_compressor    _comp;
      window_comparator       _gate;
      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
      pitch_detector<>        _pd;

      float                   _makeup_gain;
      bool                    _is_playing = false;
      float                   _synth_env_val;
      float                   _freq = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // implementation
   ////////////////////////////////////////////////////////////////////////////
   inline pitch_follower::pitch_follower(
      config const& conf
    , q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
    , float threshold
   )
    : _env(conf.comp_release, sps)
    , _fast_env(conf.env_hold, sps)
    , _synth_env(conf.attack, conf.decay, conf.release, conf.release_threshold, sps)
    , _comp(conf.comp_threshold, conf.comp_width, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _pd(lowest_freq, highest_freq, sps, threshold)
    , _lp1(highest_freq, sps)
    , _lp2(lowest_freq, sps)
    , _makeup_gain(conf.comp_gain)
   {}

   inline pitch_follower::pitch_follower(
      q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
    , float threshold
   )
    : pitch_follower(config{}, lowest_freq, highest_freq, sps, threshold)
   {}

   inline float pitch_follower::operator()(float s)
   {
      // Main envelope
      auto env = _env(std::abs(s));

      // Noise gate
      if (_gate(env))
      {
         _is_playing = true;

         // Compressor + makeup-gain + hard clip
         constexpr clip _clip;
         auto gain = float(_comp(env)) * _makeup_gain;
         s = _clip(s * gain);
      }
      else
      {
         s = 0.0f;
         _is_playing = false;
      }

      // Pitch detection
      s = _lp1(s);
      s -= _lp2(s);
      _pd(s);

      if (_is_playing)
      {
         // Set frequency
         auto f_ = _pd.frequency();
         if (f_ == 0.0f)
            f_ = _pd.predict_frequency();
         if (f_ != 0.0f)
            _freq = f_;
      }

      // Synthesize an envelope
      auto synth_env = _fast_env(std::abs(s));
      _synth_env_val = _synth_env(synth_env);

      return s;
   }
}}

#endif

