/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/signal_conditioner.hpp>

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

// Short names for the specializations: the full type is too long to
// declare with inline at every use.
using bypass = q::sc_bypass;
using full_sc  = q::basic_signal_conditioner<>;
using no_clip  = q::basic_signal_conditioner<bypass::clip>;
using no_comp  = q::basic_signal_conditioner<bypass::compressor>;
using bare_sc  = q::basic_signal_conditioner<
                    bypass::clip | bypass::compressor>;
using no_sm    = q::basic_signal_conditioner<bypass::smoother>;
using no_front = q::basic_signal_conditioner<
                    bypass::highpass | bypass::smoother>;
using only_gate = q::basic_signal_conditioner<
                    bypass::highpass | bypass::smoother
                  | bypass::clip | bypass::compressor>;

namespace
{
   constexpr float sps = 44100.0f;
   constexpr auto  lowest = q::frequency{100.0};
   constexpr auto  highest = q::frequency{400.0};

   // A note-like burst: silence, a sharp attack, a decay. Enough to exercise
   // the gate, the clip (the attack overshoots) and the compressor (the decay
   // spans a wide dynamic range).
   std::vector<float> burst(std::size_t n = 22050)
   {
      std::vector<float> in(n, 0.0f);
      auto const quiet = n / 10;
      for (std::size_t i = quiet; i != n; ++i)
      {
         auto t = float(i - quiet) / sps;
         auto amp = 0.9f * std::exp(-6.0f * t);
         in[i] = amp * std::sin(2.0f * 3.14159265f * 200.0f * t);
      }
      return in;
   }

   template <typename SC>
   std::vector<float> run(SC& sc, std::vector<float> const& in)
   {
      std::vector<float> out;
      out.reserve(in.size());
      for (auto s : in)
         out.push_back(sc(s));
      return out;
   }
}

///////////////////////////////////////////////////////////////////////////////
// The alias must be the default specialization, so dependent code that names
// signal_conditioner keeps compiling and behaving exactly as before.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("signal_conditioner is the default basic_signal_conditioner")
{
   static_assert(std::is_same_v<
      q::signal_conditioner, full_sc>);

   // The nested config type must still resolve through the alias.
   auto conf = q::signal_conditioner::config{};
   auto a = q::signal_conditioner{conf, lowest, highest, sps};
   auto b = full_sc{conf, lowest, highest, sps};

   auto in = burst();
   auto ra = run(a, in);
   auto rb = run(b, in);
   CHECK(ra == rb);
}

///////////////////////////////////////////////////////////////////////////////
// Bypassing a stage removes its work; the storage is a bonus, and only a
// partial one. A bypassed member can never grow the object, but alignment
// padding may absorb the saving entirely -- bypassing the 8-byte clip does
// exactly that here, leaving the size unchanged. Only the compressor is big
// enough to show through.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("bypassed stages never cost more than the stage")
{
   CHECK(sizeof(no_clip) <= sizeof(full_sc));
   CHECK(sizeof(no_comp) <= sizeof(full_sc));
   CHECK(sizeof(bare_sc) <= sizeof(no_clip));
   CHECK(sizeof(bare_sc) <= sizeof(no_comp));

   // Bypassing everything must actually save something.
   CHECK(sizeof(bare_sc) < sizeof(full_sc));

   CHECK(sizeof(no_sm) <= sizeof(full_sc));
   CHECK(sizeof(no_front) <= sizeof(no_sm));
}

///////////////////////////////////////////////////////////////////////////////
// The front stages exist to be replaced by an external front-end. With every
// stage bypassed only the gate is left, so the output is the input times the
// gate gain: the chain has nothing else to do.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("bypassing every stage leaves only the gate")
{
   auto conf = q::signal_conditioner::config{};
   auto sc = only_gate{conf, lowest, highest, sps};

   auto in = burst();
   for (auto s : in)
   {
      auto out = sc(s);
      CHECK(out == Approx(s * sc.gate_env()));
      CHECK(sc.signal_env() == Approx(sc.pre_env()));
   }
}

TEST_CASE("bypassing the front stages takes them out of the chain")
{
   auto conf = q::signal_conditioner::config{};
   auto full  = full_sc{conf, lowest, highest, sps};
   auto no_s  = no_sm{conf, lowest, highest, sps};
   auto no_f  = no_front{conf, lowest, highest, sps};

   // The outputs must differ somewhere: a bypassed smoother or highpass is
   // gone, not replaced by an equivalent.
   float d_sm = 0.0f, d_front = 0.0f;
   auto in = burst();
   for (auto s : in)
   {
      auto a = full(s), b = no_s(s), c = no_f(s);
      d_sm    = std::max(d_sm, std::abs(a - b));
      d_front = std::max(d_front, std::abs(b - c));
   }
   CHECK(d_sm > 1e-3f);
   CHECK(d_front > 1e-3f);
}


TEST_CASE("bypassing the compressor leaves signal_env as the plain envelope")
{
   auto conf = q::signal_conditioner::config{};
   auto no_k = no_comp{conf, lowest, highest, sps};

   auto in = burst();
   for (auto s : in)
   {
      no_k(s);
      // With no compressor there is no makeup gain, so the output envelope
      // IS the pre-gain envelope.
      CHECK(no_k.signal_env() == Approx(no_k.pre_env()));
   }
}

///////////////////////////////////////////////////////////////////////////////
// What bypassing actually does to the signal.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("bypassing the compressor removes the makeup gain")
{
   auto conf = q::signal_conditioner::config{};
   auto full = full_sc{conf, lowest, highest, sps};
   auto no_k = no_comp{conf, lowest, highest, sps};

   auto in = burst();
   auto rf = run(full, in);
   auto rk = run(no_k, in);

   auto energy = [](std::vector<float> const& v)
   {
      double a = 0; for (auto x : v) a += double(x) * x; return a;
   };
   // The compressor's makeup gain defaults to 10x, so the compressed output
   // must carry substantially more energy.
   CHECK(energy(rf) > energy(rk) * 4.0);
}

TEST_CASE("bypassing the clip lets the attack through un-saturated")
{
   auto conf = q::signal_conditioner::config{};
   auto full = full_sc{conf, lowest, highest, sps};
   auto no_c = no_clip{conf, lowest, highest, sps};

   auto in = burst();
   auto rf = run(full, in);
   auto rc = run(no_c, in);

   auto peak = [](std::vector<float> const& v)
   {
      float m = 0; for (auto x : v) m = std::max(m, std::abs(x)); return m;
   };
   // The clip saturates crests; without it the peak must be higher.
   CHECK(peak(rc) > peak(rf));
}
