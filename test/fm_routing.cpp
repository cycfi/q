/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/fm/dx7_routing.hpp>
#include <q/synth/fm/fm_routing.hpp>

namespace q = cycfi::q;
using routing = q::fm_routing;
using namespace q::fm_ops;

namespace
{
   // The DX7 chart's carrier counts, algorithms 1..32
   constexpr std::size_t chart_carriers[32] =
   {
      2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
      1, 1, 3, 3, 4, 4, 4, 5, 5, 3, 3, 3, 4, 4, 5, 6
   };

   constexpr std::size_t count(routing::mask m)
   {
      std::size_t n = 0;
      for (; m; m &= m - 1)
         ++n;
      return n;
   }
}

TEST_CASE("A stack chains to the right")
{
   constexpr auto r = routing{4}(op<4> >> op<3> >> op<2>);
   static_assert(r.valid());
   CHECK(r.get().mod[1] == 0x04);      // operator 3 modulates 2
   CHECK(r.get().mod[2] == 0x08);      // operator 4 modulates 3
   CHECK(r.get().mod[3] == 0x00);
   CHECK(r.carriers() == 0x03);        // 2 tops the stack, 1 is untouched
   CHECK(r.modulators() == 0x0c);
}

TEST_CASE("Paths side by side: several into one, or one into several")
{
   constexpr auto in = routing{4}(
      op<2> >> op<1> | op<3> >> op<1> | op<4> >> op<1>);
   CHECK(in.get().mod[0] == 0x0e);     // 2, 3 and 4 modulate 1
   CHECK(in.carriers() == 0x01);

   constexpr auto out = routing{4}(
      op<4> >> op<1> | op<4> >> op<2> | op<4> >> op<3>);
   CHECK(out.get().mod[0] == 0x08);    // 4 modulates 1, 2 and 3
   CHECK(out.get().mod[1] == 0x08);
   CHECK(out.get().mod[2] == 0x08);
   CHECK(out.carriers() == 0x07);

   // A path may be given in pieces, or all at once
   constexpr auto once = routing{3}(op<3> >> op<2> >> op<1>);
   auto twice = routing{3}(op<3> >> op<2>)(op<2> >> op<1>);
   CHECK(once.get().mod[0] == twice.get().mod[0]);
   CHECK(once.get().mod[1] == twice.get().mod[1]);
}

TEST_CASE("Carriers are derived, so they cannot fall out of step")
{
   CHECK(routing{6}.carriers() == 0x3f);              // all of them
   CHECK(routing{6}(op<2> >> op<1>).carriers() == 0x3d);
   CHECK(routing{2}(op<2> >> op<1>).carriers() == 0x01);

   // Two operators modulating one leave the rest sounding
   constexpr auto r = routing{4}(op<3> >> op<1> | op<4> >> op<1>);
   CHECK(r.get().mod[0] == 0x0c);
   CHECK(r.carriers() == 0x03);
}

TEST_CASE("Feedback is one path, to itself or to another")
{
   auto self = routing{6}.feedback(op<6>);
   CHECK(self.get().fb_src == 5);      // the table is numbered from 0
   CHECK(self.get().fb_dst == 5);

   auto across = routing{6}.feedback(op<4>, op<6>);
   CHECK(across.get().fb_src == 3);
   CHECK(across.get().fb_dst == 5);
}

TEST_CASE("A routing is invalid if a path is out of range or misordered")
{
   CHECK(routing{6}(op<6> >> op<5> >> op<4>).valid());
   CHECK(!routing{6}(op<1> >> op<2>).valid());        // a modulator below
   CHECK(!routing{6}(op<2> >> op<2>).valid());        // itself
   CHECK(!routing{6}(op<7> >> op<1>).valid());   // past the count
   CHECK(!routing{2}(op<6> >> op<5>).valid());        // not its operators
   CHECK(!routing{6}.feedback(op<9>).valid());
   CHECK(!routing{q::fm_max_operators + 1}.valid());
}

TEST_CASE("The DX7 set matches the chart's carrier counts")
{
   for (int a = 1; a <= 32; ++a)
   {
      auto r = q::dx7_routing[a - 1];
      CHECK(r.valid());
      CHECK(count(r.carriers()) == chart_carriers[a - 1]);
      CHECK((r.carriers() & 1) == 1);             // OP1 always sounds
   }

   // Algorithm 1: 2 -> 1, and 6 -> 5 -> 4 -> 3, with OP6 feeding back
   constexpr auto one = q::dx7_routing[0];
   CHECK(one.get().mod[0] == 0x02);               // OP2 modulates OP1
   CHECK(one.get().mod[2] == 0x08);               // OP4 modulates OP3
   CHECK(one.get().mod[3] == 0x10);               // OP5 modulates OP4
   CHECK(one.get().mod[4] == 0x20);               // OP6 modulates OP5
   CHECK(one.carriers() == 0x05);                 // OP1 and OP3
   CHECK(one.get().fb_src == 5);                  // OP6

   // 4 and 6 are the two that loop across operators
   CHECK(q::dx7_routing[3].get().fb_src == 3);    // OP4
   CHECK(q::dx7_routing[3].get().fb_dst == 5);    // into OP6
   CHECK(q::dx7_routing[5].get().fb_src == 4);    // OP5
   CHECK(q::dx7_routing[5].get().fb_dst == 5);    // into OP6

   // 32 is six carriers, 18 is one
   CHECK(count(q::dx7_routing[31].carriers()) == 6);
   CHECK(count(q::dx7_routing[17].carriers()) == 1);
}
