/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_VALUE_HPP_MAY_21_2018)
#define CYCFI_Q_VALUE_HPP_MAY_21_2018

#include <q/support/basic_concepts.hpp>

namespace cycfi::q
{
   namespace concepts
   {
      template <typename A, typename B>
      concept SameUnit = std::same_as<typename A::unit_type, typename B::unit_type>;
   }

   ////////////////////////////////////////////////////////////////////////////
   // On binary operations `a + b` and `a - b`, where `a` and `b` conform to
   // the `SameUnit` concept (see above), the resuling type will be whichever
   // has the `value_type` of `decltype(a.ref + b.rep)`, else if both
   // operands are promoted, then whichever has the larger `value_type` will
   // be chosen.
   //
   // Promotion logic:
   //
   // If decltype(a.rep + b.rep) is the same as a.rep choose A. Else if
   // decltype(a.rep + b.rep) is the same as b.rep choose B. Else if the
   // sizeof(a.rep) >= sizeof(b.rep) choose A. Else, choose B.
   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   using promote_unit =
      std::conditional_t<
         std::is_same_v<
            decltype(typename A::value_type{} + typename B::value_type{})
          , typename A::value_type>
       , A
       , std::conditional_t<
            std::is_same_v<
               decltype(typename B::value_type{} + typename A::value_type{})
             , typename B::value_type>
          , B
          , std::conditional_t<
               sizeof(typename B::value_type) >= sizeof(typename B::value_type)
             , A
             , B
            >
         >
      >;

   struct direct_unit_type {};
   constexpr static direct_unit_type direct_unit = {};

   ////////////////////////////////////////////////////////////////////////////
   // unit: Unit abstraction and encapsulation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   struct unit
   {
      using derived_type = Derived;
      using value_type = T;
                                    // Temporary constructor. This is not
                                    // marked deprecated because we will use
                                    // this for now.
      constexpr                     unit(T val, direct_unit_type) : rep(val) {}

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

                                    template <typename U> requires concepts::SameUnit<unit, U>
      constexpr unit&               operator=(U rhs) { rep = rhs.rep; return derived(); }

                                    template <typename U> requires concepts::SameUnit<unit, U>
      constexpr unit&               operator+=(U rhs) { rep += rhs.rep; return derived(); }

                                    template <typename U> requires concepts::SameUnit<unit, U>
      constexpr unit&               operator-=(U rhs) { rep -= rhs.rep; return derived(); }

      T rep;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Free functions
   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator==(A a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator==(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator==(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator!=(A a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator!=(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator!=(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator<(A a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator<(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator<(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator<=(A a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator<=(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator<=(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator>(A a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator>(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator>(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator>=(A a, B b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr bool operator>=(A a, unit<B, Derived> b);

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr bool operator>=(unit<A, Derived> a, B b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr promote_unit<A, B> operator+(A a, B b);

   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr promote_unit<A, B> operator-(A a, B b);

   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr typename promote_unit<A, B>::value_type operator/(A a, B b);

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
      return derived_type{-rep, direct_unit};
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator==(A a, B b)
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
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator!=(A a, B b)
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
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator<(A a, B b)
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
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator<=(A a, B b)
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
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator>(A a, B b)
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
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr bool operator>=(A a, B b)
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
   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr promote_unit<A, B> operator+(A a, B b)
   {
      return promote_unit<A, B>{a.rep + b.rep, direct_unit};
   }

   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr promote_unit<A, B> operator-(A a, B b)
   {
      return promote_unit<A, B>{a.rep - b.rep, direct_unit};
   }

   template <typename A, typename B>
   requires concepts::SameUnit<A, B>
   constexpr typename promote_unit<A, B>::value_type
   operator/(A a, B b)
   {
      return a.rep / b.rep;
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator+(A a, unit<B, Derived> b)
   {
      return Derived{a + b.rep, direct_unit};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator-(A a, unit<B, Derived> b)
   {
      return Derived{a - b.rep, direct_unit};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator*(A a, unit<B, Derived> b)
   {
      return Derived{a * b.rep, direct_unit};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<A>
   constexpr Derived operator/(A a, unit<B, Derived> b)
   {
      return Derived{a / b.rep, direct_unit};
   }

   ////////////////////////////////////////////////////////////////////////////
   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator+(unit<A, Derived> a, B b)
   {
      return Derived{a.rep + b, direct_unit};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator-(unit<A, Derived> a, B b)
   {
      return Derived{a.rep - b, direct_unit};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator*(unit<A, Derived> a, B b)
   {
      return Derived{a.rep * b, direct_unit};
   }

   template <typename A, typename B, typename Derived>
   requires concepts::Arithmetic<B>
   constexpr Derived operator/(unit<A, Derived> a, B b)
   {
      return Derived{a.rep / b, direct_unit};
   }
}

#endif
