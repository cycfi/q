/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <q/detail/db_table.hpp>
#include <q/support/concepts.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // decibel is a highly optimized class for dealing with decibels. The
   // class provides fast conversion from linear to decibel and back. The
   // decibel value is non-linear. It operates on the logarithmic domain. The
   // decibel class is perfectly suitable for dynamics processing (e.g.
   // compressors and limiters and envelopes). Q provides fast decibel
   // computations using lookup tables and fast math computations for
   // converting to and from scalars.
   ////////////////////////////////////////////////////////////////////////////
   struct decibel
   {
      struct _direct {};
      constexpr static _direct direct = {};

      constexpr            decibel();
      explicit             decibel(double val);
      constexpr            decibel(double val, _direct);

      constexpr decibel    operator+() const;
      constexpr decibel    operator-() const;

      constexpr decibel&   operator+=(decibel b);
      constexpr decibel&   operator+=(concepts::arithmetic_scalar auto b);

      constexpr decibel&   operator-=(decibel b);
      constexpr decibel&   operator-=(concepts::arithmetic_scalar auto b);

      constexpr decibel&   operator*=(concepts::arithmetic_scalar auto b);
      constexpr decibel&   operator/=(concepts::arithmetic_scalar auto b);

      double rep = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Free functions
   ////////////////////////////////////////////////////////////////////////////
   double               as_double(decibel db);
   constexpr float      as_float(decibel db);

   constexpr decibel    operator+(decibel a, decibel b);
   constexpr decibel    operator+(decibel a, concepts::arithmetic_scalar auto b);
   constexpr decibel    operator+(concepts::arithmetic_scalar auto a, decibel b);

   constexpr decibel    operator-(decibel a, decibel b);
   constexpr decibel    operator-(decibel a, concepts::arithmetic_scalar auto b);
   constexpr decibel    operator-(concepts::arithmetic_scalar auto a, decibel b);

   constexpr decibel    operator*(decibel a, concepts::arithmetic_scalar auto b);
   constexpr decibel    operator*(concepts::arithmetic_scalar auto a, decibel b);

   constexpr double     operator/(decibel a, decibel b);
   constexpr decibel    operator/(decibel a, concepts::arithmetic_scalar auto b);

   constexpr bool       operator==(decibel a, decibel b);
   constexpr bool       operator!=(decibel a, decibel b);
   constexpr bool       operator<(decibel a, decibel b);
   constexpr bool       operator<=(decibel a, decibel b);
   constexpr bool       operator>(decibel a, decibel b);
   constexpr bool       operator>=(decibel a, decibel b);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline double as_double(decibel db)
   {
      return std::pow(10, db.rep/20);
   }

   constexpr float as_float(decibel db)
   {
      return detail::db2a(db.rep);
   }

   inline decibel approx_db(float val)
   {
      return decibel{20.0f * faster_log10(val), decibel::direct};
   }

   constexpr decibel::decibel()
    : rep(0.0f)
   {
   }

   inline decibel::decibel(double val)
    : rep(20.0f * fast_log10(val))
   {
   }

   constexpr decibel::decibel(double val, _direct)
    : rep(val)
   {
   }

   constexpr decibel decibel::operator+() const
   {
      return {rep, direct };
   }

   constexpr decibel decibel::operator-() const
   {
      return {-rep, direct };
   }

   constexpr decibel& decibel::operator+=(decibel b)
   {
      rep += b.rep;
      return *this;
   }

   constexpr decibel& decibel::operator+=(concepts::arithmetic_scalar auto b)
   {
      rep += b;
      return *this;
   }

   constexpr decibel& decibel::operator-=(decibel b)
   {
      rep -= b.rep;
      return *this;
   }

   constexpr decibel& decibel::operator-=(concepts::arithmetic_scalar auto b)
   {
      rep -= b;
      return *this;
   }

   constexpr decibel& decibel::operator*=(concepts::arithmetic_scalar auto b)
   {
      rep *= b; return *this;
   }

   constexpr decibel& decibel::operator/=(concepts::arithmetic_scalar auto b)
   {
      rep /= b; return *this;
   }

   constexpr decibel operator+(decibel a, decibel b)
   {
      return decibel{a.rep + b.rep, decibel::direct};
   }

   constexpr decibel operator+(decibel a, concepts::arithmetic_scalar auto b)
   {
      return decibel{a.rep + b, decibel::direct};
   }

   constexpr decibel operator+(concepts::arithmetic_scalar auto a, decibel b)
   {
      return decibel{a + b.rep, decibel::direct};
   }

   constexpr decibel operator-(decibel a, decibel b)
   {
      return decibel{a.rep - b.rep, decibel::direct};
   }

   constexpr decibel operator-(decibel a, concepts::arithmetic_scalar auto b)
   {
      return decibel{a.rep - b, decibel::direct};
   }

   constexpr decibel operator-(concepts::arithmetic_scalar auto a, decibel b)
   {
      return decibel{a - b.rep, decibel::direct};
   }

   constexpr decibel operator*(decibel a, concepts::arithmetic_scalar auto b)
   {
      return decibel{a.rep * b, decibel::direct};
   }

   constexpr decibel operator*(concepts::arithmetic_scalar auto a, decibel b)
   {
      return decibel{a * b.rep, decibel::direct};
   }

   constexpr double operator/(decibel a, decibel b)
   {
      return a.rep / b.rep;
   }

   constexpr decibel operator/(decibel a, concepts::arithmetic_scalar auto b)
   {
      return decibel{a.rep / b, decibel::direct};
   }

   constexpr bool operator==(decibel a, decibel b)
   {
      return a.rep == b.rep;
   }

   constexpr bool operator!=(decibel a, decibel b)
   {
      return a.rep != b.rep;
   }

   constexpr bool operator<(decibel a, decibel b)
   {
      return a.rep < b.rep;
   }

   constexpr bool operator<=(decibel a, decibel b)
   {
      return a.rep <= b.rep;
   }

   constexpr bool operator>(decibel a, decibel b)
   {
      return a.rep > b.rep;
   }

   constexpr bool operator>=(decibel a, decibel b)
   {
      return a.rep >= b.rep;
   }
}

#endif
