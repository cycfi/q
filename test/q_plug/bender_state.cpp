/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Q Bender's learning in the session: what the formant learner has learned
// is saved with the state and comes back with it, at any sample rate.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"
#include "bender_controller.hpp"

#include <chrono>
#include <cmath>
#include <numbers>
#include <thread>
#include <vector>

namespace
{
   struct no_sink : q_plug::edit_sink
   {
      void begin_edit(int) override {}
      void edit_parameter(int, double) override {}
      void end_edit(int) override {}
   };

   struct live_controller : bender_controller
   {
      live_controller() { init(_sink); }
      no_sink _sink;
   };

   // Notes with 1/k harmonics, decaying over their second, from A2 up two
   // octaves.
   std::vector<float> play(float rate, int notes)
   {
      std::vector<float> out;
      for (int n = 0; n != notes; ++n)
      {
         auto const f0 = 110.0 * std::exp2(24.0 * n / notes / 12.0);
         auto const len = std::size_t(rate);
         for (std::size_t i = 0; i != len; ++i)
         {
            auto const t = double(i) / rate;
            double s = 0;
            for (int k = 1; k * f0 < 0.45 * rate; ++k)
               s += std::sin(2 * std::numbers::pi * k * f0 * t) / k;
            out.push_back(float(0.3 * s * std::exp(-3.0 * t)));
         }
      }
      return out;
   }

   // Feed the worker as the audio thread would, a block at a time, at
   // about ten times real time so its ring never fills.
   void feed(formant_worker& w, std::vector<float> const& x)
   {
      std::size_t const block = 512;
      for (std::size_t i = 0; i + block <= x.size(); i += block)
      {
         w.feed({x.data() + i, block});
         std::this_thread::sleep_for(std::chrono::microseconds(1000));
      }
   }

   template <typename F>
   bool wait_for(F done)
   {
      auto const end = std::chrono::steady_clock::now() + std::chrono::seconds{10};
      while (!done())
      {
         if (std::chrono::steady_clock::now() > end)
            return false;
         std::this_thread::sleep_for(std::chrono::milliseconds{5});
      }
      return true;
   }
}

TEST_CASE("The learning comes back with the session, and goes on")
{
   live_controller a;
   a.worker.start(48000);
   feed(a.worker, play(48000, 8));
   REQUIRE(wait_for([&]{ return a.worker.saved().has_value(); }));
   a.worker.stop();

   auto const learned = *a.worker.saved();
   CHECK(learned.frames > 0);
   auto const state = a.state();
   REQUIRE(state.contains("formants"));

   // A new instance, at another sample rate, takes the state.
   live_controller b;
   REQUIRE(b.state(state));
   auto const pending = b.worker.saved();
   REQUIRE(pending);
   CHECK(pending->frames == learned.frames);
   CHECK(pending->spectrum == learned.spectrum);

   // Started, it fits a table from it at once, with no new input.
   b.worker.start(44100);
   REQUIRE(wait_for([&]{ return b.worker.table() != nullptr; }));
   CHECK(b.worker.coverage() == a.worker.coverage());
   CHECK(b.worker.coverage() > 0.5f);

   // And a restart keeps it.
   b.worker.stop();
   b.worker.start(48000);
   REQUIRE(wait_for([&]{ return b.worker.table() != nullptr; }));
   CHECK(b.worker.saved()->frames == learned.frames);
   b.worker.stop();
}

TEST_CASE("A state with no learning, or one that does not parse, learns afresh")
{
   live_controller a;
   auto state = a.state();
   CHECK(!state.contains("formants"));

   state["formants"] = {{"sps", "fast"}, {"frames", 3}};
   live_controller b;
   b.state(state);
   CHECK(!b.worker.saved());
}
