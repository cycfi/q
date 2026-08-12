/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/bypassable.hpp>

#include <type_traits>

namespace q = cycfi::q;

namespace
{
   // A stage with no default constructor -- the case that forces the
   // factory to exist, since a member-init list cannot hold `if constexpr`.
   struct gain
   {
      explicit gain(float g) : value{g} {}
      float value;
   };

   template <bool Bypass>
   struct host
   {
      q::bypassable<Bypass, gain> _gain;
      double                      _other;

      host() : _gain{q::make_bypassable<Bypass, gain>(2.0f)}, _other{0} {}
   };
}

TEST_CASE("bypassable resolves to the type when present")
{
   static_assert(std::is_same_v<q::bypassable<false, gain>, gain>);
}

TEST_CASE("bypassable resolves to an empty stand-in when bypassed")
{
   using b = q::bypassable<true, gain>;
   static_assert(!std::is_same_v<b, gain>);
   static_assert(std::is_empty_v<b>);
}

TEST_CASE("a bypassed member never costs more than the stage")
{
   // The stand-in is empty, so a bypassed member can only shrink the host or
   // leave it unchanged -- never grow it. It is NOT a guarantee of a smaller
   // object: an empty class still occupies a byte, and alignment padding can
   // absorb that byte plus the whole of a small stage. Here gain is 4 bytes
   // beside an 8-byte double, so both hosts come to 16.
   CHECK(sizeof(host<true>) <= sizeof(host<false>));
}

TEST_CASE("make_bypassable constructs a type that has no default constructor")
{
   auto present = q::make_bypassable<false, gain>(3.5f);
   CHECK(present.value == 3.5f);

   auto absent = q::make_bypassable<true, gain>(3.5f);
   static_assert(std::is_empty_v<decltype(absent)>);
}

TEST_CASE("bypassable works through a host with a bypassed member")
{
   host<false> a;
   CHECK(a._gain.value == 2.0f);

   host<true> b;          // must still construct, ignoring the argument
   CHECK(b._other == 0);
}
