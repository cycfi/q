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
   struct note
   {
      constexpr static auto base_frequency = frequency{8.1757989156437};

      constexpr            note() : rep(-1.0f) {}
      explicit             note(frequency f);
      constexpr            note(int val) : rep(val) {}
      constexpr            note(float val) : rep(val) {}
      constexpr            note(double val) : rep(val) {}

      constexpr explicit   operator bool() const   { return rep > 0;}
      constexpr bool       valid() const           { return rep > 0;}

                           template <typename T>
      constexpr note&      operator+=(basic_interval<T> b)  { rep += b.rep; return *this; }

                           template <typename T>
      constexpr note&      operator-=(basic_interval<T> b)  { rep -= b.rep; return *this; }

      double rep = 0.0f;
   };

   // Free functions
   inline frequency  as_frequency(note n);
   inline float      as_float(note n);

   template <typename T>
   constexpr note    operator-(note a, basic_interval<T> b);

   template <typename T>
   constexpr note    operator+(basic_interval<T> a, note b);

   template <typename T>
   constexpr note    operator+(note a, basic_interval<T> b);

   constexpr bool    operator==(note a, note b);
   constexpr bool    operator!=(note a, note b);
   constexpr bool    operator<(note a, note b);
   constexpr bool    operator<=(note a, note b);
   constexpr bool    operator>(note a, note b);
   constexpr bool    operator>=(note a, note b);

   constexpr note    round(note n);
   constexpr note    ceil(note n);
   constexpr note    floor(note n);

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

   inline note::note(frequency f)
    : rep{12 * fast_log2(as_double(f / base_frequency))}
   {}

   inline frequency as_frequency(note n)
   {
      return note::base_frequency*fast_pow2(n.rep / 12);
   }

   inline float as_float(note n)
   {
      return n.rep;
   }

   template <typename T>
   constexpr note operator-(note a, basic_interval<T> b)
   {
      return note{a.rep - double(b)};
   }

   template <typename T>
   constexpr note operator+(basic_interval<T> a, note b)
   {
      return note{double(a)+b.rep};
   }

   template <typename T>
   constexpr note operator+(note a, basic_interval<T> b)
   {
      return note{a.rep + as_double(b)};
   }

   constexpr bool operator==(note a, note b)
   {
      return a.rep == b.rep;
   }

   constexpr bool operator!=(note a, note b)
   {
      return a.rep != b.rep;
   }

   constexpr bool operator<(note a, note b)
   {
      return a.rep < b.rep;
   }

   constexpr bool operator<=(note a, note b)
   {
      return a.rep <= b.rep;
   }

   constexpr bool operator>(note a, note b)
   {
      return a.rep > b.rep;
   }

   constexpr bool operator>=(note a, note b)
   {
      return a.rep >= b.rep;
   }

   constexpr note round(note n)
   {
      return {std::round(n.rep)};
   }

   constexpr note ceil(note n)
   {
      return {std::ceil(n.rep)};
   }

   constexpr note floor(note n)
   {
      return {std::floor(n.rep)};
   }
}

#endif

