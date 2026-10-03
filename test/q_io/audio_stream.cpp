/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_io/audio_device.hpp>
#include <q_io/audio_stream.hpp>

#include <atomic>
#include <chrono>
#include <thread>

namespace q = cycfi::q;

////////////////////////////////////////////////////////////////////////////
// The audio device and stream against whatever hardware the machine has.
// A machine with no output device, a CI runner for one, skips the stream
// test with a warning rather than failing it: there is nothing to open.
////////////////////////////////////////////////////////////////////////////
namespace
{
   // Plays silence on the default output and counts the callbacks. The
   // default, not the first listed: a display's audio device can come and
   // go, and a stream on one dies with it.
   struct counter : q::audio_stream
   {
      counter()
       : audio_stream(0, 2)
      {}

      void process(out_channels const& out) override
      {
         for (std::size_t ch = 0; ch != out.size(); ++ch)
            for (auto& s : out[ch])
               s = 0.0f;
         ++_calls;
      }

      std::atomic<std::size_t> _calls{0};
   };
}

TEST_CASE("Every listed audio device can be fetched by id and describes itself")
{
   auto const devices = q::audio_device::list();
   for (auto const& device : devices)
   {
      auto const again = q::audio_device::get(device.id());
      CHECK(again.id() == device.id());
      CHECK(again.name() == device.name());
      CHECK(!device.name().empty());
      CHECK(device.default_sample_rate() > 0);
      CHECK(device.input_channels() + device.output_channels() > 0);
   }
}

TEST_CASE("An output stream opens, runs its callback and reports its rates")
{
   counter stream;
   if (!stream.is_valid())
   {
      // A CI runner has no output device: nothing to open.
      WARN(std::string("The default output would not open: ") + stream.error());
      return;
   }

   CHECK(stream.input_channels() == 0);
   CHECK(stream.output_channels() == 2);
   CHECK(stream.sampling_rate() > 0);
   CHECK(stream._calls == 0);

   stream.start();
   std::this_thread::sleep_for(std::chrono::milliseconds(300));
   auto const calls = stream._calls.load();
   auto const elapsed = stream.time();
   stream.stop();

   // 300 ms at any block size is many callbacks, and the stream clock has
   // moved with them.
   CHECK(calls > 2);
   CHECK(elapsed.count() > 0.1);                 // seconds
   CHECK(stream.output_latency().count() >= 0.0);
   CHECK(stream.cpu_load() >= 0.0);

   // Stopped, the callback stops. A callback may still land between the
   // reads above and stop(), so count from after stop() returns.
   auto const stopped = stream._calls.load();
   std::this_thread::sleep_for(std::chrono::milliseconds(100));
   CHECK(stream._calls == stopped);
}
