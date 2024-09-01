/*=============================================================================
   Copyright (C) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_INTERVAL_HPP_APRIL_22_2021)
#define CYCFI_Q_INTERVAL_HPP_APRIL_22_2021

#include <q/support/frequency.hpp>
#include <q/support/unit.hpp>

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
   struct interval_unit;

   template <typename T>
   struct basic_interval : unit<T, basic_interval<T>>
   {
      using base_type = unit<T, basic_interval<T>>;
      using base_type::base_type;
      using unit_type = interval_unit;

      constexpr explicit   basic_interval(concepts::Arithmetic auto val);
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

   template <typename T>
   requires std::floating_point<T>
   constexpr basic_interval<T> round(basic_interval<T> i);

   template <typename T>
   requires std::floating_point<T>
   constexpr basic_interval<T> ceil(basic_interval<T> i);

   template <typename T>
   requires std::floating_point<T>
   constexpr basic_interval<T> floor(basic_interval<T> i);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   constexpr basic_interval<T>::basic_interval(concepts::Arithmetic auto val)
    : base_type(val)
   {
   }

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

   template <typename T>
   requires std::floating_point<T>
   constexpr basic_interval<T> round(basic_interval<T> i)
   {
      return {std::round(i.rep)};
   }

   template <typename T>
   requires std::floating_point<T>
   constexpr basic_interval<T> ceil(basic_interval<T> i)
   {
      return {std::ceil(i.rep)};
   }

   template <typename T>
   requires std::floating_point<T>
   constexpr basic_interval<T> floor(basic_interval<T> i)
   {
      return {std::floor(i.rep)};
   }
}

#endif

