/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

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
   // The envelope follower will follow the envelope of a signal with gradual
   // release (given by the release parameter). The signal decays
   // exponentially if the signal is below the peak.
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_follower
   {
      envelope_follower(duration attack, duration release, std::uint32_t sps)
       : _attack(fast_exp3(-2.0f / (sps * double(attack))))
       , _release(fast_exp3(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         return y = s + ((s > y)? _attack : _release) * (y - s);
      }

      float operator()() const
      {
         return y;
      }

      envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void config(duration attack, duration release, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * double(attack)));
         _release = fast_exp3(-2.0f / (sps * double(release)));
      }

      void attack(float attack_, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * attack_));
      }

      void release(float release_, std::uint32_t sps)
      {
         _release = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0.0f, _attack, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Same as envelope follower above, but with attack = 0;
   ////////////////////////////////////////////////////////////////////////////
   struct peak_envelope_follower
   {
      peak_envelope_follower(duration release, std::uint32_t sps)
       : _release(fast_exp3(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         if (s > y)
            y = s;
         else
            y = s + _release * (y - s);
         return y;
      }

      float operator()() const
      {
         return y;
      }

      peak_envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void release(float release_, std::uint32_t sps)
      {
         _release = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0.0f, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // This envelope follower combines fast response, low ripple.
   //
   // Based on http://tinyurl.com/yat2tuf8
   //
   // There is no filtering. The output is a jagged, staircase-like envelope.
   // That way, this can be useful for analysis such as onset detection. For
   // monophonic signals, the hold duration should be longer than the period
   // of the lowest frequency of the signal we wish to track. The hold
   // parameter determines the staircase step duration. This staircase-like
   // envelope can be effectively smoothed out using a moving average filter
   // with the same duration as the hold parameter.
   ////////////////////////////////////////////////////////////////////////////
   struct fast_envelope_follower
   {
      fast_envelope_follower(duration hold, std::uint32_t sps)
       : _reset((float(hold) * sps))
      {}

      fast_envelope_follower(std::size_t hold_samples)
       : _reset(hold_samples)
      {}

      float operator()(float s)
      {
         // Update _y1 and _y2
         if (s > _y1)
            _y1 = s;
         if (s > _y2)
            _y2 = s;

         // Reset _y1 and _y2 alternately every so often (the hold parameter)
         if (_tick++ == _reset)
         {
            _tick = 0;
            if (_i++ & 1)
               _y1 = 0;
            else
               _y2 = 0;
         }

         // The peak is the maximum of _y1 and _y2
         _latest = std::max(_y1, _y2);
         return _latest;
      }

      float operator()() const
      {
         return _latest;
      }

      float _y1 = 0, _y2 = 0, _latest = 0;
      std::uint16_t _tick = 0, _i = 0;
      std::uint16_t const _reset;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak_hold holds the peak in a given time window, specified by the hold
   // of hold_samples parameters. The peak presented is delayed by this
   // specified window.
   //
   // For better time resolution, there are two overlapping windows of the
   // same specified size, spaced by half the window size. Such an
   // arrangement will provide peak updates every half of the specified
   // window.
   //
   // peak_hold returns is a pair, with the actual result in the first
   // element and the second element a bool indicating the result is a new
   // peak.
   ////////////////////////////////////////////////////////////////////////////
   struct peak_hold
   {
      peak_hold(std::size_t hold_samples)
       : _reset(hold_samples)
       , _peak{ 0 }
       , _y1{ 0 }
       , _y2{ 0 }
       , _tick1{ 0 }
       , _tick2(hold_samples / 2)
      {
      }

      peak_hold(duration hold, std::uint32_t sps)
       : peak_hold((float(hold) * sps))
      {
      }

      std::pair<float, bool> operator()(float s)
      {
         // Reset _y1 and _y2 every so often (the hold parameter)
         if (_tick1++ == _reset)
         {
            _tick1 = 0;
            _peak = _y1;
            _y1 = 0;
         }
         if (_tick2++ == _reset)
         {
            _tick2 = 0;
            _peak = _y2;
            _y2 = 0;
         }
         _y1 = std::max(_y1, s);
         _y2 = std::max(_y2, s);
         return { _peak, _tick1 == 0 || _tick2 == 0 };
      }

      float operator()() const
      {
         return _peak;
      }

      float _peak, _y1, _y2;
      std::uint16_t _tick1, _tick2;
      std::uint16_t const _reset;
   };

   ////////////////////////////////////////////////////////////////////////////
   // This is a fast_envelope_follower followed by a moving average filter to
   // smooth out the staircase ripples as mentioned in the
   // fast_envelope_follower notes.
   ////////////////////////////////////////////////////////////////////////////
   struct smoothed_fast_envelope_follower
   {
      smoothed_fast_envelope_follower(duration hold, std::uint32_t sps)
       : _fenv(hold, sps)
       , _ma(hold, sps)
      {}

      smoothed_fast_envelope_follower(std::size_t hold_samples)
       : _fenv(hold_samples)
       , _ma(hold_samples)
      {}

      float operator()(float s)
      {
         return _ma(_fenv(s));
      }

      float operator()() const
      {
         return _ma();
      }

      fast_envelope_follower  _fenv;
      moving_average          _ma;
   };

   ////////////////////////////////////////////////////////////////////////////
   // This rms envelope follower combines fast response, low ripple using
   // moving RMS detection and the smoothed_fast_envelope_follower for
   // tracking the moving RMS. Unlike other envelope followers in this
   // header, this one works in the dB domain, which makes it easy to use as
   // an envelope follower for dynamic range effects (compressor, expander,
   // and agc) which also work in the dB domain.
   //
   // The signal path is as follows:
   //    1. Square signal
   //    2. Smoothed fast envelope follower
   //    3. Square root.
   //
   // Designed by Joel de Guzman (June 2020)
   ////////////////////////////////////////////////////////////////////////////
   struct fast_rms_envelope_follower
   {
      constexpr static auto threshold = float(-120_dB);

      fast_rms_envelope_follower(duration hold, std::uint32_t sps)
       : _fenv(hold, sps)
      {
      }

      decibel operator()(float s)
      {
         auto e = _fenv(s * s);
         if (e < threshold)
            e = 0;

         // Perform square-root in the dB domain:
         _db = decibel{ e } / 2.0f;
         return _db;
      }

      decibel operator()() const
      {
         return _db;
      }

      q::decibel                       _db = 0_dB;
      smoothed_fast_envelope_follower  _fenv;
   };
}

#endif
