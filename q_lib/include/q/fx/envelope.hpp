/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FX_ENVELOPE_HPP_MAY_17_2018)
#define CYCFI_Q_FX_ENVELOPE_HPP_MAY_17_2018

#include <q/support/literals.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/lowpass.hpp>
#include <q/support/decibel.hpp>
#include <algorithm>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // During the attack phase of an audio signal, the peak envelope follower
   // closely tracks the maximum peak level. When the signal level drops
   // below the peak, the follower gradually releases the peak level with an
   // exponential decay.
   ////////////////////////////////////////////////////////////////////////////
   struct peak_envelope_follower
   {
                              peak_envelope_follower(duration release, float sps);

      float                   operator()(float s);
      float                   operator()() const;
      peak_envelope_follower& operator=(float y_);
      void                    release(duration release_, float sps);

      float y = 0.0f, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The ar_envelope_follower follower tracks the envelope of a signal with
   // the given attack parameter and with gradual release given by the
   // release parameter. The signal decays exponentially if the signal is
   // below the peak.
   ////////////////////////////////////////////////////////////////////////////
   struct ar_envelope_follower
   {
                              ar_envelope_follower(
                                 duration attack
                               , duration release
                               , float sps
                              );

      float                   operator()(float s);
      float                   operator()() const;
      ar_envelope_follower&   operator=(float y_);
      void                    config(duration attack, duration release, float sps);
      void                    attack(float attack_, float sps);
      void                    release(float release_, float sps);

      float y = 0.0f, _attack, _release;
   };

   using envelope_follower [[deprecated("Use ar_envelope_follower instead.")]]
      = ar_envelope_follower;

   ////////////////////////////////////////////////////////////////////////////
   // This envelope follower combines fast response, low ripple.
   //
   // Based on https://bit.ly/3zmGLIf, https://bit.ly/40OhYJf
   //
   // There is no filtering. The output is a jagged, staircase-like envelope.
   // That way, this can be useful for analysis such as onset detection. For
   // monophonic signals, the hold duration should be equal to or slightly
   // longer than 1/div the period of the lowest frequency of the signal we
   // wish to track, where `div` is the sole template parameter. The hold
   // parameter determines the staircase step duration. This staircase-like
   // envelope can be effectively smoothed out using a moving average filter
   // with the same duration as the hold parameter.
   //
   // fast_envelope_follower is provided, which has div = 2.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t div>
   struct basic_fast_envelope_follower
   {
      static_assert(div >= 1, "div must be >= 1");
      static constexpr std::size_t size = div+1;

               basic_fast_envelope_follower(duration hold, float sps);
               basic_fast_envelope_follower(std::size_t hold_samples);

      float    operator()(float s);
      float    operator()() const;

      std::array<float, size> _y;
      float _peak = 0;
      std::uint16_t _tick = 0, _i = 0;
      std::uint16_t const _reset;
   };

   using fast_envelope_follower = basic_fast_envelope_follower<2>;

   ////////////////////////////////////////////////////////////////////////////
   // This is a fast_envelope_follower followed by a moving average filter to
   // smooth out the staircase ripples as mentioned in the
   // fast_envelope_follower notes.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t div>
   struct basic_fast_ave_envelope_follower
   {
               basic_fast_ave_envelope_follower(duration hold, float sps);
               basic_fast_ave_envelope_follower(std::size_t hold_samples);

      float    operator()(float s);
      float    operator()() const;

      basic_fast_envelope_follower<div> _fenv;
      moving_average _ma;
   };

   using fast_ave_envelope_follower = basic_fast_ave_envelope_follower<2>;

   ////////////////////////////////////////////////////////////////////////////
   // This rms envelope follower combines fast response, low ripple using
   // moving RMS detection and the fast_ave_envelope_follower for
   // tracking the moving RMS.
   //
   // The signal path is as follows:
   //    1. Square signal
   //    2. Fast averaging envelope follower
   //    3. Square root.
   //
   // The `fast_rms_envelope_follower_db` variant works in the dB domain,
   // which makes it easy to use as an envelope follower for dynamic range
   // effects (compressor, expander, and agc) which already work in the dB
   // domain, so we eliminate a linear to decibel conversion and optimize
   // computation by using division by 2 instead of sqrt as an added bonus.
   //
   // Designed by Joel de Guzman (June 2020)
   ////////////////////////////////////////////////////////////////////////////
   struct fast_rms_envelope_follower
   {
      constexpr static auto threshold = lin_float(-120_dB);

               fast_rms_envelope_follower(duration hold, float sps);
      float    operator()(float s);

      fast_ave_envelope_follower  _fenv;
   };

   struct fast_rms_envelope_follower_db : fast_rms_envelope_follower
   {
      using fast_rms_envelope_follower::fast_rms_envelope_follower;

      decibel operator()(float s);
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // peak_envelope_follower
   inline peak_envelope_follower::peak_envelope_follower(duration release, float sps)
      : _release(fast_exp3(-2.0f / (sps * as_double(release))))
   {}

   inline float peak_envelope_follower::operator()(float s)
   {
      if (s > y)
         y = s;
      else
         y = s + _release * (y - s);
      return y;
   }

   inline float peak_envelope_follower::operator()() const
   {
      return y;
   }

   inline peak_envelope_follower& peak_envelope_follower::operator=(float y_)
   {
      y = y_;
      return *this;
   }

   inline void peak_envelope_follower::release(duration release_, float sps)
   {
      _release = fast_exp3(-2.0f / (sps * as_double(release_)));
   }


   ////////////////////////////////////////////////////////////////////////////
   // ar_envelope_follower
   inline ar_envelope_follower::ar_envelope_follower(
      duration attack, duration release, float sps)
      : _attack(fast_exp3(-2.0f / (sps * as_double(attack))))
      , _release(fast_exp3(-2.0f / (sps * as_double(release))))
   {}

   inline float ar_envelope_follower::operator()(float s)
   {
      return y = s + ((s > y)? _attack : _release) * (y - s);
   }

   inline float ar_envelope_follower::operator()() const
   {
      return y;
   }

   inline ar_envelope_follower& ar_envelope_follower::operator=(float y_)
   {
      y = y_;
      return *this;
   }

   inline void ar_envelope_follower::config(duration attack, duration release, float sps)
   {
      _attack = fast_exp3(-2.0f / (sps * as_double(attack)));
      _release = fast_exp3(-2.0f / (sps * as_double(release)));
   }

   inline void ar_envelope_follower::attack(float attack_, float sps)
   {
      _attack = fast_exp3(-2.0f / (sps * attack_));
   }

   inline void ar_envelope_follower::release(float release_, float sps)
   {
      _release = fast_exp3(-2.0f / (sps * release_));
   }

   ////////////////////////////////////////////////////////////////////////////
   // basic_fast_envelope_follower<div>
   template <std::size_t div>
   inline basic_fast_envelope_follower<div>
      ::basic_fast_envelope_follower(duration hold, float sps)
      : basic_fast_envelope_follower((as_float(hold) * sps))
   {}

   template <std::size_t div>
   inline basic_fast_envelope_follower<div>::
      basic_fast_envelope_follower(std::size_t hold_samples)
      : _reset(hold_samples)
   {
      _y.fill(0);
   }

   template <std::size_t div>
   inline float basic_fast_envelope_follower<div>::operator()(float s)
   {
      // Update _y
      for (auto& y : _y)
         y = std::max(s, y);

      // Reset _y in a round-robin fashion every so often (the hold parameter)
      if (_tick++ == _reset)
      {
         _tick = 0;
         _y[_i++ % size] = 0;
      }

      // The peak is the maximum of _y
      _peak = *std::max_element(_y.begin(), _y.end());
      return _peak;
   }

   template <std::size_t div>
   inline float basic_fast_envelope_follower<div>::operator()() const
   {
      return _peak;
   }

   ////////////////////////////////////////////////////////////////////////////
   // basic_fast_ave_envelope_follower<div>
   template <std::size_t div>
   inline basic_fast_ave_envelope_follower<div>::
      basic_fast_ave_envelope_follower(duration hold, float sps)
      : _fenv(hold, sps)
      , _ma(hold, sps)
   {}

   template <std::size_t div>
   inline basic_fast_ave_envelope_follower<div>::
      basic_fast_ave_envelope_follower(std::size_t hold_samples)
      : _fenv(hold_samples)
      , _ma(hold_samples)
   {}

   template <std::size_t div>
   inline float basic_fast_ave_envelope_follower<div>::operator()(float s)
   {
      return _ma(_fenv(s));
   }

   template <std::size_t div>
   inline float basic_fast_ave_envelope_follower<div>::operator()() const
   {
      return _ma();
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast_rms_envelope_follower
   inline fast_rms_envelope_follower::fast_rms_envelope_follower(
      duration hold, float sps)
    : _fenv(hold, sps)
   {
   }

   inline float fast_rms_envelope_follower::operator()(float s)
   {
      auto e = _fenv(s*s);
      if (e < threshold)
         e = 0;
      return fast_sqrt(e);
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast_rms_envelope_follower
   inline decibel fast_rms_envelope_follower_db::operator()(float s)
   {
      auto e = _fenv(s * s);
      if (e < threshold)
         e = 0;

      // Perform square-root in the dB domain:
      return lin_to_db(e) / 2.0f;
   }
}

#endif
