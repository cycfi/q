/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_BEND_RANGE_HPP_NOVEMBER_5_2026)
#define CYCFI_Q_MIDI_BEND_RANGE_HPP_NOVEMBER_5_2026

#include <q/midi/messages.hpp>
#include <q/midi/ump_messages.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cycfi::q::midi
{
   ////////////////////////////////////////////////////////////////////////////
   // bend_range: a pitch bend as a pitch offset, in semitones or as a
   // frequency ratio, with its own depth below and above the center. A bar
   // or a pedal may dive farther than it pulls up; a wheel bends the same
   // both ways. The depths are in semitones, both positive.
   //
   // The center is exact, 0 semitones, and each end reaches its full
   // depth. The bend is three straight segments: down, middle and up.
   // The middle spans `linear` semitones either side of the center at one
   // rate, so a vibrato across the center is as deep up as down; the down
   // and up segments carry on from it to their own depths (see note 1).
   //
   // Reads both protocols' pitch bend: MIDI 1.0's 14 bits and MIDI 2.0's
   // 32. The default depth, 2 semitones both ways, is the specifications'
   // default pitch bend sensitivity.
   ////////////////////////////////////////////////////////////////////////////
   struct bend_range
   {
      constexpr            bend_range(float depth = 2.0f);
      constexpr            bend_range(
                              float down, float up, float linear = 1.0f);

      constexpr float      semitones(midi_1_0::pitch_bend msg) const;
      constexpr float      semitones(midi_2_0::pitch_bend msg) const;
      float                ratio(midi_1_0::pitch_bend msg) const;
      float                ratio(midi_2_0::pitch_bend msg) const;

      float                down;
      float                up;
      float                linear;

   private:

      constexpr float      offset(double x) const;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Notes:
   //
   // 1. x is the bend from -1 to 1: below the center divided by the span
   //    down to the minimum, above it by the span up to the maximum, one
   //    less (8192 and 8191 for 14 bits). The middle runs at the mean of
   //    the two depths, k = (down + up) / 2 semitones per unit of x, out
   //    to w = linear / k, so its share of the travel is its share of the
   //    semitones. The down and up segments are straight from there to
   //    their depths. `linear` is limited to the smaller depth, so a side
   //    of depth 0 leaves no middle.
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr bend_range::bend_range(float depth)
    : down{depth}
    , up{depth}
    , linear{1.0f}
   {}

   constexpr bend_range::bend_range(float down, float up, float linear)
    : down{down}
    , up{up}
    , linear{linear}
   {}

   constexpr float bend_range::offset(double x) const
   {
      auto const sign = x < 0 ? -1.0 : 1.0;
      auto const a = x * sign;
      auto const depth = double(x < 0 ? down : up);
      auto const k = (double(down) + double(up)) / 2;
      auto const lo = double(std::min(down, up));
      auto const m = std::clamp(double(linear), 0.0, lo);

      if (k <= 0)
         return 0.0f;

      auto const w = m / k;
      if (a <= w)
         return float(sign * k * a);
      return float(sign * (m + (depth - m) * (a - w) / (1.0 - w)));
   }

   constexpr float bend_range::semitones(midi_1_0::pitch_bend msg) const
   {
      auto const d = int(msg.value()) - 8192;
      return offset(d < 0 ? d / 8192.0 : d / 8191.0);
   }

   constexpr float bend_range::semitones(midi_2_0::pitch_bend msg) const
   {
      constexpr auto center = std::int64_t{midi_2_0::pitch_bend::center};
      auto const d = std::int64_t{msg.value()} - center;
      return offset(d < 0 ? double(d) / center : double(d) / (center - 1));
   }

   inline float bend_range::ratio(midi_1_0::pitch_bend msg) const
   {
      return std::exp2(semitones(msg) / 12.0f);
   }

   inline float bend_range::ratio(midi_2_0::pitch_bend msg) const
   {
      return std::exp2(semitones(msg) / 12.0f);
   }
}

#endif
