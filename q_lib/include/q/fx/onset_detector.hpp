/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ONSET_DETECTOR_NOVEMBER_9_2019)
#define CYCFI_Q_ONSET_DETECTOR_NOVEMBER_9_2019

#include <q/fx/envelope.hpp>
#include <q/fx/special.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/dynamic.hpp>

namespace cycfi::q
{
   struct onset_detector
   {
      static constexpr auto pre_gain = 4.0f;
      static constexpr auto makeup_gain = 4.0f;
      static constexpr auto compressor_threshold = -24_dB;
      static constexpr auto compressor_slope = 1.0/10;
      static constexpr auto release = 50_ms;

      onset_detector(decibel hysteresis, std::uint32_t sps)
       : _comp_env{ 2_ms, 1_s, sps }
       , _comp{ compressor_threshold, compressor_slope }
       , _pos_env{ release, sps }
       , _neg_env{ release, sps }
       , _slow_env{ 10_ms, release, sps }
       , _trigger{ hysteresis }
      {}

      float operator()(float s)
      {
         // Second derivative (acceleration)
         auto diff = _diff2(_diff1(s));

         // Compressor
         diff *= pre_gain;
         q::decibel env_out = _comp_env(std::abs(diff));
         auto gain = float(_comp(env_out)) * makeup_gain;
         diff *= gain;

         // Peak Envelope Followers
         auto pos_env = _pos_env(diff);
         auto neg_env = _neg_env(-diff);
         auto peak_env = (pos_env + neg_env) / 2;

         // Peak detection
         auto slow_env = _slow_env(peak_env);
         return _trigger(peak_env, slow_env);
      }

      float peak_env() const
      {
         return (_pos_env() + _neg_env()) / 2;
      }

      envelope_follower       _comp_env;
      compressor              _comp;

      central_difference      _diff1;
      differentiator          _diff2;
      peak_envelope_follower  _pos_env, _neg_env;

      envelope_follower       _slow_env;
      schmitt_trigger         _trigger;
   };
}

#endif
