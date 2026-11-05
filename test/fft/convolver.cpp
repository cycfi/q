/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fft/convolver.hpp>

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <cmath>
#include <random>
#include <vector>

namespace q = cycfi::q;

namespace
{
   std::vector<float> noise(std::size_t n, unsigned seed)
   {
      std::mt19937 rng{seed};
      std::uniform_real_distribution<float> u{-1.0f, 1.0f};
      std::vector<float> x(n);
      for (auto& s : x)
         s = u(rng);
      return x;
   }

   // A decaying noise burst, as a room or a cabinet's IR is.
   std::vector<float> some_ir(std::size_t n)
   {
      auto ir = noise(n, 5);
      for (std::size_t i = 0; i != n; ++i)
         ir[i] *= std::exp(-4.0f * float(i) / float(n));
      return ir;
   }

   // The largest difference from direct convolution, against the output's
   // peak.
   double error(std::vector<float> const& ir, std::size_t block)
   {
      auto const x = noise(6000, 9);
      q::convolver conv{ir, block};
      double worst = 0, peak = 0;
      for (std::size_t n = 0; n != x.size(); ++n)
      {
         double direct = 0;
         for (std::size_t t = 0; t != ir.size() && t <= n; ++t)
            direct += double(ir[t]) * x[n - t];
         worst = std::max(worst, std::abs(conv(x[n]) - direct));
         peak = std::max(peak, std::abs(direct));
      }
      return worst / peak;
   }
}

TEST_CASE("convolver: is direct convolution, with no latency", "[convolver]")
{
   // Shorter than a block, a block exactly, a block and one, many blocks
   // with a partial last one.
   for (std::size_t len : {7, 64, 65, 1000, 3000})
   {
      for (std::size_t block : {16, 64})
      {
         INFO("IR " << len << ", block " << block);
         CHECK(error(some_ir(len), block) < 1e-5);
      }
   }
}

TEST_CASE("convolver: a unit impulse passes the input through", "[convolver]")
{
   auto const x = noise(1000, 3);
   q::convolver conv{std::vector<float>{1.0f}, 32};
   for (auto s : x)
      REQUIRE(conv(s) == s);
}

TEST_CASE("convolver: reset clears the history", "[convolver]")
{
   auto const ir = some_ir(500);
   auto const x = noise(800, 4);
   q::convolver a{ir, 32};
   q::convolver b{ir, 32};
   for (auto s : x)
      a(s);
   a.reset();
   for (auto s : x)
      REQUIRE(a(s) == b(s));
   CHECK(a.block() == 32);
   CHECK(a.length() == 500);
}
