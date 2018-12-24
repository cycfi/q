/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_FOLLOWER_HPP_NOVEMBER_18_2018)
#define CYCFI_Q_PITCH_FOLLOWER_HPP_NOVEMBER_18_2018

#include <q/support/literals.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/low_pass.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/utility/ring_buffer.hpp>
#include <q/synth/sin.hpp>

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
         duration             env_hold                = 10_ms;

         // Compressor
         duration             comp_release            = 30_ms;
         decibel              comp_threshold          = -18_dB;
         decibel              comp_width              = 3_dB;
         double               comp_slope              = 1.0/4;
         double               comp_gain               = 4;

         // Gate
         decibel              gate_on_threshold       = -36_dB;
         decibel              gate_off_threshold      = -60_dB;

         // Attack / Decay
         duration             attack                  = 100_ms;
         duration             decay                   = 300_ms;
         duration             release                 = 800_ms;
         decibel              release_threshold       = -36_dB;

         // Release vibrato frequency
         frequency            release_vibrato_freq    = 3_Hz;
         double               release_vibrato_depth   = 0.018;
      };

                              pitch_follower(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel threshold = -30_dB
                              );

                              pitch_follower(
                                 config const& conf
                               , frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel threshold = -30_dB
                              );

      float                   operator()(float s);
      float                   envelope() const           { return _synth_env_val; }
      float                   frequency() const          { return _freq + _modulation; }
      float                   signal_envelope() const    { return _fast_env(); }

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
      float                   _synth_env_val;
      float                   _freq = 0.0f;
      float                   _stable_freq = 0.0f;
      float                   _max_stable_freq = 0.0f;
      float                   _min_stable_freq = 0.0f;
      bool                    _release_edge = false;
      phase_iterator          _release_vibrato_phase;
      float                   _release_vibrato_depth;
      float                   _modulation = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // implementation
   ////////////////////////////////////////////////////////////////////////////
   inline pitch_follower::pitch_follower(
      config const& conf
    , q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
    , decibel threshold
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
    , _release_vibrato_phase(conf.release_vibrato_freq, sps)
    , _release_vibrato_depth(conf.release_vibrato_depth)
   {}

   inline pitch_follower::pitch_follower(
      q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
    , decibel threshold
   )
    : pitch_follower(config{}, lowest_freq, highest_freq, sps, threshold)
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
         // Set frequency
         auto f_ = _pd.frequency();
         if (f_ == 0.0f)
            f_ = _pd.predict_frequency();
         if (f_ != 0.0f)
         {
            // Disregard if there is a sudden drop (> 3dB) in the
            // envelope, possibly due to palm mute or similar
            // or if we have a low periodicity.
            if (prev < (fast_env * 1.5) && _pd.periodicity() >= 0.8)
               _freq = f_;
            else if (_stable_freq != 0.0f)
               _freq = _stable_freq;   // get the latest stable frequency
         }
         _modulation = 0.0f;
         _release_edge = true;
      }
      else
      {
         if (_release_edge)
         {
            _freq = _stable_freq;
            _release_edge = false;
            _release_vibrato_depth =
               (_max_stable_freq - _min_stable_freq) / _stable_freq;
            _release_vibrato_phase._phase = phase{}; // reset the initial phase
         }
         _stable_freq = 0.0f;
         _modulation =
            _freq * q::sin(_release_vibrato_phase++) * _release_vibrato_depth;
         _release_vibrato_depth *= 0.999;
      }

      // Get the latest stable frequency
      if (pd_ready && _pd.periodicity() > 0.99)
      {
         if (_stable_freq == 0.0f)
         {
            _max_stable_freq = _min_stable_freq = _stable_freq = _freq;
         }
         else
         {
            auto deviation = _freq / 16;   // approx 1 semitone
            if (std::abs(_freq - _stable_freq) < deviation)
            {
               if (_freq > _max_stable_freq)
                  _max_stable_freq = _freq;
               if (_freq < _min_stable_freq)
                  _min_stable_freq = _freq;
               _stable_freq =
                  _min_stable_freq + ((_max_stable_freq - _min_stable_freq) / 2);
            }
            else
            {
               _max_stable_freq = _min_stable_freq = _stable_freq = _freq;
            }
         }
      }

      // Synthesize an envelope
      _synth_env_val = _synth_env(fast_env);

      return s;
   }
}}

#endif

