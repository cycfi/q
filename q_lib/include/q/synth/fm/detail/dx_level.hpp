/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_DX_LEVEL_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_DX_LEVEL_HPP_SEPTEMBER_15_2026

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/synth/fm/detail/dx_tables.hpp>
#include <algorithm>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // DX7 levels, as measured on a DX7s (Music Synthesizer for Android
   // wiki): an operator's total level is counted in steps of 1/256 of a
   // doubling (about 0.0235 dB), 64 per envelope level unit plus 32 per
   // output level unit, with full scale when both are 99. Levels more than
   // 3824 counts below full scale are silent.
   //
   // Envelope levels above 19 move in pairs (L >> 1), so an even level
   // acts as the odd one above it. Output levels move in 0.75 dB steps.
   ////////////////////////////////////////////////////////////////////////////
   constexpr int dx_eg_level_units(int level)
   {
      if (level <= 5)
         return 2 * level;
      if (level <= 16)
         return 5 + level;
      if (level <= 19)
         return 4 + level;
      return 14 + (level >> 1);
   }

   constexpr int dx_output_level_units(int level)
   {
      level = std::clamp(level, 0, 99);
      return level < 20 ? dx_output_level_low[level] : 28 + level;
   }

   constexpr float dx_full_scale = 64.0f * 63 + 32.0f * 127;   // 8096
   constexpr float dx_min_counts = dx_full_scale - 3824;
   constexpr float dx_db_per_count = 6.0206f / 256;

   // Gain of a total level in counts; silent at the floor
   constexpr float dx_counts_gain(float counts)
   {
      if (counts <= dx_min_counts)
         return 0.0f;
      return lin_float(dB((counts - dx_full_scale) * dx_db_per_count));
   }

   // Gain of an output level alone, the envelope at full
   constexpr float dx_level_gain(float level)
   {
      auto units = dx_output_level_units(int(level + 0.5f));
      return dx_counts_gain(64.0f * 63 + 32.0f * units);
   }
}

#endif
