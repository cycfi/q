/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The FM synth's factory presets: the voices of the DX7's first ROM
// cartridge, as states in the plugin's JSON. Nothing in the build reads
// the file, so these check it against the parameters the plugin declares,
// and that every preset is a sound.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"
#include "dexter_controller.hpp"
#include <artist/resources.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <map>
#include <vector>
#include <string>

namespace
{
   using json = nlohmann::json;

   json read_factory_presets()
   {
      std::ifstream in(Q_PLUG_DEXTER_PRESETS);
      REQUIRE(in.good());
      auto j = json::parse(in, nullptr, false);
      REQUIRE(!j.is_discarded());
      return j;
   }

   // The parameters a preset holds: those not marked dont_save, by id
   std::map<clap_id, q_plug::parameter> saved()
   {
      std::map<clap_id, q_plug::parameter> params;
      for (auto const& p : dexter_controller{}.parameters())
      {
         if (p.is_saved())
            params.emplace(p.id(), p);
      }
      return params;
   }
}

TEST_CASE("The factory presets are a cartridge's 32 voices")
{
   auto const j = read_factory_presets();
   REQUIRE(j.is_object());
   CHECK(j.size() == 32);
}

TEST_CASE("Every factory preset is this plugin's state, whole and in range")
{
   auto const params = saved();
   auto const j = read_factory_presets();
   for (auto const& [name, state] : j.items())
   {
      INFO("preset " << name);
      CHECK(state.value("plugin", "") == std::string("com.qplug.dexter"));
      CHECK(state.value("version", std::uint32_t(0)) == 1);

      std::map<clap_id, bool> seen;
      for (auto const& e : state["params"])
      {
         auto const id = e["id"].get<clap_id>();
         INFO("parameter " << id);
         auto const i = params.find(id);
         REQUIRE(i != params.end());
         CHECK(e["name"].get<std::string>() == i->second.name());
         auto const value = e["value"].get<double>();
         CHECK(value >= i->second.min());
         CHECK(value <= i->second.max());
         seen[id] = true;
      }
      CHECK(seen.size() == params.size());
   }
}

TEST_CASE("Every factory preset is a sound")
{
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

      // Not silent: above -80 dB. Some are quiet by design, TAKE OFF a
      // sound effect at about -60 dB, and the mix leaves room for chords.
      note_events on{60, 0, true};
      synth.run(&on._in);
      float peak = 0.0f;
      for (int i = 0; i != 40; ++i)
         peak = std::max(peak, synth.run(nullptr));
      CHECK(peak > 1e-4f);
   }
}

///////////////////////////////////////////////////////////////////////////////
// Importing a .syx file. The cartridge is made here, its voices all
// zeros, so no voice of Yamaha's is in the test.
///////////////////////////////////////////////////////////////////////////////
namespace
{
   struct no_sink : cycfi::q_plug::edit_sink
   {
      void begin_edit(int) override {}
      void edit_parameter(int, double) override {}
      void end_edit(int) override {}
   };

   struct live_controller : dexter_controller
   {
      live_controller() { init(_sink); }
      no_sink _sink;
   };

   std::vector<std::uint8_t> blank_cartridge()
   {
      std::vector<std::uint8_t> d{0xF0, 0x43, 0x00, 0x09, 0x20, 0x00};
      d.resize(6 + 4096, 0);
      d.push_back(0);
      d.push_back(0xF7);
      return d;
   }
}

TEST_CASE("A cartridge's voices become user presets, each named once")
{
   live_controller ctl;
   auto const names = ctl.import_voices(blank_cartridge(), "test cart");
   REQUIRE(names.size() == 32);

   // Blank names are Untitled; the one already taken gets the file's
   // name, and the ones after a number.
   std::map<std::string, int> seen;
   for (auto const& n : names)
   {
      CHECK(ctl.has_preset(n));
      CHECK(!ctl.is_factory_preset(n));
      CHECK(++seen[n] == 1);
   }

   // A voice loads as the patch it was
   REQUIRE(ctl.load_preset(names.back()));
   auto const p = ctl.patch();
   CHECK(p.algorithm.algorithm == 1);
   CHECK(p.op[0].output_level == 0);

   for (auto const& n : names)
      ctl.delete_preset(n);
   for (auto const& n : names)
      CHECK(!ctl.has_preset(n));
}

TEST_CASE("The presets are listed in their files' order")
{
   // The factory file is put on the search path where it lies in the
   // source tree, as a test binary has no resources of its own.
   cycfi::artist::add_search_path(
      cycfi::fs::path{Q_PLUG_DEXTER_PRESETS}.parent_path());

   // The factory presets, in the cartridge's order, not by name
   std::ifstream in(Q_PLUG_DEXTER_PRESETS);
   std::vector<std::string> cartridge;
   auto const file = nlohmann::ordered_json::parse(in);
   for (auto const& [name, state] : file.items())
      cartridge.push_back(name);

   live_controller ctl;
   auto const names = ctl.preset_names();
   REQUIRE(names.size() >= cartridge.size());
   CHECK(std::vector<std::string>(
      names.begin(), names.begin() + cartridge.size()) == cartridge);

   // The user's, in the order they were added
   auto const added = ctl.import_voices(blank_cartridge(), "order test");
   REQUIRE(!added.empty());
   auto const after = ctl.preset_names();
   REQUIRE(after.size() >= added.size());
   CHECK(std::vector<std::string>(after.end() - added.size(), after.end())
      == added);

   for (auto const& n : added)
      ctl.delete_preset(n);
}

TEST_CASE("A file that is not a DX7 dump imports nothing")
{
   live_controller ctl;
   std::vector<std::uint8_t> const junk(4104, 0x55);
   CHECK(ctl.import_voices(junk, "junk").empty());
}

TEST_CASE("The operator switches are in the session, not in a preset")
{
   using f = dexter_controller;
   live_controller ctl;
   auto const op1 = f::op_index(0, f::on);
   ctl.set_parameter(op1, 0.0);

   // The session keeps it
   auto has = [](nlohmann::json const& j, clap_id id)
   {
      for (auto const& e : j["params"])
      {
         if (e["id"].get<clap_id>() == id)
            return true;
      }
      return false;
   };
   CHECK(has(ctl.state(), 121));

   // A preset does not, and loading one turns the operator on
   std::string const name = "dont_save test";
   REQUIRE(ctl.save_preset(name));
   ctl.set_parameter(op1, 0.0);
   REQUIRE(ctl.load_preset(name));
   CHECK(ctl.enabled(0));
   ctl.delete_preset(name);
}

TEST_CASE("Only a voice parameter's change counts toward a rebuild")
{
   // The controller counts the changes the processor has to rebuild
   // for. Dexter's own parameters are live, read as they are used, and
   // a value set again to what it was is no change.
   using f = dexter_controller;
   live_controller ctl;
   auto const start = ctl.changes();

   ctl.set_parameter(f::volume_id, -12.0);
   ctl.set_parameter(f::op_index(0, f::on), 0.0);
   CHECK(ctl.changes() == start);

   ctl.set_parameter(f::op_index(0, f::level), 50.0);
   auto const once = ctl.changes();
   CHECK(once != start);

   ctl.set_parameter(f::op_index(0, f::level), 50.0);
   CHECK(ctl.changes() == once);
}
