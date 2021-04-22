/*=============================================================================
   Copyright (C) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOTE_HPP_APRIL_22_2021)
#define CYCFI_Q_NOTE_HPP_APRIL_22_2021

#include <q/support/frequency.hpp>
#include <q/support/value.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   struct interval : value<double, interval>
   {
      using base_type = value<double, interval>;
      using base_type::base_type;

      constexpr            interval(double val) : base_type(val) {}

      constexpr operator   float() const     { return rep; }
      constexpr operator   double() const    { return rep; }
   };

   ////////////////////////////////////////////////////////////////////////////
   struct note
   {
      constexpr static auto base_frequency = frequency{8.1757989156437};

      constexpr            note() : rep(0.0f) {}
      explicit             note(frequency f);
      constexpr            note(int val) : rep(val) {}
      constexpr            note(float val) : rep(val) {}
      constexpr            note(double val) : rep(val) {}

      constexpr explicit   operator bool() const   { return rep > 0;}
      constexpr bool       valid() const           { return rep > 0;}

      constexpr note       operator+() const       { return {rep}; }
      constexpr note       operator-() const       { return {-rep}; }

      constexpr note&      operator+=(interval b)  { rep += b.rep; return *this; }
      constexpr note&      operator-=(interval b)  { rep -= b.rep; return *this; }

      double rep = 0.0f;
   };

   // Free functions
   inline frequency  as_frequency(note n);

   constexpr note    operator-(note a, interval b);
   constexpr note    operator+(interval a, note b);
   constexpr note    operator+(note a, interval b);

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
   inline note::note(frequency f)
    : rep{12 * std::log2(as_double(f / base_frequency))}
   {}

   inline frequency as_frequency(note n)
   {
      return note::base_frequency*fast_pow2(n.rep / 12);
   }

   constexpr note operator-(note a, interval b)
   {
      return note{a.rep - double(b)};
   }

   constexpr note operator+(interval a, note b)
   {
      return note{double(a)+b.rep};
   }

   constexpr note operator+(note a, interval b)
   {
      return note{a.rep + double(b)};
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
}

#endif

