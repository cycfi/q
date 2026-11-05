/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Q Bender driven through its own CLAP entry, as a host drives it: a sine
// in, the host's pitch bend, samples out.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"
#include <numbers>

namespace
{
   // The plugin as an effect: one channel in, one out.
   struct effect
   {
      effect()
      {
         REQUIRE(q_plug_entry_init(""));
         auto factory = static_cast<clap_plugin_factory_t const*>(
            q_plug_entry_get_factory(CLAP_PLUGIN_FACTORY_ID));
         REQUIRE(factory != nullptr);
         auto const* desc = factory->get_plugin_descriptor(factory, 0);
         REQUIRE(desc != nullptr);
         _host = make_host();
         _plugin = factory->create_plugin(factory, &_host, desc->id);
         REQUIRE(_plugin != nullptr);
         REQUIRE(_plugin->init(_plugin));
         REQUIRE(_plugin->activate(_plugin, sps, 1, block));
         REQUIRE(_plugin->start_processing(_plugin));

         _in_buf.resize(block);
         _out_buf.resize(block);
         _in_ch[0] = _in_buf.data();
         _out_ch[0] = _out_buf.data();
         _in.data32 = _in_ch;
         _in.channel_count = 1;
         _out.data32 = _out_ch;
         _out.channel_count = 1;
      }

      ~effect()
      {
         _plugin->stop_processing(_plugin);
         _plugin->deactivate(_plugin);
         _plugin->destroy(_plugin);
         q_plug_entry_deinit();
      }

      // One block of the sine at f, through the plugin. Appends the input
      // and the output to x and y.
      void run(float f, clap_input_events_t const* ev
       , std::vector<float>& x, std::vector<float>& y)
      {
         for (auto& s : _in_buf)
         {
            s = 0.5f * std::sin(_phase);
            _phase += 2 * std::numbers::pi_v<float> * f / sps;
            if (_phase > 2 * std::numbers::pi_v<float>)
               _phase -= 2 * std::numbers::pi_v<float>;
         }

         clap_process_t proc{};
         proc.steady_time = _steady;
         proc.frames_count = block;
         proc.audio_inputs = &_in;
         proc.audio_inputs_count = 1;
         proc.audio_outputs = &_out;
         proc.audio_outputs_count = 1;
         proc.in_events = ev ? ev : no_events();
         proc.out_events = no_output_events();
         REQUIRE(_plugin->process(_plugin, &proc) != CLAP_PROCESS_ERROR);
         _steady += block;

         x.insert(x.end(), _in_buf.begin(), _in_buf.end());
         y.insert(y.end(), _out_buf.begin(), _out_buf.end());
      }

      clap_host_t             _host{};
      clap_plugin_t const*    _plugin = nullptr;
      std::vector<float>      _in_buf, _out_buf;
      float*                  _in_ch[1] = {};
      float*                  _out_ch[1] = {};
      clap_audio_buffer_t     _in{}, _out{};
      std::int64_t            _steady = 0;
      float                   _phase = 0;
   };

   // The frequency of the last `n` samples of y, from its upward zero
   // crossings.
   float frequency(std::vector<float> const& y, std::size_t n)
   {
      std::size_t first = 0, last = 0, count = 0;
      for (std::size_t i = y.size() - n + 1; i != y.size(); ++i)
      {
         if (y[i - 1] < 0 && y[i] >= 0)
         {
            if (count++ == 0)
               first = i;
            last = i;
         }
      }
      REQUIRE(count > 2);
      return float(count - 1) * sps / float(last - first);
   }

   // A MIDI 1.0 pitch bend, 14 bits, channel 0.
   midi_events bend(std::uint16_t v)
   {
      return midi_events{0xE0, std::uint8_t(v & 0x7F), std::uint8_t(v >> 7)};
   }
}

TEST_CASE("At rest, the input passes through bit for bit")
{
   effect fx;
   std::vector<float> x, y;
   for (int i = 0; i != 40; ++i)
      fx.run(220.0f, nullptr, x, y);
   CHECK(x == y);
}

TEST_CASE("The bar at the top plays an octave up")
{
   effect fx;
   std::vector<float> x, y;
   auto top = bend(16383);
   fx.run(220.0f, &top._in, x, y);
   for (int i = 0; i != 80; ++i)
      fx.run(220.0f, nullptr, x, y);
   CHECK(frequency(y, sps / 2) == Approx(440.0f).epsilon(0.01));
}

TEST_CASE("The bar at the bottom plays an octave down")
{
   effect fx;
   std::vector<float> x, y;
   auto bottom = bend(0);
   fx.run(220.0f, &bottom._in, x, y);
   for (int i = 0; i != 80; ++i)
      fx.run(220.0f, nullptr, x, y);
   CHECK(frequency(y, sps / 2) == Approx(110.0f).epsilon(0.01));
}

TEST_CASE("Back at the center, the pitch returns")
{
   effect fx;
   std::vector<float> x, y;
   auto top = bend(16383);
   fx.run(220.0f, &top._in, x, y);
   for (int i = 0; i != 40; ++i)
      fx.run(220.0f, nullptr, x, y);
   auto center = bend(8192);
   fx.run(220.0f, &center._in, x, y);
   for (int i = 0; i != 80; ++i)
      fx.run(220.0f, nullptr, x, y);
   CHECK(frequency(y, sps / 2) == Approx(220.0f).epsilon(0.01));
}

TEST_CASE("The wheel at the top plays an octave up")
{
   effect fx;
   std::vector<float> x, y;
   param_events wheel{7, 1.0};               // Bend, its top
   fx.run(220.0f, &wheel._in, x, y);
   for (int i = 0; i != 80; ++i)
      fx.run(220.0f, nullptr, x, y);
   CHECK(frequency(y, sps / 2) == Approx(440.0f).epsilon(0.01));
}

TEST_CASE("With the wheel at rest, the host's bend plays")
{
   effect fx;
   std::vector<float> x, y;
   param_events wheel{7, 0.0};
   fx.run(220.0f, &wheel._in, x, y);
   auto bottom = bend(0);
   fx.run(220.0f, &bottom._in, x, y);
   for (int i = 0; i != 80; ++i)
      fx.run(220.0f, nullptr, x, y);
   CHECK(frequency(y, sps / 2) == Approx(110.0f).epsilon(0.01));
}
