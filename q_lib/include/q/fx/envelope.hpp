/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FX_ENVELOPE_HPP_MAY_17_2018)
#define CYCFI_Q_FX_ENVELOPE_HPP_MAY_17_2018

#include <q/support/literals.hpp>
#include <algorithm>

namespace cycfi { namespace q
{
   using namespace q::literals;

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
         s = std::abs(s);
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
   // That way, this can be useful for analysis such as onset detection.
   ////////////////////////////////////////////////////////////////////////////
   struct fast_envelope_follower
   {
      fast_envelope_follower(duration hold, std::uint32_t sps)
       : _reset((float(hold) * sps))
      {}

      float operator()(float s)
      {
         if (s > _peak)
            _peak = s;

         // Get the peak and hold it in _y1 and _y2
         if (_peak > _y1)
            _y1 = _peak;
         if (_peak > _y2)
            _y2 = _peak;

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
         _peak = 0;
         return _latest;
      }

      float operator()() const
      {
         return _latest;
      }

      float _y1 = 0, _y2 = 0, _peak = 0, _latest = 0;
      std::uint16_t _tick = 0, _i = 0;
      std::uint16_t const _reset;
   };

   ////////////////////////////////////////////////////////////////////////////
   // envelope_shaper is an envelope processor that allows control of the
   // envelope's attack, decay and release parameters. Take note that the
   // envelope_shaper is a processor and does not synthesize an envelope. It
   // takes in an envelope and processes it to increase (but not decrease)
   // attack, decay and release.
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_shaper
   {
      static constexpr float hysteresis = 0.0001; // -80dB

      envelope_shaper(
         duration attack
       , duration decay
       , duration release
       , decibel release_threshold
       , std::uint32_t sps
      ) : envelope_shaper(
         fast_exp3(-2.0f / (sps * double(attack)))
       , fast_exp3(-2.0f / (sps * double(decay)))
       , fast_exp3(-2.0f / (sps * double(release)))
       , double(release_threshold))
      {}

      envelope_shaper(
         float attack
       , float decay
       , float release
       , float release_threshold
      )
       : _attack(attack)
       , _decay(decay)
       , _release(release)
       , _release_threshold(release_threshold)
      {}

      float operator()(float s)
      {
         if (y < _peak || s > y) // upward
         {
            if (_peak < s)
               _peak = s;
            auto target = 1.6f * s;
            y = target + _attack * (y - target);
            if (y > _peak)
               _peak = 0;
         }
         else
         {
            auto slope = (s < _release_threshold)? _release : _decay;
            y = s + slope * (y - s);
            if (y < hysteresis)
               _peak = y = 0;
         }
         return y;
      }

      float operator()() const
      {
         return y;
      }

      void config(duration attack, duration decay, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * double(attack)));
         _decay = fast_exp3(-2.0f / (sps * double(decay)));
      }

      void attack(float attack_, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * attack_));
      }

      void release(float release_, std::uint32_t sps)
      {
         _decay = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0, _peak = 0, _attack, _decay, _release, _release_threshold;
   };
}}

#endif
