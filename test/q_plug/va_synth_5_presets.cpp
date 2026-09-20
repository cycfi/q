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
#include <artist/resources.hpp>
#include <qplug/presenter.hpp>
#include <elements.hpp>

#include <fstream>
#include <map>
#include <string>
#include <vector>

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

TEST_CASE("A factory name is refused, and the factory preset stands")
{
   // A controller finds the factory presets among the plugin's resources,
   // which a test binary has none of, so the file is put on the search
   // path where it lies in the source tree.
   cycfi::artist::add_search_path(
      cycfi::fs::path{QPLUG_VA_SYNTH_5_PRESETS}.parent_path());

   // Saved over, the user's preset would be the one that loaded while
   // the list still called the name factory, and the editor would not
   // delete it. The name stays the plugin's.
   live_controller ctl;
   auto const names = ctl.preset_names();
   REQUIRE(!names.empty());
   auto const& factory = names.front();
   REQUIRE(ctl.is_factory_preset(factory));

   REQUIRE(ctl.load_preset(factory));
   auto const original = ctl.get_parameter(0);

   ctl.set_parameter(0, original + 0.1);
   CHECK(!ctl.save_preset(factory));

   // Nothing was written, so the name still loads what the plugin ships.
   live_controller other;
   REQUIRE(other.load_preset(factory));
   CHECK(other.get_parameter(0) == original);

   // The list did not grow a second entry for it either.
   CHECK(other.preset_names() == names);
}

///////////////////////////////////////////////////////////////////////////////
// The preset section of the main menu. The items are built without a
// view, so a presenter stands up on a controller alone and the list can
// be read back.
///////////////////////////////////////////////////////////////////////////////
namespace
{
   struct test_presenter : cycfi::qplug::presenter
   {
      using presenter::presenter;
      using presenter::menu;
      using presenter::preset_menu_items;
   };

   // The items in order, as what carries their enabled state and click.
   std::vector<cycfi::elements::basic_menu_item_element*>
   preset_items(test_presenter& p, test_presenter::menu& items)
   {
      p.preset_menu_items(items);
      std::vector<cycfi::elements::basic_menu_item_element*> out;
      for (auto& e : items)
         out.push_back(
            dynamic_cast<cycfi::elements::basic_menu_item_element*>(e.get()));
      return out;
   }
}

TEST_CASE("The preset section is Save, Save As and Delete")
{
   live_controller ctl;
   test_presenter p{ctl};
   test_presenter::menu items;

   auto const got = preset_items(p, items);
   REQUIRE(got.size() == 3);
   for (auto* item : got)
      REQUIRE(item != nullptr);
}

TEST_CASE("Save Preset is off with no preset, a factory one, or nothing moved")
{
   cycfi::artist::add_search_path(
      cycfi::fs::path{QPLUG_VA_SYNTH_5_PRESETS}.parent_path());

   live_controller ctl;
   test_presenter p{ctl};
   test_presenter::menu items;
   auto const got = preset_items(p, items);
   auto* save = got[0];

   // Nothing named: nothing to write to.
   CHECK(ctl.preset_name().empty());
   CHECK(!save->is_enabled());

   // A factory preset is not the user's to write over, moved or not.
   auto const factory = ctl.preset_names().front();
   REQUIRE(ctl.is_factory_preset(factory));
   ctl.preset_name(factory);
   ctl.set_parameter(0, 0.3);
   REQUIRE(ctl.preset_edited());
   CHECK(!save->is_enabled());

   // The user's own, but naming it leaves it unedited: nothing to write.
   ctl.preset_name("qplug test save");
   REQUIRE(!ctl.preset_edited());
   CHECK(!save->is_enabled());

   // Moved since, and it is on.
   ctl.set_parameter(0, 0.31);
   CHECK(save->is_enabled());
}

TEST_CASE("Save Preset writes the preset shown and clears the edited mark")
{
   live_controller ctl;
   test_presenter p{ctl};
   test_presenter::menu items;
   auto const got = preset_items(p, items);
   auto* save = got[0];

   ctl.preset_name("qplug test save");
   ctl.set_parameter(0, 0.42);
   REQUIRE(ctl.preset_edited());
   REQUIRE(save->is_enabled());

   save->on_click();

   CHECK(ctl.preset_name() == "qplug test save");
   CHECK(!ctl.preset_edited());

   // It is on disk, and it is what was showing when Save was clicked.
   live_controller other;
   REQUIRE(other.load_preset("qplug test save"));
   CHECK(other.get_parameter(0) == 0.42);

   REQUIRE(other.delete_preset("qplug test save"));
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
