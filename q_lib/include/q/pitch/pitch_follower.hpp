/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_FOLLOWER_HPP_NOVEMBER_18_2018)
#define CYCFI_Q_PITCH_FOLLOWER_HPP_NOVEMBER_18_2018

#include <q/support/literals.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi { namespace q
{
   using namespace q::literals;

   ////////////////////////////////////////////////////////////////////////////
   // pitch_follower
   ////////////////////////////////////////////////////////////////////////////
   class pitch_follower
   {
   public:

      struct config
      {
         // Compressor
         duration             comp_release            = 30_ms;
         decibel              comp_threshold          = -18_dB;
         decibel              comp_width              = 3_dB;
         double               comp_slope              = 1.0/4;
         double               comp_gain               = 4;

         // Gate
         decibel              gate_on_threshold       = -28_dB;
         decibel              gate_off_threshold      = -60_dB;
         decibel              note_hold_threshold     = -28_dB;

         // Attack / Decay
         duration             attack                  = 100_ms;
         duration             decay                   = 300_ms;
         duration             release                 = 300_ms;
         decibel              release_threshold       = -36_dB;
      };

                              pitch_follower(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel hysteresis = -30_dB
                              );

                              pitch_follower(
                                 config const& conf
                               , frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel hysteresis = -30_dB
                              );

      float                   operator()(float s);
      float                   envelope() const           { return _synth_env_val; }
      float                   get_frequency() const      { return _frequency; }
      float                   signal_envelope() const    { return _fast_env(); }

   private:

      peak_envelope_follower  _env;
      fast_envelope_follower  _fast_env;
      envelope_shaper         _synth_env;
      soft_knee_compressor    _comp;
      window_comparator       _gate;
      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
      pitch_detector          _pd;

      float                   _makeup_gain;
      float                   _synth_env_val;
      float                   _default_frequency;
      float                   _frequency = 0.0f;
      float                   _stable_frequency = 0.0f;
      bool                    _release_edge = false;
      float                   _note_hold_threshold;
   };

   ////////////////////////////////////////////////////////////////////////////
   // implementation
   ////////////////////////////////////////////////////////////////////////////
   inline pitch_follower::pitch_follower(
      config const& conf
    , q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
    , decibel hysteresis
   )
    : _env(conf.comp_release, sps)
    , _fast_env(lowest_freq.period(), sps)
    , _synth_env(conf.attack, conf.decay, conf.release, conf.release_threshold, sps)
    , _comp(conf.comp_threshold, conf.comp_width, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _pd(lowest_freq, highest_freq, sps, hysteresis)
    , _lp1(highest_freq, sps)
    , _lp2(lowest_freq, sps)
    , _makeup_gain(conf.comp_gain)
    , _default_frequency(float(lowest_freq) * 2)
    , _note_hold_threshold(conf.note_hold_threshold)
   {}

   inline pitch_follower::pitch_follower(
      q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
    , decibel hysteresis
   )
    : pitch_follower(config{}, lowest_freq, highest_freq, sps, hysteresis)
   {}

   inline float pitch_follower::operator()(float s)
   {
      // Bandpass filter
      s = _lp1(s);
      s -= _lp2(s);

      // Main envelope
      auto env = _env(std::abs(s));

      // Noise gate
      if (_gate(env))
      {
         // Compressor + makeup-gain + hard clip
         constexpr clip _clip;
         auto gain = float(_comp(env)) * _makeup_gain;
         s = _clip(s * gain);
      }
      else
      {
         s = 0.0f;
      }

      // Pitch detection
      bool pd_ready = _pd(s);

      // Fast envelope
      auto prev = _fast_env();
      auto fast_env = _fast_env(std::abs(s));

      if (_gate())
      {
         bool hold_note = fast_env > _note_hold_threshold;

         // Get the estimated frequency
         auto f_ = hold_note? _pd.get_frequency() : 0.0f;

         // On rising envelope, if we do not have a viable autocorelation
         // result yet, attempt to predict the frequency (via period counting
         // using zero-crossing)
         if (fast_env >= prev && hold_note)
         {
            if (f_ == 0.0f)
               f_ = _pd.predict_frequency();
            if (f_ != 0.0f)
               _frequency = f_;
            else
               _frequency = _default_frequency;
         }

         // On falling envelope, disregard result if there is a sudden drop
         // (> 3dB) in the envelope, possibly due to palm mute or similar or
         // if we have a low periodicity.
         else if (_pd.periodicity() >= 0.8 && prev < (fast_env * 1.5))
         {
            if (f_ != 0.0f)
               _frequency = f_;
         }

         // Otherwise, get the latest stable frequency
         else if (_stable_frequency != 0.0f)
         {
            _frequency = _stable_frequency;
         }
         _release_edge = true;
      }
      else
      {
         if (_release_edge)
         {
            if (_stable_frequency != 0.0f)
               _frequency = _stable_frequency;
            _release_edge = false;
         }
         _stable_frequency = 0.0f;
      }

      // Get the latest stable frequency
      if (pd_ready && _pd.periodicity() > 0.99)
         _stable_frequency = _frequency;

      // Synthesize an envelope
      _synth_env_val = _synth_env(fast_env);

      return s;
   }
}}

#endif

