/*=============================================================================
   Copyright (C) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOTE_HPP_APRIL_22_2021)
#define CYCFI_Q_NOTE_HPP_APRIL_22_2021

#include <q/support/frequency.hpp>
#include <q/support/value.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // An interval is the distance between two pitches, measured in semitones.
   // It is the basis for melody and harmony as well as all musical scales
   // and chords. The `basic_interval<T>` is a template class, parameterized
   // by the underlying type `T`.
   //
   // There are two basic type instantiations: `interval` and
   // `exact_interval`. `interval` is fractional and can represent microtones
   // —intervals smaller than a semitone. exact_interval`deals with exact,
   // whole number intervals only.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_interval : value<T, basic_interval<T>>
   {
      using base_type = value<T, basic_interval<T>>;
      using base_type::base_type;

      constexpr explicit   basic_interval(T val) : base_type(val) {}
   };

   using interval = basic_interval<double>;
   using exact_interval = basic_interval<std::int8_t>;

   // Free functions
   template <typename T>
   constexpr int as_int(basic_interval<T> i);

   template <typename T>
   constexpr float as_float(basic_interval<T> i);

   template <typename T>
   constexpr double as_double(basic_interval<T> i);

   ////////////////////////////////////////////////////////////////////////////
   // `pitch` is determined by its position on the chromatic scale, which is
   // a system of 12 notes that repeat in octaves. The distance between each
   // pitch on the chromatic scale is a semitone, and each pitch represents a
   // specific frequency measured in hertz (Hz).
   //
   // The `pitch` struct includes construction from `frequency` as well as
   // scalars representing the absolute position in the chromatic scale from
   // the base frequency of `8.1757989156437` Hz, which is an octave below
   // F#0. The constructors support both fixed (integer) positions (e.g. 48
   // semitones) above the base frequency, and fractional positions (e.g.
   // 120.6 semitones) above the base frequency.
   //
   // The `pitch` struct also includes conversions to `frequency`. Q provides
   // fast `pitch` computations using fast math functions.
   ////////////////////////////////////////////////////////////////////////////
   struct pitch
   {
      constexpr static auto base_frequency = frequency{8.1757989156437};

      constexpr            pitch() : rep(-1.0f) {}
      explicit             pitch(frequency f);
      constexpr            pitch(int val) : rep(val) {}
      constexpr            pitch(float val) : rep(val) {}
      constexpr            pitch(double val) : rep(val) {}

      constexpr explicit   operator bool() const   { return rep > 0;}
      constexpr bool       valid() const           { return rep > 0;}

                           template <typename T>
      constexpr pitch&     operator+=(basic_interval<T> b)  { rep += b.rep; return *this; }

                           template <typename T>
      constexpr pitch&     operator-=(basic_interval<T> b)  { rep -= b.rep; return *this; }

      double rep = 0.0f;
   };

   // Free functions
   inline frequency  as_frequency(pitch n);
   inline float      as_float(pitch n);
   inline double     as_double(pitch n);

   template <typename T>
   constexpr pitch    operator-(pitch a, basic_interval<T> b);

   template <typename T>
   constexpr pitch    operator+(basic_interval<T> a, pitch b);

   template <typename T>
   constexpr pitch    operator+(pitch a, basic_interval<T> b);

   constexpr bool    operator==(pitch a, pitch b);
   constexpr bool    operator!=(pitch a, pitch b);
   constexpr bool    operator<(pitch a, pitch b);
   constexpr bool    operator<=(pitch a, pitch b);
   constexpr bool    operator>(pitch a, pitch b);
   constexpr bool    operator>=(pitch a, pitch b);

   constexpr pitch    round(pitch n);
   constexpr pitch    ceil(pitch n);
   constexpr pitch    floor(pitch n);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   // Free functions
   template <typename T>
   constexpr int as_int(basic_interval<T> i)
   {
      return i.rep;
   }

   template <typename T>
   constexpr float as_float(basic_interval<T> i)
   {
      return i.rep;
   }

   template <typename T>
   constexpr double as_double(basic_interval<T> i)
   {
      return i.rep;
   }

   inline pitch::pitch(frequency f)
    : rep{12 * fast_log2(as_double(f / base_frequency))}
   {}

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

   template <typename T>
   constexpr pitch operator-(pitch a, basic_interval<T> b)
   {
      return pitch{a.rep - double(b)};
   }

   template <typename T>
   constexpr pitch operator+(basic_interval<T> a, pitch b)
   {
      return pitch{double(a) + b.rep};
   }

   template <typename T>
   constexpr pitch operator+(pitch a, basic_interval<T> b)
   {
      return pitch{a.rep + as_double(b)};
   }

   constexpr bool operator==(pitch a, pitch b)
   {
      return a.rep == b.rep;
   }

   constexpr bool operator!=(pitch a, pitch b)
   {
      return a.rep != b.rep;
   }

   constexpr bool operator<(pitch a, pitch b)
   {
      return a.rep < b.rep;
   }

   constexpr bool operator<=(pitch a, pitch b)
   {
      return a.rep <= b.rep;
   }

   constexpr bool operator>(pitch a, pitch b)
   {
      return a.rep > b.rep;
   }

   constexpr bool operator>=(pitch a, pitch b)
   {
      return a.rep >= b.rep;
   }

   constexpr pitch round(pitch n)
   {
      return {std::round(n.rep)};
   }

   constexpr pitch ceil(pitch n)
   {
      return {std::ceil(n.rep)};
   }

   constexpr pitch floor(pitch n)
   {
      return {std::floor(n.rep)};
   }
}

#endif

