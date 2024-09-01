/*=============================================================================
   Copyright (C) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_HPP_APRIL_22_2021)
#define CYCFI_Q_PITCH_HPP_APRIL_22_2021

#include <q/support/frequency.hpp>
#include <q/support/interval.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // `pitch` is determined by its position on the chromatic scale, which is
   // a system of 12 notes that repeat in octaves. The distance between each
   // pitch on the chromatic scale is a semitone, and each pitch represents a
   // specific frequency measured in hertz (Hz).
   //
   // `pitch` is an `interval` with a base frequency of `8.1757989156437`
   // that correspond to MIDI note 0 (which is an octave below F#0). `pitch`
   // is represented by the MIDI value. Therefore, only positive values are
   // valid.
   //
   // `pitch` includes construction from `frequency` as well as numeric
   // values representing the absolute position in the chromatic scale from
   // the base frequency. `pitch` also includes conversions to `frequency`.
   //
   // Conversions to and from `frequency` utilize fast log2 and pow2
   // computations using fast math functions.
   ////////////////////////////////////////////////////////////////////////////
   struct pitch : interval
   {
      using base_type = interval;
      using base_type::base_type;

      constexpr static auto base_frequency = frequency{8.1757989156437};

      constexpr            pitch();
      explicit             pitch(frequency f);

      constexpr bool       valid() const;

      // These operations do not make sense and are not allowed.
      pitch&               operator+=(pitch) = delete;
      pitch&               operator-=(pitch) = delete;
   };

   // Free functions
   inline frequency  as_frequency(pitch n);
   inline float      as_float(pitch n);
   inline double     as_double(pitch n);

   constexpr pitch   round(pitch n);
   constexpr pitch   ceil(pitch n);
   constexpr pitch   floor(pitch n);

   // These operations do not make sense and are not allowed.
   pitch operator+(pitch, pitch) = delete;
   pitch operator-(pitch, pitch) = delete;

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline pitch::pitch(frequency f)
    : base_type{12 * fast_log2(f / base_frequency)}
   {}

   constexpr pitch::pitch()
    : base_type(-1.0f)
   {}

   constexpr bool pitch::valid() const
   {
      return rep > 0;
   }

   inline frequency as_frequency(pitch n)
   {
      return pitch::base_frequency * fast_pow2(n.rep / 12);
   }

   inline float as_float(pitch n)
   {
      return n.rep;
   }

   inline double as_double(pitch n)
   {
      return n.rep;
   }

   constexpr pitch round(pitch n)
   {
      return pitch{std::round(n.rep)};
   }

   constexpr pitch ceil(pitch n)
   {
      return pitch{std::ceil(n.rep)};
   }

   constexpr pitch floor(pitch n)
   {
      return pitch{std::floor(n.rep)};
   }
}

#endif

