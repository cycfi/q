/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_VALUE_HPP_MAY_21_2018)
#define CYCFI_Q_VALUE_HPP_MAY_21_2018

#include <q/support/concepts.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // unit: Unit abstraction and encapsulation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   struct unit
   {
      using derived_type = Derived;
      using value_type = T;

      constexpr                     unit(T val) : rep(val) {}
      constexpr                     unit(unit const&) = default;
      constexpr                     unit(unit&&) = default;

      constexpr unit&               operator=(unit const&) = default;
      constexpr unit&               operator=(unit&&) = default;

      constexpr derived_type        operator+() const;
      constexpr derived_type        operator-() const;

      constexpr derived_type&       operator+=(unit rhs);
      constexpr derived_type&       operator+=(concepts::Arithmetic auto rhs);

      constexpr derived_type&       operator-=(unit rhs);
      constexpr derived_type&       operator-=(concepts::Arithmetic auto rhs);

      constexpr derived_type&       operator*=(concepts::Arithmetic auto b);
      constexpr derived_type&       operator/=(concepts::Arithmetic auto b);

      constexpr derived_type const& derived() const;
      constexpr derived_type&       derived();

      T rep;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Free functions
   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator==(unit<A, DerivedA> a, unit<B, DerivedB> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator==(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator==(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator!=(unit<A, DerivedA> a, unit<B, DerivedB> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator!=(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator!=(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator<(unit<A, DerivedA> a, unit<B, DerivedB> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator<(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator<(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr bool operator<=(unit<A, Derived> a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator<=(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator<=(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator>(unit<A, DerivedA> a, unit<B, DerivedB> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator>(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator>(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator>=(unit<A, DerivedA> a, unit<B, DerivedB> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator>=(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator>=(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr Derived operator+(unit<A, Derived> a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   constexpr Derived operator-(unit<A, Derived> a, unit<B, Derived> b);

   template <typename T, typename Derived>
   constexpr T operator/(unit<T, Derived> a, unit<T, Derived> b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator+(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator-(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator*(A a, unit<B, Derived> b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator+(unit<A, Derived> a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator-(unit<A, Derived> a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator*(unit<A, Derived> a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator/(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr typename unit<T, Derived>::derived_type const&
   unit<T, Derived>::derived() const
   {
      return *static_cast<derived_type const*>(this);
   }

   template <typename T, typename Derived>
   constexpr typename unit<T, Derived>::derived_type&
   unit<T, Derived>::derived()
   {
      return *static_cast<derived_type*>(this);
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived unit<T, Derived>::operator+() const
   {
      return derived();
   }

   template <typename T, typename Derived>
   constexpr Derived unit<T, Derived>::operator-() const
   {
      return derived_type{-rep};
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator==(unit<A, DerivedA> a, unit<B, DerivedB> b)
   {
      return a.rep == b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator==(A a, unit<B, Derived> b)
   {
      return a == b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator==(unit<A, Derived> a, B b)
   {
      return a.rep == b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator!=(unit<A, DerivedA> a, unit<B, DerivedB> b)
   {
      return a.rep != b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator!=(A a, unit<B, Derived> b)
   {
      return a != b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator!=(unit<A, Derived> a, B b)
   {
      return a.rep != b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator<(unit<A, DerivedA> a, unit<B, DerivedB> b)
   {
      return a.rep < b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator<(A a, unit<B, Derived> b)
   {
      return a < b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator<(unit<A, Derived> a, B b)
   {
      return a.rep < b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr bool operator<=(unit<A, Derived> a, unit<B, Derived> b)
   {
      return a.rep <= b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator<=(A a, unit<B, Derived> b)
   {
      return a <= b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator<=(unit<A, Derived> a, B b)
   {
      return a.rep <= b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator>(unit<A, DerivedA> a, unit<B, DerivedB> b)
   {
      return a.rep > b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator>(A a, unit<B, Derived> b)
   {
      return a > b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator>(unit<A, Derived> a, B b)
   {
      return a.rep > b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator>=(unit<A, DerivedA> a, unit<B, DerivedB> b)
   {
      return a.rep >= b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator>=(A a, unit<B, Derived> b)
   {
      return a >= b.rep;
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator>=(unit<A, Derived> a, B b)
   {
      return a.rep >= b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& unit<T, Derived>::operator+=(unit<T, Derived> rhs)
   {
      rep += rhs.rep;
      return derived();
   }

   template <typename T, typename Derived>
   constexpr Derived& unit<T, Derived>::operator+=(concepts::Arithmetic auto rhs)
   {
      rep += rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& unit<T, Derived>::operator-=(unit<T, Derived> rhs)
   {
      rep -= rhs.rep;
      return derived();
   }

   template <typename T, typename Derived>
   constexpr Derived& unit<T, Derived>::operator-=(concepts::Arithmetic auto rhs)
   {
      rep -= rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& unit<T, Derived>::operator*=(concepts::Arithmetic auto rhs)
   {
      rep *= rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& unit<T, Derived>::operator/=(concepts::Arithmetic auto rhs)
   {
      rep /= rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr Derived operator+(unit<A, Derived> a, unit<B, Derived> b)
   {
      return Derived{a.rep + b.rep};
   }

   template <typename A, typename B, typename Derived>
   constexpr Derived operator-(unit<A, Derived> a, unit<B, Derived> b)
   {
      return Derived{a.rep - b.rep};
   }

   template <typename T, typename Derived>
   constexpr T operator/(unit<T, Derived> a, unit<T, Derived> b)
   {
      return a.rep / b.rep;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator+(A a, unit<B, Derived> b)
   {
      return Derived{a + b.rep};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator-(A a, unit<B, Derived> b)
   {
      return Derived{a - b.rep};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator*(A a, unit<B, Derived> b)
   {
      return Derived{a * b.rep};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator/(A a, unit<B, Derived> b)
   {
      return Derived{a / b.rep};
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator+(unit<A, Derived> a, B b)
   {
      return Derived{a.rep + b};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator-(unit<A, Derived> a, B b)
   {
      return Derived{a.rep - b};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator*(unit<A, Derived> a, B b)
   {
      return Derived{a.rep * b};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator/(unit<A, Derived> a, B b)
   {
      return Derived{a.rep / b};
   }
}

#endif
