/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Stage 5 of the synth. What it adds is presets: named states, the
// factory ones shipped beside the plugin as JSON. The file is a resource
// and nothing in the build reads it, so a typo in a name or a value out
// of a parameter's range would only show as a preset that quietly does
// nothing. These check it against the parameters the plugin advertises.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"
#include "va_synth_controller.hpp"
#include <nlohmann/json.hpp>

#include <fstream>
#include <map>
#include <string>

namespace
{
   using json = nlohmann::json;

   json read_factory_presets()
   {
      std::ifstream in(QPLUG_VA_SYNTH_5_PRESETS);
      REQUIRE(in.good());
      auto j = json::parse(in, nullptr, false);
      REQUIRE(!j.is_discarded());
      return j;
   }

   // What the plugin says its parameters are, by id, straight from the
   // params extension the host reads.
   struct param_info
   {
      std::string name;
      double      min;
      double      max;
   };

   std::map<clap_id, param_info> advertised(instance& synth)
   {
      auto const* ext = static_cast<clap_plugin_params_t const*>(
         synth._plugin->get_extension(synth._plugin, CLAP_EXT_PARAMS));
      REQUIRE(ext != nullptr);

      std::map<clap_id, param_info> params;
      auto const count = ext->count(synth._plugin);
      for (std::uint32_t i = 0; i != count; ++i)
      {
         clap_param_info_t info{};
         REQUIRE(ext->get_info(synth._plugin, i, &info));
         params[info.id] = {info.name, info.min_value, info.max_value};
      }
      return params;
   }
}

TEST_CASE("The factory presets file is a JSON object of named states")
{
   auto const j = read_factory_presets();
   CHECK(j.is_object());
   CHECK(j.size() >= 2);
   for (auto const& [name, state] : j.items())
   {
      CHECK_FALSE(name.empty());
      CHECK(state.is_object());
   }
}

TEST_CASE("Every factory preset is this plugin's own state")
{
   instance synth;
   auto const* desc = synth._plugin->desc;
   REQUIRE(desc != nullptr);

   auto const j = read_factory_presets();
   for (auto const& [name, state] : j.items())
   {
      INFO("preset " << name);
      CHECK(state.value("plugin", "") == std::string(desc->id));
      CHECK(state.value("version", std::uint32_t(0)) > 0);
      CHECK(state.contains("params"));
      CHECK(state["params"].is_array());
   }
}

TEST_CASE("Every factory preset names only real parameters, in range")
{
   instance synth;
   auto const params = advertised(synth);
   REQUIRE(!params.empty());

   auto const j = read_factory_presets();
   for (auto const& [name, state] : j.items())
   {
      for (auto const& e : state["params"])
      {
         auto const id = e["id"].get<clap_id>();
         INFO("preset " << name << ", parameter " << id);

         auto const i = params.find(id);
         REQUIRE(i != params.end());
         CHECK(e["name"].get<std::string>() == i->second.name);

         auto const value = e["value"].get<double>();
         CHECK(value >= i->second.min);
         CHECK(value <= i->second.max);
      }
   }
}

TEST_CASE("Every factory preset carries every parameter")
{
   // A state the plugin loads takes its default for anything the state
   // does not carry, so a factory preset that leaves one out is a preset
   // whose sound depends on what was loaded before it.
   instance synth;
   auto const params = advertised(synth);

   auto const j = read_factory_presets();
   for (auto const& [name, state] : j.items())
   {
      INFO("preset " << name);
      std::map<clap_id, bool> seen;
      for (auto const& e : state["params"])
         seen[e["id"].get<clap_id>()] = true;
      CHECK(seen.size() == params.size());
      for (auto const& [id, info] : params)
      {
         INFO("parameter " << info.name);
         CHECK(seen.count(id) == 1);
      }
   }
}

TEST_CASE("Every factory preset is a sound")
{
   // Sent in as the host would send them, every preset's values make a
   // note that can be heard: no preset is silent by accident.
   auto const j = read_factory_presets();
   for (auto const& [name, state] : j.items())
   {
      INFO("preset " << name);
      instance synth;
      for (auto const& e : state["params"])
      {
         param_events set{
            e["id"].get<clap_id>(), e["value"].get<double>()};
         synth.run(&set._in);
      }

      note_events on{69, 0, true};
      synth.run(&on._in);

      float peak = 0.0f;
      for (int i = 0; i != 20; ++i)
         peak = std::max(peak, synth.run(nullptr));
      CHECK(peak > 0.01f);
   }
}

///////////////////////////////////////////////////////////////////////////////
// The preset the panel shows is part of the state, so a session comes back
// showing the preset it was saved with, edited or not. A controller with
// parameters to move needs standing up the way a plugin does it.
///////////////////////////////////////////////////////////////////////////////
namespace
{
   struct no_sink : cycfi::qplug::edit_sink
   {
      void begin_edit(int) override {}
      void edit_parameter(int, double) override {}
      void end_edit(int) override {}
   };

   struct live_controller : va_synth_controller
   {
      live_controller() { init(_sink); }
      no_sink _sink;
   };
}

TEST_CASE("A fresh controller shows no preset")
{
   va_synth_controller ctl;
   CHECK(ctl.preset_name().empty());
   CHECK(!ctl.preset_edited());
}

TEST_CASE("The preset name and its edited flag ride in the state")
{
   va_synth_controller a;
   a.preset_name("Strings");
   a.preset_edited(true);

   va_synth_controller b;
   REQUIRE(b.state(a.state()));
   CHECK(b.preset_name() == "Strings");
   CHECK(b.preset_edited());
}

TEST_CASE("A state saved before presets loads with none")
{
   va_synth_controller a;
   auto j = a.state();
   j.erase("preset");

   va_synth_controller b;
   b.preset_name("Lead");
   b.preset_edited(true);
   REQUIRE(b.state(j));
   CHECK(b.preset_name().empty());
   CHECK(!b.preset_edited());
}

TEST_CASE("Any parameter moving marks the preset edited")
{
   live_controller ctl;
   ctl.preset_name("Brass");
   CHECK(!ctl.preset_edited());
   ctl.set_parameter(0, 0.5);
   CHECK(ctl.preset_edited());
}

TEST_CASE("Loading a preset names it, unedited")
{
   live_controller a;
   a.set_parameter(0, 0.5);
   REQUIRE(a.save_preset("qplug test name"));

   live_controller b;
   b.preset_name("Something else");
   b.set_parameter(0, 0.7);
   REQUIRE(b.load_preset("qplug test name"));
   CHECK(b.preset_name() == "qplug test name");
   CHECK(!b.preset_edited());

   REQUIRE(b.delete_preset("qplug test name"));
}

TEST_CASE("Naming a preset does not mark it edited, and clearing forgets it")
{
   va_synth_controller ctl;
   ctl.preset_edited(true);
   ctl.preset_name("Organ");
   CHECK(!ctl.preset_edited());

   ctl.preset_edited(true);
   ctl.preset_name("");
   CHECK(ctl.preset_name().empty());
   CHECK(!ctl.preset_edited());
}

///////////////////////////////////////////////////////////////////////////////
// The editor's zoom is the plugin's, not the preset's: it rides in the
// state a session keeps and stays out of the state a preset holds.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("The view scale rides in the state")
{
   va_synth_controller a;
   a.view_scale(1.3f);

   va_synth_controller b;
   REQUIRE(b.state(a.state()));
   CHECK(b.view_scale() == 1.3f);
}

TEST_CASE("A state without a view scale leaves the zoom alone")
{
   va_synth_controller a;
   auto j = a.state();
   j.erase("view");

   va_synth_controller b;
   b.view_scale(1.5f);
   REQUIRE(b.state(j));
   CHECK(b.view_scale() == 1.5f);
}

TEST_CASE("A preset carries no view scale, so loading one does not zoom")
{
   // Saved through the user preset file, which lands in the user's own
   // directory; the name is one no one would keep.
   va_synth_controller a;
   a.view_scale(1.7f);
   REQUIRE(a.save_preset("qplug test zoom"));

   va_synth_controller b;
   b.view_scale(0.8f);
   REQUIRE(b.load_preset("qplug test zoom"));
   CHECK(b.view_scale() == 0.8f);

   REQUIRE(b.delete_preset("qplug test zoom"));
}
