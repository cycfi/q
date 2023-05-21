/*=============================================================================
   Copyright (C) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_HPP_APRIL_22_2021)
#define CYCFI_Q_PITCH_HPP_APRIL_22_2021

#include <q/support/frequency.hpp>
#include <q/support/interval.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Pitch is determined by its position on the chromatic scale, which is a
   // system of 12 notes that repeat in octaves. The distance between each
   // pitch on the chromatic scale is a semitone, and each pitch represents a
   // specific frequency measured in hertz (Hz) using 12-tone equal
   // temperament (12-TET).
   //
   // Pitch is represented as an `interval` with an implied base frequency of
   // `8.1757989156437` Hz that corresponds to MIDI note 0 (which is an
   // octave below F#0), and is therefore represented by the MIDI value. Only
   // positive values are valid.
   //
   // 12-TET conversion functions to and from `interval` and `frequency` are
   // provided. `pitch(f)` is a function that constructs an `interval` from
   // `frequency`. `as_frequency(i)` converts a `basic_interval<T>` to
   // `frequency`. These functions utilize fast log2 and pow2 computations
   // using fast math functions.
   ////////////////////////////////////////////////////////////////////////////
   constexpr auto base_pitch_frequency = frequency{8.1757989156437};

   constexpr interval   pitch();
   interval             pitch(frequency f);

                        template <typename T>
   constexpr bool       is_valid_pitch(basic_interval<T> i);

                        template <typename T>
   frequency            as_frequency(basic_interval<T> i);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline interval pitch(frequency f)
   {
      return interval{12 * fast_log2(f / base_pitch_frequency)};
   }

   constexpr interval pitch()
   {
      return interval{-1.0};
   }

   template <typename T>
   constexpr bool is_valid_pitch(basic_interval<T> i)
   {
      return i.rep > 0;
   }

   template <typename T>
   inline frequency as_frequency(basic_interval<T> i)
   {
      return base_pitch_frequency * fast_pow2(i.rep / 12);
   }
}

#endif

