/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_CHORUS_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_CHORUS_SEPTEMBER_11_2026

#include <q/fx/delay.hpp>
#include <q/synth/sin_cos_gen.hpp>
#include <q/support/duration.hpp>
#include <q/support/frequency.hpp>
#include <algorithm>

namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// chorus: a delay line whose length is swept by a slow sine, its output
// mixed with the dry signal.
//
//    chorus c{10_ms, 2_ms, 1_Hz, 0.5f, sps};
//    y = c(x);
//
// The base is the centre of the sweep, the depth how far each side of it
// the sine reaches, the rate how fast. The line is sized at construction
// for base plus depth, and the sweep is held between one sample and that,
// so a setting that asks for more flattens at the edge rather than
// reading past the line or ahead of now. The mix is 0 for dry, 1 for the
// delayed copy alone; a chorus sits in between.
//
// This lives here, in the plugin, rather than in Q, because there is no
// one chorus: another would use several taps at different phases, or a
// triangle instead of a sine, or feed its output back in, or spread the
// copies across the stereo field. Those are choices an instrument makes,
// not a library. What Q provides is what has no such choice in it: the
// fractional delay, which is what lets the read point glide between
// samples instead of stepping, so the delayed copy stays in tune with
// itself as it wavers, and sin_cos_gen, a recursive sine made for low
// frequencies at a few multiplies a sample.
///////////////////////////////////////////////////////////////////////////////
class chorus
{
public:

                     chorus(
                        q::duration base, q::duration depth
                      , q::frequency rate, float mix, float sps);

   float             operator()(float s);

   void              base(q::duration d, float sps);
   void              depth(q::duration d, float sps);
   void              rate(q::frequency f, float sps);
   void              mix(float m);
   void              reset();

private:

   q::delay          _delay;
   q::sin_cos_gen    _lfo;
   float             _base;      // in samples
   float             _depth;     // in samples
   float             _longest;   // the line's length, in samples
   float             _mix;
};

///////////////////////////////////////////////////////////////////////////////
// Inline Implementation
///////////////////////////////////////////////////////////////////////////////
inline chorus::chorus(
   q::duration base_, q::duration depth_, q::frequency rate_
 , float mix_, float sps)
 : _delay{q::duration{q::as_double(base_) + q::as_double(depth_)}, sps}
 , _lfo{rate_, sps}
 , _base{q::as_float(base_) * sps}
 , _depth{q::as_float(depth_) * sps}
 , _longest{float(_delay.size()) - 2.0f}
 , _mix{mix_}
{
}

inline float chorus::operator()(float s)
{
   // The line reads at its index and then takes the new sample, so an
   // index of i is a delay of i + 1: asked for d, it is given d - 1.
   auto const d = std::clamp(
      _base + (_depth * _lfo().first), 1.0f, _longest);
   auto const wet = _delay(s, d - 1.0f);
   return (s * (1.0f - _mix)) + (wet * _mix);
}

inline void chorus::base(q::duration d, float sps)
{
   _base = q::as_float(d) * sps;
}

inline void chorus::depth(q::duration d, float sps)
{
   _depth = q::as_float(d) * sps;
}

inline void chorus::rate(q::frequency f, float sps)
{
   _lfo.config(f, sps);
}

inline void chorus::mix(float m)
{
   _mix = m;
}

// Where the sweep sits in its cycle is not state worth keeping, so only
// the line is cleared: the LFO keeps its rate and runs on.
inline void chorus::reset()
{
   _delay.clear();
}

#endif
