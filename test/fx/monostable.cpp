/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/monostable.hpp>
#include <q/support/literals.hpp>
#include <cstdint>
#include <string>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   // Feed `trig_len` true samples, then false, and return the outputs as a
   // string of '1' and '0'.
   template <typename M>
   std::string run(M m, int trig_len, int len = 12)
   {
      std::string out;
      for (int i = 0; i < len; ++i)
         out += m(i < trig_len) ? '1' : '0';
      return out;
   }
}

TEST_CASE("monostable: the pulse is n - 1 samples from the trigger sample")
{
   q::monostable m{5_ms, 1000};               // n = 5 samples
   REQUIRE(m.n_samples() == 5);
   CHECK(run(m, 1) == "111100000000");

   for (std::uint32_t n = 2; n < 8; ++n)
   {
      m.n_samples(n);
      auto out = run(m, 1);
      CHECK(out == std::string(n - 1, '1') + std::string(13 - n, '0'));
   }
}

TEST_CASE("monostable: a pulse length of 0 or 1 gives no pulse")
{
   q::monostable m{0_ms, 1000};
   for (std::uint32_t n : {0u, 1u})
   {
      m.n_samples(n);
      CHECK(run(m, 1) == "000000000000");
   }
}

TEST_CASE("retriggerable_monostable: n - 1 samples from the last trigger")
{
   q::retriggerable_monostable m{5_ms, 1000};  // n = 5 samples
   CHECK(run(m, 1) == "111100000000");
   CHECK(run(m, 3) == "111111000000");          // last trigger at sample 2
}
