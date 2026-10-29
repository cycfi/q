/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/duration.hpp>    // before audio_stream.hpp, see below
#include <q_io/audio_device.hpp>
#include <q_io/audio_stream.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

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

TEST_CASE("Stream times are std::chrono durations, whatever came first")
{
   // q::duration is in scope here before audio_stream.hpp. The stream's
   // times are still the std::chrono type its implementation returns.
   using chrono_seconds = std::chrono::duration<double>;
   using stream = q::audio_stream const&;
   STATIC_REQUIRE(std::is_same_v<
      decltype(std::declval<stream>().time()), chrono_seconds>);
   STATIC_REQUIRE(std::is_same_v<
      decltype(std::declval<stream>().input_latency()), chrono_seconds>);
   STATIC_REQUIRE(std::is_same_v<
      decltype(std::declval<stream>().output_latency()), chrono_seconds>);
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

TEST_CASE("A device keeps its handle and id from one listing to the next")
{
   // Devices may come and go between the listings (a display's audio
   // device does), so only what both listings hold is compared.
   auto const first = q::audio_device::list();
   std::vector<std::pair<int, std::string>> seen;
   for (auto const& device : first)
      seen.emplace_back(device.id(), device.name());

   auto const second = q::audio_device::list();

   // The earlier handles still read their own devices.
   for (std::size_t i = 0; i != first.size(); ++i)
   {
      CHECK(first[i].id() == seen[i].first);
      CHECK(first[i].name() == seen[i].second);
   }

   // A device in both listings has the same id in both.
   for (auto const& device : second)
   {
      for (auto const& [id, name] : seen)
      {
         if (id == device.id())
            CHECK(name == device.name());
      }
   }
}

TEST_CASE("An id never handed out gives a listed device, or the default")
{
   // With no device at all, a CI runner's case, this is the default.
   auto const unknown = 1'000'000;
   auto const device = q::audio_device::get(unknown);
   CHECK(device.id() != unknown);
   if (q::audio_device::list().empty())
      CHECK(device.id() == q::audio_device::default_id);
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

   // Some devices take a while to start: WASAPI sets the device up on the
   // stream's own thread before the first callback, and a USB interface
   // can take seconds. The count starts from the first callback.
   stream.start();
   auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
   while (stream._calls == 0 && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   auto const first = stream._calls.load();
   std::this_thread::sleep_for(std::chrono::milliseconds(300));
   auto const calls = stream._calls.load() - first;
   auto const elapsed = stream.time();
   stream.stop();

   // 300 ms at any block size is many callbacks, and the stream clock has
   // moved with them. A stream that opened but did not start says why in
   // its error.
   INFO("Audio system error: " << stream.error());
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
