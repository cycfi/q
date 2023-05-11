/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_VALUE_HPP_MAY_21_2018)
#define CYCFI_Q_VALUE_HPP_MAY_21_2018

#include <type_traits>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // value: Value abstraction and encapsulation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   struct value
   {
      using derived_type = Derived;
      using value_type = T;

      constexpr explicit            value(T val = T(0)) : rep(val) {}
      constexpr                     value(value const&) = default;
      constexpr                     value(value&&) = default;

      constexpr value&              operator=(value const&) = default;
      constexpr value&              operator=(value&&) = default;

      constexpr explicit operator   T() const;
      constexpr derived_type        operator+() const;
      constexpr derived_type        operator-() const;

      constexpr derived_type&       operator+=(value rhs);
      constexpr derived_type&       operator+=(T rhs);
      constexpr derived_type&       operator-=(value rhs);
      constexpr derived_type&       operator-=(T rhs);
      constexpr derived_type&       operator*=(T b);
      constexpr derived_type&       operator/=(T b);

      constexpr derived_type const& derived() const;
      constexpr derived_type&       derived();

      T rep;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr value<T, Derived>::operator T() const
   {
      return rep;
   }

   template <typename T, typename Derived>
   constexpr Derived value<T, Derived>::operator+() const
   {
      return derived();
   }

   template <typename T, typename Derived>
   constexpr Derived value<T, Derived>::operator-() const
   {
      return derived_type{-rep};
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr typename value<T, Derived>::derived_type const&
   value<T, Derived>::derived() const
   {
      return *static_cast<derived_type const*>(this);
   }

   template <typename T, typename Derived>
   constexpr typename value<T, Derived>::derived_type&
   value<T, Derived>::derived()
   {
      return *static_cast<derived_type*>(this);
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator==(value<A, DerivedA> a, value<B, DerivedB> b)
   {
      return a.rep == b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, bool
   >::type
   operator==(A a, value<B, Derived> b)
   {
      return a == b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr bool operator==(value<A, Derived> a, B b)
   {
      return a.rep == b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator!=(value<A, DerivedA> a, value<B, DerivedB> b)
   {
      return a.rep != b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, bool
   >::type
   operator!=(A a, value<B, Derived> b)
   {
      return a != b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr bool operator!=(value<A, Derived> a, B b)
   {
      return a.rep != b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator<(value<A, DerivedA> a, value<B, DerivedB> b)
   {
      return a.rep < b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, bool
   >::type
   operator<(A a, value<B, Derived> b)
   {
      return a < b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<B>::value, bool
   >::type
   operator<(value<A, Derived> a, B b)
   {
      return a.rep < b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr bool operator<=(value<A, Derived> a, value<B, Derived> b)
   {
      return a.rep <= b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, bool
   >::type
   operator<=(A a, value<B, Derived> b)
   {
      return a <= b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<B>::value, bool
   >::type
   operator<=(value<A, Derived> a, B b)
   {
      return a.rep <= b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator>(value<A, DerivedA> a, value<B, DerivedB> b)
   {
      return a.rep > b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, bool
   >::type
   operator>(A a, value<B, Derived> b)
   {
      return a > b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr bool operator>(value<A, Derived> a, B b)
   {
      return a.rep > b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename DerivedA, typename DerivedB>
   constexpr bool operator>=(value<A, DerivedA> a, value<B, DerivedB> b)
   {
      return a.rep >= b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, bool
   >::type
   operator>=(A a, value<B, Derived> b)
   {
      return a >= b.rep;
   }

   template <typename A, typename B, typename Derived>
   constexpr bool operator>=(value<A, Derived> a, B b)
   {
      return a.rep >= b;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& value<T, Derived>::operator+=(value<T, Derived> rhs)
   {
      rep += rhs.rep;
      return derived();
   }

   template <typename T, typename Derived>
   constexpr Derived& value<T, Derived>::operator+=(T rhs)
   {
      rep += rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& value<T, Derived>::operator-=(value<T, Derived> rhs)
   {
      rep -= rhs.rep;
      return derived();
   }

   template <typename T, typename Derived>
   constexpr Derived& value<T, Derived>::operator-=(T rhs)
   {
      rep -= rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& value<T, Derived>::operator*=(T rhs)
   {
      rep *= rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived& value<T, Derived>::operator/=(T rhs)
   {
      rep /= rhs;
      return derived();
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr Derived operator+(value<A, Derived> a, value<B, Derived> b)
   {
      return Derived(a.rep + b.rep);
   }

   template <typename A, typename B, typename Derived>
   constexpr Derived operator-(value<A, Derived> a, value<B, Derived> b)
   {
      return Derived(a.rep - b.rep);
   }

   template <typename T, typename Derived>
   constexpr T operator/(value<T, Derived> a, value<T, Derived> b)
   {
      return a.rep / b.rep;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, Derived
   >::type
   operator+(A a, value<B, Derived> b)
   {
      return Derived(a + b.rep);
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
     std::is_arithmetic<A>::value, Derived
   >::type
   operator-(A a, value<B, Derived> b)
   {
      return Derived(a - b.rep);
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, Derived
   >::type
   operator*(A a, value<B, Derived> b)
   {
      return Derived(a * b.rep);
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<A>::value, Derived
   >::type
   operator/(A a, value<B, Derived> b)
   {
      return Derived(a / b.rep);
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<B>::value, Derived
   >::type
   operator+(value<A, Derived> a, B b)
   {
      return Derived(a.rep + b);
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<B>::value, Derived
   >::type
   operator-(value<A, Derived> a, B b)
   {
      return Derived(a.rep - b);
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<B>::value, Derived
   >::type
   operator*(value<A, Derived> a, B b)
   {
      return Derived(a.rep * b);
   }

   template <typename A, typename B, typename Derived>
   constexpr typename std::enable_if<
      std::is_arithmetic<B>::value, Derived
   >::type
   operator/(value<A, Derived> a, B b)
   {
      return Derived(a.rep / b);
   }
}

#endif
