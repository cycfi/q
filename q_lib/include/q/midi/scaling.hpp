/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_SCALING_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_SCALING_HPP_SEPTEMBER_9_2026

#include <cstdint>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // Resolution scaling between the two protocols. MIDI 2.0 Bit Scaling
   // and Resolution (M2-115-U), sections 3 and 4.
   //
   // MIDI 1.0 holds 7 bits, or 14 for a bend; MIDI 2.0 holds 16 for a
   // velocity and 32 for everything else. Moving a value between them is
   // not one shift, because the three points a player can feel must land
   // exactly: silence on silence, full on full, and the centre of a wheel
   // on the centre. Two schemes cover it.
   //
   // Min-Center-Max, section 3, for controllers and anything continuous.
   // Below centre it is a shift. Above centre, the vacated low bits are
   // filled by repeating the value's own bits, which stretches the top half
   // so the maximum reaches the maximum. Coming back down is a plain shift,
   // and recovers the original.
   //
   // Zero extension, section 4, for a registered controller whose index is
   // 0 to 31: those are counts and settings, pitch bend sensitivity being a
   // number of semitones, and a count must not be stretched. Up is a shift,
   // down is a shift with rounding, clamped.
   ////////////////////////////////////////////////////////////////////////////

   // 3.3.1: the specification's own algorithm, kept in its shape.
   constexpr std::uint32_t scale_up(
      std::uint32_t value, unsigned from_bits, unsigned to_bits)
   {
      if (to_bits <= from_bits)
         return value;

      auto const scale_bits = to_bits - from_bits;
      auto const centre = std::uint32_t(1) << (from_bits - 1);
      auto const shifted = value << scale_bits;

      if (value <= centre)
         return shifted;

      // Above centre: repeat all but the top bit into the vacated bits, as
      // many times as they fit, so the maximum comes out all ones.
      auto const repeat_bits = from_bits - 1;
      auto const repeat_mask = (std::uint32_t(1) << repeat_bits) - 1;
      auto repeat = value & repeat_mask;

      if (scale_bits > repeat_bits)
         repeat <<= scale_bits - repeat_bits;
      else
         repeat >>= repeat_bits - scale_bits;

      auto result = shifted;
      while (repeat != 0)
      {
         result |= repeat;
         repeat >>= repeat_bits;
      }
      return result;
   }

   // 3.4.1: "simple bit shift". A value that was scaled up comes back as
   // it went.
   constexpr std::uint32_t scale_down(
      std::uint32_t value, unsigned from_bits, unsigned to_bits)
   {
      if (from_bits <= to_bits)
         return value;
      return value >> (from_bits - to_bits);
   }

   // 4.3.1: up is a shift, so 127 becomes 65024 and not 65535.
   constexpr std::uint32_t zero_extend_up(
      std::uint32_t value, unsigned from_bits, unsigned to_bits)
   {
      if (to_bits <= from_bits)
         return value;
      return value << (to_bits - from_bits);
   }

   // 4.4.1: down adds half the dropped range first, rounding to the nearest
   // low resolution value, then clamps, because rounding can carry out of
   // the destination.
   constexpr std::uint32_t zero_extend_down(
      std::uint32_t value, unsigned from_bits, unsigned to_bits)
   {
      if (from_bits <= to_bits)
         return value;

      auto const scale_bits = from_bits - to_bits;
      auto const half = std::uint32_t(1) << (scale_bits - 1);
      auto const max = (std::uint32_t(1) << to_bits) - 1;

      // Widened, so adding half cannot overflow a 32 bit source.
      auto const shifted = std::uint32_t(
         (std::uint64_t(value) + half) >> scale_bits);
      return shifted > max? max : shifted;
   }

   // 3.1, 4.1: registered controllers with an index of 0 to 31 are the
   // MIDI 1.0 registered parameters, and keep their values by zero
   // extension. Every other controller uses Min-Center-Max.
   constexpr bool registered_uses_zero_extension(std::uint8_t index)
   {
      return index < 32;
   }
}

#endif
