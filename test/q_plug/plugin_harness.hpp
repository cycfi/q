/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// A plugin driven through its own CLAP entry, the way a host drives it: a
// note in, samples out. No audio device and no window, so it runs in
// ctest. Each stage of the synth links its own implementation and includes
// this to drive it.
#if !defined(QPLUG_TEST_PLUGIN_HARNESS_HPP_SEPTEMBER_11_2026)
#define QPLUG_TEST_PLUGIN_HARNESS_HPP_SEPTEMBER_11_2026

#include <infra/catch.hpp>
#include <clap/clap.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" bool        qplug_entry_init(char const* plugin_path);
extern "C" void        qplug_entry_deinit();
extern "C" void const* qplug_entry_get_factory(char const* factory_id);

namespace
{
   constexpr std::uint32_t sps = 44100;
   constexpr std::uint32_t block = 512;

   // A host with nothing in it. The plugin asks for extensions and for a
   // main thread callback; neither has to do anything here.
   clap_host_t make_host()
   {
      clap_host_t h{};
      h.clap_version = CLAP_VERSION;
      h.name = "qplug test host";
      h.vendor = "QPlug";
      h.url = "";
      h.version = "1.0";
      h.get_extension = [](clap_host_t const*, char const*) -> void const*
      {
         return nullptr;
      };
      h.request_restart = [](clap_host_t const*) {};
      h.request_process = [](clap_host_t const*) {};
      h.request_callback = [](clap_host_t const*) {};
      return h;
   }

   // One note on, at the frame it is stamped with.
   struct note_events
   {
      note_events(std::uint8_t key, std::uint32_t time, bool on)
      {
         _ev.header.size = sizeof(_ev);
         _ev.header.time = time;
         _ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
         _ev.header.type = on? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
         _ev.note_id = -1;
         _ev.port_index = 0;
         _ev.channel = 0;
         _ev.key = key;
         _ev.velocity = 1.0;

         _in.ctx = this;
         _in.size = [](clap_input_events_t const*) -> std::uint32_t
         {
            return 1;
         };
         _in.get = [](clap_input_events_t const* l, std::uint32_t)
         {
            return &static_cast<note_events const*>(l->ctx)->_ev.header;
         };
      }

      clap_event_note_t     _ev{};
      clap_input_events_t   _in{};
   };

   // The same note in the MIDI dialect: three bytes, as a keyboard sends.
   struct midi_events
   {
      midi_events(std::uint8_t status, std::uint8_t d1, std::uint8_t d2)
      {
         _ev.header.size = sizeof(_ev);
         _ev.header.time = 0;
         _ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
         _ev.header.type = CLAP_EVENT_MIDI;
         _ev.port_index = 0;
         _ev.data[0] = status;
         _ev.data[1] = d1;
         _ev.data[2] = d2;

         _in.ctx = this;
         _in.size = [](clap_input_events_t const*) -> std::uint32_t
         {
            return 1;
         };
         _in.get = [](clap_input_events_t const* l, std::uint32_t)
         {
            return &static_cast<midi_events const*>(l->ctx)->_ev.header;
         };
      }

      clap_event_midi_t     _ev{};
      clap_input_events_t   _in{};
   };

   // And in the MIDI 2.0 dialect, which is the one this plugin tells the
   // host it prefers.
   struct midi2_events
   {
      midi2_events(std::uint32_t w0, std::uint32_t w1)
      {
         _ev.header.size = sizeof(_ev);
         _ev.header.time = 0;
         _ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
         _ev.header.type = CLAP_EVENT_MIDI2;
         _ev.port_index = 0;
         _ev.data[0] = w0;
         _ev.data[1] = w1;

         _in.ctx = this;
         _in.size = [](clap_input_events_t const*) -> std::uint32_t
         {
            return 1;
         };
         _in.get = [](clap_input_events_t const* l, std::uint32_t)
         {
            return &static_cast<midi2_events const*>(l->ctx)->_ev.header;
         };
      }

      clap_event_midi2_t    _ev{};
      clap_input_events_t   _in{};
   };

   clap_input_events_t const* no_events()
   {
      static clap_input_events_t in{};
      in.ctx = nullptr;
      in.size = [](clap_input_events_t const*) -> std::uint32_t { return 0; };
      in.get = [](clap_input_events_t const*, std::uint32_t)
         -> clap_event_header_t const* { return nullptr; };
      return &in;
   }

   clap_output_events_t const* no_output_events()
   {
      static clap_output_events_t out{};
      out.ctx = nullptr;
      out.try_push = [](clap_output_events_t const*
       , clap_event_header_t const*) { return true; };
      return &out;
   }

   // The plugin, up and activated, with buffers to write into.
   struct instance
   {
      instance()
      {
         REQUIRE(qplug_entry_init(""));
         auto factory = static_cast<clap_plugin_factory_t const*>(
            qplug_entry_get_factory(CLAP_PLUGIN_FACTORY_ID));
         REQUIRE(factory != nullptr);
         REQUIRE(factory->get_plugin_count(factory) == 1);

         auto const* desc = factory->get_plugin_descriptor(factory, 0);
         REQUIRE(desc != nullptr);
         _host = make_host();
         _plugin = factory->create_plugin(factory, &_host, desc->id);
         REQUIRE(_plugin != nullptr);
         REQUIRE(_plugin->init(_plugin));
         REQUIRE(_plugin->activate(_plugin, sps, 1, block));
         REQUIRE(_plugin->start_processing(_plugin));

         _left.resize(block);
         _right.resize(block);
         _channels[0] = _left.data();
         _channels[1] = _right.data();
         _out.data32 = _channels;
         _out.data64 = nullptr;
         _out.channel_count = 2;
         _out.latency = 0;
         _out.constant_mask = 0;
      }

      ~instance()
      {
         if (_plugin)
         {
            _plugin->stop_processing(_plugin);
            _plugin->deactivate(_plugin);
            _plugin->destroy(_plugin);
         }
         qplug_entry_deinit();
      }

      // One block. Returns the peak of what came out.
      float run(clap_input_events_t const* in)
      {
         std::fill(_left.begin(), _left.end(), 0.0f);
         std::fill(_right.begin(), _right.end(), 0.0f);

         clap_process_t proc{};
         proc.steady_time = _steady;
         proc.frames_count = block;
         proc.transport = nullptr;
         proc.audio_inputs = nullptr;
         proc.audio_inputs_count = 0;
         proc.audio_outputs = &_out;
         proc.audio_outputs_count = 1;
         proc.in_events = in? in : no_events();
         proc.out_events = no_output_events();

         auto const status = _plugin->process(_plugin, &proc);
         REQUIRE(status != CLAP_PROCESS_ERROR);
         _steady += block;

         float peak = 0.0f;
         for (auto s : _left)
            peak = std::max(peak, std::abs(s));
         return peak;
      }

      clap_host_t             _host{};
      clap_plugin_t const*    _plugin = nullptr;
      std::vector<float>      _left, _right;
      float*                  _channels[2] = {};
      clap_audio_buffer_t     _out{};
      std::int64_t            _steady = 0;
   };

   // One parameter value, as a host sends it.
   struct param_events
   {
      param_events(clap_id id, double value)
      {
         _ev.header.size = sizeof(_ev);
         _ev.header.time = 0;
         _ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
         _ev.header.type = CLAP_EVENT_PARAM_VALUE;
         _ev.param_id = id;
         _ev.value = value;
         _ev.note_id = -1;
         _ev.port_index = -1;
         _ev.channel = -1;
         _ev.key = -1;

         _in.ctx = this;
         _in.size = [](clap_input_events_t const*) -> std::uint32_t
         {
            return 1;
         };
         _in.get = [](clap_input_events_t const* l, std::uint32_t)
         {
            return &static_cast<param_events const*>(l->ctx)->_ev.header;
         };
      }

      clap_event_param_value_t   _ev{};
      clap_input_events_t        _in{};
   };

   // The largest step between one sample and the next: a sound that moves
   // smoothly has no big steps in it.
   inline float largest_step(std::vector<float> const& v)
   {
      float worst = 0.0f;
      for (std::size_t i = 1; i != v.size(); ++i)
         worst = std::max(worst, std::abs(v[i] - v[i-1]));
      return worst;
   }
}

#endif
