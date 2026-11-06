/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fft/spectral_learner.hpp>

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fx/biquad.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <numbers>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000.0f;

   // A synthetic instrument: notes with 1/k harmonics, decaying over
   // their second, from A1 up three octaves, through a fixed resonance.
   std::vector<float> play(q::frequency resonance, int notes, float rate = sps)
   {
      q::lowpass body{resonance, rate, 3.0};
      std::vector<float> out;
      for (int n = 0; n != notes; ++n)
      {
         auto const f0 = 55.0 * std::exp2(36.0 * n / notes / 12.0);
         auto const len = std::size_t(rate);
         for (std::size_t i = 0; i != len; ++i)
         {
            auto const t = double(i) / rate;
            double s = 0;
            for (int k = 1; k * f0 < 0.45 * rate; ++k)
               s += std::sin(2 * std::numbers::pi * k * f0 * t) / k;
            out.push_back(body(float(0.3 * s * std::exp(-3.0 * t))));
         }
      }
      return out;
   }

   // The frequency of the spectral envelope's highest point in lo .. hi.
   double peak_hz(q::spectral_learner const& l, double lo, double hi)
   {
      std::vector<float> db(l.bins());
      l.spectral_envelope(db);
      auto const bin_hz = l.sps() / (2.0 * double(l.bins() - 1));
      std::size_t best = std::size_t(lo / bin_hz);
      for (auto b = best; b * bin_hz <= hi; ++b)
         if (db[b] > db[best])
            best = b;
      return best * bin_hz;
   }
}

TEST_CASE("spectral_learner: learns a fixed resonance", "[spectral]")
{
   q::spectral_learner learner{sps};
   learner(play(3000_Hz, 30));

   CHECK(learner.frames() > 0);
   CHECK(learner.coverage() > 0.8f);
   CHECK(learner.bins() == 2049);
   CHECK(std::abs(peak_hz(learner, 1000, 10000) / 3000 - 1) < 0.15);

   // The average is at unit power a frame.
   double total = 0;
   for (auto p : learner.spectrum())
      total += p;
   CHECK(std::abs(total - 1) < 1e-3);
}

TEST_CASE("spectral_learner: learn, freeze and drift", "[spectral]")
{
   q::spectral_learner learner{sps};
   auto const a = play(3000_Hz, 20);
   learner(a);
   auto const frames = learner.frames();
   auto const same = learner.drift();

   learner.freeze(true);
   learner(a);
   CHECK(learner.frames() == frames);
   CHECK(learner.frozen());
   learner.freeze(false);

   // Another instrument: the recent playing drifts from what was learned.
   learner(play(800_Hz, 8));
   auto const other = learner.drift();
   INFO("drift: same " << same << " dB, other " << other << " dB");
   CHECK(other > 2.0f * same);

   learner.learn();
   CHECK(learner.frames() == 0);
   CHECK(learner.coverage() == 0.0f);
}

TEST_CASE("spectral_learner: save and restore", "[spectral]")
{
   q::spectral_learner a{sps};
   a(play(3000_Hz, 20));
   q::spectral_learner::state s;
   a.save(s);

   q::spectral_learner b{sps};
   REQUIRE(b.restore(s));
   CHECK(b.frames() == a.frames());
   CHECK(b.coverage() == a.coverage());
   CHECK(std::equal(a.spectrum().begin(), a.spectrum().end()
    , b.spectrum().begin()));

   // Learning goes on from it as from the original. The original also
   // holds a partial frame of input, which is not learning, so the two
   // may be a frame apart.
   auto const more = play(3000_Hz, 5);
   a(more);
   b(more);
   CHECK(std::abs(b.frames() - a.frames()) <= 1);
   float diff = 0, top = 0;
   for (std::size_t i = 0; i != a.bins(); ++i)
   {
      diff = std::max(diff, std::abs(a.spectrum()[i] - b.spectrum()[i]));
      top = std::max(top, a.spectrum()[i]);
   }
   CHECK(diff < 0.01f * top);

   // Nothing learned, or a state of no shape, is refused.
   q::spectral_learner c{sps};
   CHECK(!c.restore(q::spectral_learner::state{}));
   CHECK(c.frames() == 0);
}

TEST_CASE("spectral_learner: restore at another sample rate", "[spectral]")
{
   q::spectral_learner a{sps};
   a(play(3000_Hz, 30));
   q::spectral_learner::state s;
   a.save(s);

   q::spectral_learner b{44100.0f};
   REQUIRE(b.restore(s));
   CHECK(b.frames() == a.frames());
   CHECK(b.coverage() == a.coverage());
   CHECK(std::abs(peak_hz(b, 1000, 10000) / 3000 - 1) < 0.15);

   double total = 0;
   for (auto p : b.spectrum())
      total += p;
   CHECK(std::abs(total - 1) < 1e-3);

   // And it keeps learning at its own rate.
   b(play(3000_Hz, 5, 44100.0f));
   CHECK(b.frames() > a.frames());
   CHECK(std::abs(peak_hz(b, 1000, 10000) / 3000 - 1) < 0.15);
}
