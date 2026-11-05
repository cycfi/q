/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TRANSIENT_DETECTOR_NOVEMBER_1_2026)
#define CYCFI_Q_TRANSIENT_DETECTOR_NOVEMBER_1_2026

#include <q/fx/delta_gate.hpp>
#include <q/fx/monostable.hpp>
#include <q/fx/spectral_flux.hpp>
#include <q/support/decibel.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // transient_detector: true for one sample at a transient. The signal's
   // level above `cutoff` (spectral_flux, peak-held over a millisecond)
   // must rise `rise` over its own value about `lookback` ago (delta_gate)
   // while above `floor`; after firing it is deaf for `refractory`. Time
   // domain, with no frame to wait for: on a plucked string it fires
   // within a fraction of a millisecond of the release, and not on the
   // pick touching the string before it, which is 20 to 30 dB lower.
   //
   // The defaults are tuned on Nu Multi pickup recordings: single notes,
   // harmonics and strummed chords, where a strum's strings fire once. The
   // short peak hold keeps re-strums over ringing strings visible, but a
   // tone pulsed in that band with silence between its pulses (a clean
   // synthetic sawtooth) can fire on its pulses. The recordings it was
   // tuned on never do.
   ////////////////////////////////////////////////////////////////////////////
   struct transient_detector
   {
                     transient_detector(float sps);
                     transient_detector(
                        frequency cutoff
                      , decibel rise
                      , duration lookback
                      , decibel floor
                      , duration refractory
                      , float sps
                     );

      bool           operator()(float s);
      bool           operator()() const;

      static constexpr duration window = duration{0.001};

   private:

      spectral_flux  _flux;
      delta_gate     _rise;
      monostable     _refractory;
      float          _floor;
      bool           _was = false;
      bool           _fired = false;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline transient_detector::transient_detector(float sps)
    : transient_detector{
         frequency{4000.0}, decibel{22.0, direct_unit}, duration{0.010}
       , decibel{-45.0, direct_unit}, duration{0.100}, sps}
   {}

   inline transient_detector::transient_detector(
      frequency cutoff
    , decibel rise
    , duration lookback
    , decibel floor
    , duration refractory
    , float sps
   )
    : _flux{cutoff, window, sps}
    , _rise{rise, lookback, sps}
    , _refractory{refractory, sps}
    , _floor{lin_float(floor)}
   {}

   inline bool transient_detector::operator()(float s)
   {
      auto const level = _flux(s);
      bool const on = _rise(level) && level > _floor;
      bool const deaf = _refractory();
      _fired = on && !_was && !deaf;
      _refractory(_fired);
      _was = on;
      return _fired;
   }

   inline bool transient_detector::operator()() const
   {
      return _fired;
   }
}

#endif
