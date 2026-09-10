/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The synth driven through its own CLAP entry, the way a host drives it: a
// note in, samples out. No audio device and no window, so it runs in ctest.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <clap/clap.h>

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
}

TEST_CASE("An instrument reports no audio input and one note port")
{
   instance synth;

   auto ports = static_cast<clap_plugin_audio_ports_t const*>(
      synth._plugin->get_extension(synth._plugin, CLAP_EXT_AUDIO_PORTS));
   REQUIRE(ports != nullptr);
   CHECK(ports->count(synth._plugin, true) == 0);      // no audio in
   CHECK(ports->count(synth._plugin, false) == 1);     // stereo out

   auto notes = static_cast<clap_plugin_note_ports_t const*>(
      synth._plugin->get_extension(synth._plugin, CLAP_EXT_NOTE_PORTS));
   REQUIRE(notes != nullptr);
   REQUIRE(notes->count(synth._plugin, true) == 1);

   clap_note_port_info_t info{};
   REQUIRE(notes->get(synth._plugin, 0, true, &info));
   CHECK((info.supported_dialects & CLAP_NOTE_DIALECT_MIDI) != 0);
   CHECK((info.supported_dialects & CLAP_NOTE_DIALECT_MIDI2) != 0);
   CHECK((info.supported_dialects & CLAP_NOTE_DIALECT_CLAP) != 0);
}

TEST_CASE("Silence in, silence out")
{
   instance synth;
   CHECK(synth.run(nullptr) == 0.0f);
}

TEST_CASE("A note makes a sound")
{
   instance synth;

   note_events on{60, 0, true};
   auto const peak = synth.run(&on._in);
   CHECK(peak > 0.01f);
}

TEST_CASE("A note keeps sounding after the block it started in")
{
   instance synth;

   note_events on{60, 0, true};
   synth.run(&on._in);
   CHECK(synth.run(nullptr) > 0.01f);
}

TEST_CASE("A note released falls silent")
{
   instance synth;

   note_events on{60, 0, true};
   synth.run(&on._in);

   note_events off{60, 0, false};
   synth.run(&off._in);

   // The release runs for the release time, so give it a second of blocks.
   float peak = 1.0f;
   for (int i = 0; i != int(sps / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

TEST_CASE("A note in the MIDI dialect makes a sound")
{
   instance synth;
   midi_events on{0x90, 60, 100};      // note on, channel 0, key 60
   CHECK(synth.run(&on._in) > 0.01f);
}

TEST_CASE("A note in the MIDI 2.0 dialect makes a sound")
{
   // The dialect this plugin says it prefers, so it is the one a host that
   // reads the note port will send.
   instance synth;
   midi2_events on{0x40903C00u, 0xFFFF0000u};
   CHECK(synth.run(&on._in) > 0.01f);
}

////////////////////////////////////////////////////////////////////////////
// The damper pedal, controller 64: while it is down, a note that is
// released keeps sounding, and lifting the pedal releases what it held.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("The damper pedal holds a note through its note off")
{
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);

   note_events off{60, 0, false};
   synth.run(&off._in);

   // A second of blocks: without the pedal the release would be long
   // gone, since the note off would have started it.
   float peak = 0.0f;
   for (int i = 0; i != int(sps / block); ++i)
      peak = synth.run(nullptr);
   CHECK(peak > 0.01f);
}

TEST_CASE("Lifting the damper pedal releases what it held")
{
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);
   synth.run(nullptr);

   midi_events pedal_up{0xB0, 64, 0};
   synth.run(&pedal_up._in);

   float peak = 1.0f;
   for (int i = 0; i != int(sps * 4 / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

TEST_CASE("A pedal held note is not released by lifting the key twice")
{
   // The pedal is what decides, not how many note offs arrived.
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);

   note_events off{60, 0, false};
   synth.run(&off._in);
   synth.run(&off._in);

   CHECK(synth.run(nullptr) > 0.01f);
}

TEST_CASE("A note struck again while the pedal is down sounds again")
{
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);

   // Struck again: it must sound, and lifting the pedal must still end it.
   synth.run(&on._in);
   CHECK(synth.run(nullptr) > 0.01f);

   midi_events pedal_up{0xB0, 64, 0};
   synth.run(&pedal_up._in);
   note_events off2{60, 0, false};
   synth.run(&off2._in);

   float peak = 1.0f;
   for (int i = 0; i != int(sps * 4 / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

TEST_CASE("Half pedal counts as down, per the convention for controller 64")
{
   // 64 and above is down, below is up.
   instance synth;

   midi_events half{0xB0, 64, 64};
   synth.run(&half._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);

   float peak = 0.0f;
   for (int i = 0; i != int(sps / block); ++i)
      peak = synth.run(nullptr);
   CHECK(peak > 0.01f);
}

TEST_CASE("A voice stolen from the pedal is not released by lifting it")
{
   // Sixteen voices, all held by the pedal, then a seventeenth note: it
   // has to steal one of them. The stolen voice is now playing a key that
   // is still down, so lifting the pedal must not end it.
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   for (int i = 0; i != 16; ++i)
   {
      note_events on{std::uint8_t(60 + i), 0, true};
      synth.run(&on._in);
   }
   for (int i = 0; i != 16; ++i)
   {
      note_events off{std::uint8_t(60 + i), 0, false};
      synth.run(&off._in);
   }

   note_events stealer{76, 0, true};      // the seventeenth, key still down
   synth.run(&stealer._in);

   midi_events pedal_up{0xB0, 64, 0};
   synth.run(&pedal_up._in);

   // Long enough for every released voice to have died away.
   float peak = 0.0f;
   for (int i = 0; i != int(sps * 2 / block); ++i)
      peak = synth.run(nullptr);

   CHECK(peak > 0.01f);
}
