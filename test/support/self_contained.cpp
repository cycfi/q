/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Each header below comes first in its own unit of the test, so it has to
// compile on its own. Order matters: do not put another Q header above them.
#include <q/support/period.hpp>
#include <q/fx/moving_maximum.hpp>
#include <q/fx/hilbert_quadrature.hpp>

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

namespace q = cycfi::q;

TEST_CASE("Test_period_from_frequency")
{
   auto p = q::period{q::frequency{440.0}};
   CHECK(as_double(p) == Approx(1.0 / 440.0));
}

TEST_CASE("Test_moving_maximum_int")
{
   q::moving_maximum<int> m{4};
   CHECK(m(3) == 3);
   CHECK(m(-5) == 3);
   CHECK(m(7) == 7);
   CHECK(m(1) == 7);
   CHECK(m(2) == 7);
   CHECK(m(0) == 7);
   CHECK(m(0) == 2);
}

TEST_CASE("Test_hilbert_quadrature_alone")
{
   q::hilbert_quadrature hq;
   auto [i, qd] = hq(1.0f);
   CHECK(std::isfinite(i));
   CHECK(std::isfinite(qd));
}
