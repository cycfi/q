/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fx/transient_detector.hpp>
#include <q_io/audio_file.hpp>

#include <cmath>
#include <string>
#include <vector>

namespace q = cycfi::q;

namespace
{
   std::vector<float> load(std::string const& name, float& sps)
   {
      q::wav_reader src{"audio_files/" + name + ".wav"};
      REQUIRE(src);
      sps = src.sps();
      std::vector<float> x(src.length());
      src.read(x);
      return x;
   }

   std::vector<double> fires(std::vector<float> const& x, float sps)
   {
      q::transient_detector d{sps};
      std::vector<double> at;
      for (std::size_t t = 0; t != x.size(); ++t)
      {
         if (d(x[t]))
            at.push_back(t / double(sps));
      }
      return at;
   }
}

TEST_CASE("transient_detector: one fire, at the release")
{
   // Single notes on Nu Multi pickups. The release times are where the
   // waveform first rises past four times the level of the 50 ms before
   // it: the pick leaving the string, after it has pressed on it.
   struct note { char const* name; double release; };
   note const notes[] = {
      {"1a-Low-E", 1.0021}, {"3a-D", 0.9843}, {"4a-G", 0.9483}
    , {"harmonic Low E", 0.7761}, {"harmonic D", 0.3964}
    , {"harmonic G", 0.3155}
   };
   for (auto const& n : notes)
   {
      float sps;
      auto const x = load(n.name, sps);
      auto at = fires(x, sps);
      INFO(n.name);
      REQUIRE(at.size() == 1);
      CHECK(at[0] == Approx(n.release).margin(0.001));
   }
}

TEST_CASE("transient_detector: picks in a staccato line")
{
   // The first eight picks of a staccato line, found the same way.
   double const releases[] = {
      0.5344, 0.9625, 1.8621, 2.3127, 2.7689, 2.9765, 3.2105, 3.4317
   };
   float sps;
   auto const x = load("GStaccato", sps);
   auto at = fires(x, sps);
   REQUIRE(at.size() >= 8);
   for (std::size_t i = 0; i != 8; ++i)
      CHECK(at[i] == Approx(releases[i]).margin(0.001));
}

TEST_CASE("transient_detector: one fire a refractory time")
{
   // The same string picked again: the note is damped over 30 ms by the
   // pick, which then releases it again. 50 ms after the first release
   // the second falls inside the refractory time; 150 ms after, it
   // fires again.
   float sps;
   auto x = load("3a-D", sps);
   auto repicked = [&](double apart)
   {
      auto const d = std::size_t(apart * sps);
      auto const damp = std::size_t(0.03 * sps);
      auto const first = std::size_t(0.9843 * sps);
      std::vector<float> y(x.size());
      for (std::size_t t = 0; t != x.size(); ++t)
      {
         auto const end = first + d;
         float g = t < end - damp ? 1.0f
            : t < end ? float(end - t) / damp : 0.0f;
         y[t] = g * x[t] + (t >= d ? x[t - d] : 0.0f);
      }
      return y;
   };
   CHECK(fires(repicked(0.05), sps).size() == 1);
   CHECK(fires(repicked(0.15), sps).size() == 2);
}

TEST_CASE("transient_detector: silence never fires")
{
   std::vector<float> x(88200, 0.0f);
   CHECK(fires(x, 44100).empty());
}
