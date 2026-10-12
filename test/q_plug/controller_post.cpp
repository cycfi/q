/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// controller::post, through a plugin of the test's own, driven by its CLAP
// entry as a host drives it: the processor posts, the host is asked for a
// main-thread callback, and the task runs there and edits a parameter,
// which reaches the host's output events.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_plug/plugin.hpp>
#include <q/support/literals.hpp>
#include <clap/clap.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;
using namespace q::literals;

extern "C" bool        q_plug_entry_init(char const* plugin_path);
extern "C" void        q_plug_entry_deinit();
extern "C" void const* q_plug_entry_get_factory(char const* factory_id);

namespace
{
   constexpr std::uint32_t sps = 48000;
   constexpr std::uint32_t block = 480;      // 10 ms

   // What the processor posts, and at which block. Set by each test.
   int post_at = -1;
   int post_delayed_at = -1;

   class test_controller : public q_plug::controller
   {
   public:

      enum { flag_id, count_id };

      parameter_list parameters() const override
      {
         static parameter params[] =
         {
            parameter{ 1, "Flag", false },
            parameter{ 2, "Count", 0.0 }.range(0.0, 100.0)
         };
         return { params };
      }
   };

   // An edit made on the main thread, which reaches the host.
   void edit(test_controller* c, int index, double value)
   {
      c->begin_edit(index);
      c->edit_parameter(index, value);
      c->end_edit(index);
   }

   class test_processor : public q_plug::processor
   {
   public:

      test_processor(test_controller& ctl)
       : _ctl(ctl)
      {}

      channel_config channels() const override { return {0, 2}; }

      void process(in_channels const&, out_channels const&) override
      {
         auto c = &_ctl;
         if (_block == post_at)
            _ctl.post([c] { edit(c, test_controller::flag_id, 1.0); });
         if (_block == post_delayed_at)
            _ctl.post(100_ms,
               [c] { edit(c, test_controller::count_id, 42.0); });
         ++_block;
      }

   private:

      test_controller&  _ctl;
      int               _block = 0;
   };
}

namespace cycfi::q_plug
{
   controller_ptr make_controller()
   {
      return std::make_unique<test_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<test_processor>(
         static_cast<test_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller&)
   {
      return nullptr;
   }

   plugin_info const& info()
   {
      static char const* const features[] = { "instrument", nullptr };
      static plugin_info const i =
      {
         "com.qplug.test.controller_post", "Controller Post", "QPlug",
         "", "", "", "0.1.0", "", features, {100, 100}, 1
      };
      return i;
   }
}

namespace
{
   // A host that counts the callbacks asked for, and keeps the parameter
   // values the plugin sends out.
   struct test_host
   {
      test_host()
      {
         _host.clap_version = CLAP_VERSION;
         _host.host_data = this;
         _host.name = "controller_post test";
         _host.vendor = "QPlug";
         _host.url = "";
         _host.version = "1.0";
         _host.get_extension =
            [](clap_host_t const*, char const*) -> void const*
            {
               return nullptr;
            };
         _host.request_restart = [](clap_host_t const*) {};
         _host.request_process = [](clap_host_t const*) {};
         _host.request_callback =
            [](clap_host_t const* h)
            {
               ++static_cast<test_host*>(h->host_data)->callbacks;
            };

         _out.ctx = this;
         _out.try_push =
            [](clap_output_events_t const* o, clap_event_header_t const* e)
            {
               if (e->type == CLAP_EVENT_PARAM_VALUE)
               {
                  auto v = reinterpret_cast<
                     clap_event_param_value_t const*>(e);
                  static_cast<test_host*>(o->ctx)->sent.push_back(
                     {v->param_id, v->value});
               }
               return true;
            };

         _in.size = [](clap_input_events_t const*) -> std::uint32_t
         {
            return 0;
         };
         _in.get = [](clap_input_events_t const*, std::uint32_t)
            -> clap_event_header_t const* { return nullptr; };
      }

      struct sent_value
      {
         clap_id        id;
         double         value;
      };

      clap_host_t                _host{};
      clap_output_events_t       _out{};
      clap_input_events_t        _in{};
      int                        callbacks = 0;
      std::vector<sent_value>    sent;
   };

   // The plugin, up and activated, and a block at a time.
   struct instance
   {
      instance()
      {
         REQUIRE(q_plug_entry_init(""));
         auto factory = static_cast<clap_plugin_factory_t const*>(
            q_plug_entry_get_factory(CLAP_PLUGIN_FACTORY_ID));
         auto const* desc = factory->get_plugin_descriptor(factory, 0);
         _plugin = factory->create_plugin(factory, &host._host, desc->id);
         REQUIRE(_plugin);
         REQUIRE(_plugin->init(_plugin));
         REQUIRE(_plugin->activate(_plugin, sps, 1, block));
         REQUIRE(_plugin->start_processing(_plugin));
         _params = static_cast<clap_plugin_params_t const*>(
            _plugin->get_extension(_plugin, CLAP_EXT_PARAMS));
         REQUIRE(_params);

         _left.resize(block);
         _right.resize(block);
         _channels[0] = _left.data();
         _channels[1] = _right.data();
         _out.data32 = _channels;
         _out.channel_count = 2;
      }

      ~instance()
      {
         _plugin->stop_processing(_plugin);
         _plugin->deactivate(_plugin);
         _plugin->destroy(_plugin);
         q_plug_entry_deinit();
      }

      void process()
      {
         clap_process_t p{};
         p.steady_time = -1;
         p.frames_count = block;
         p.audio_outputs = &_out;
         p.audio_outputs_count = 1;
         p.in_events = &host._in;
         p.out_events = &host._out;
         _plugin->process(_plugin, &p);
      }

      void main_thread()
      {
         _plugin->on_main_thread(_plugin);
      }

      double value(clap_id id)
      {
         double v = -1;
         REQUIRE(_params->get_value(_plugin, id, &v));
         return v;
      }

      test_host                  host;
      clap_plugin_t const*       _plugin = nullptr;
      clap_plugin_params_t const* _params = nullptr;
      std::vector<float>         _left, _right;
      float*                     _channels[2];
      clap_audio_buffer_t        _out{};
   };
}

TEST_CASE("controller::post runs a task on the main thread")
{
   post_at = 2;
   post_delayed_at = -1;
   instance in;

   in.process();
   in.process();
   CHECK(in.host.callbacks == 0);

   // The processor posts: the host is asked for a callback, and nothing
   // changes until the main thread runs it.
   in.process();
   CHECK(in.host.callbacks == 1);
   CHECK(in.value(1) == 0.0);

   in.main_thread();
   CHECK(in.value(1) == 1.0);

   // The edit reaches the host at the next block.
   in.process();
   REQUIRE(!in.host.sent.empty());
   CHECK(in.host.sent.back().id == 1);
   CHECK(in.host.sent.back().value == 1.0);
}

TEST_CASE("controller::post with a delay waits for audio time")
{
   post_at = -1;
   post_delayed_at = 0;
   instance in;

   // Posted at block 0: a callback is asked for at once, so the main
   // thread polls the task. It is not due, so nothing changes.
   in.process();
   CHECK(in.host.callbacks == 1);
   in.main_thread();
   CHECK(in.value(2) == 0.0);

   // Posted at audio time 0, the task is due at 100 ms, 4800 frames: the
   // end of block 9. No callback is asked for before then, however often
   // the main thread runs.
   for (int i = 1; i != 9; ++i)
      in.process();
   CHECK(in.host.callbacks == 1);
   in.main_thread();
   CHECK(in.value(2) == 0.0);

   // Block 9 brings the clock to the due time: one callback is asked for,
   // and the main thread runs the task.
   in.process();
   CHECK(in.host.callbacks == 2);
   in.process();
   CHECK(in.host.callbacks == 2);
   in.main_thread();
   CHECK(in.value(2) == 42.0);
}
