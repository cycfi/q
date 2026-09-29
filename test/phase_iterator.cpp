/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/phase.hpp>

#include <type_traits>

namespace q = cycfi::q;

namespace
{
   // A quarter of a cycle per step: four steps make one turn.
   constexpr auto sps = 48000.0f;
   constexpr auto quarter = q::frequency(sps / 4);
}

TEST_CASE("phase_iterator: wraps around after a full cycle")
{
   q::phase_iterator i{quarter, sps};
   CHECK(i.first());

   ++i; ++i; ++i;
   CHECK(i.last());
   CHECK(!i.first());

   ++i;
   CHECK(i._phase.rep == q::phase::begin().rep);
   CHECK(i.first());

   --i;                                      // and back through zero
   CHECK(i.last());
}

TEST_CASE("phase_iterator: set changes the step, not the phase")
{
   q::phase_iterator i{quarter, sps};
   ++i;
   auto const at = i._phase;

   i.set(q::frequency(sps / 8), sps);
   CHECK(i._phase.rep == at.rep);
   CHECK(i._step.rep == q::phase(q::frequency(sps / 8), sps).rep);
}

TEST_CASE("one_shot_phase_iterator: stops at the end of the cycle")
{
   q::one_shot_phase_iterator i{quarter, sps};

   ++i; ++i; ++i;
   CHECK(i._phase.rep != q::phase::end().rep);

   ++i;
   CHECK(i._phase.rep == q::phase::end().rep);
   CHECK(i.last());

   // It stays there, however far it is pushed.
   for (int n = 0; n != 10; ++n)
      i++;
   CHECK(i._phase.rep == q::phase::end().rep);
}

TEST_CASE("one_shot_phase_iterator: stops at the start going down")
{
   q::one_shot_phase_iterator i{quarter, sps};
   ++i;
   --i;
   CHECK(i._phase.rep == q::phase::begin().rep);

   --i;
   i--;
   CHECK(i._phase.rep == q::phase::begin().rep);
   CHECK(i.first());
}

TEST_CASE("one_shot_phase_iterator: steps like phase_iterator until then")
{
   auto const f = q::frequency(1234.5);
   q::phase_iterator a{f, sps};
   q::one_shot_phase_iterator b{f, sps};

   while (!b.last())
   {
      CHECK(a._phase.rep == b._phase.rep);
      ++a;
      ++b;
   }
}

TEST_CASE("one_shot_phase_iterator: restarts when assigned begin()")
{
   q::one_shot_phase_iterator i{quarter, sps};
   for (int n = 0; n != 6; ++n)
      ++i;
   CHECK(i._phase.rep == q::phase::end().rep);

   i = i.begin();                            // still a one-shot
   CHECK(i.first());
   for (int n = 0; n != 6; ++n)
      ++i;
   CHECK(i._phase.rep == q::phase::end().rep);
}

TEST_CASE("one_shot_phase_iterator: begin, end and middle stay one-shot")
{
   q::one_shot_phase_iterator i{quarter, sps};

   auto b = i.begin();
   static_assert(std::is_same_v<decltype(b), q::one_shot_phase_iterator>);
   for (int n = 0; n != 6; ++n)
      ++b;
   CHECK(b._phase.rep == q::phase::end().rep);

   auto m = i.middle();
   CHECK(m._phase.rep == q::phase::middle().rep);
   for (int n = 0; n != 4; ++n)
      ++m;
   CHECK(m._phase.rep == q::phase::end().rep);

   auto e = i.end();
   ++e;
   CHECK(e._phase.rep == q::phase::end().rep);
   for (int n = 0; n != 6; ++n)
      --e;
   CHECK(e._phase.rep == q::phase::begin().rep);
}
