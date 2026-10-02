/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "dexter_controller.hpp"
#include <q/synth/fm/dx_sysex.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

using parameter_list = dexter_controller::parameter_list;
using parameter = q_plug::parameter;
using namespace cycfi::q::literals;

namespace
{
   char const* const curves[] = {"-Lin", "-Exp", "+Exp", "+Lin", nullptr};
   char const* const waves[] =
   {
      "Triangle", "Saw Down", "Saw Up", "Square", "Sine", "S/Hold", nullptr
   };
   char const* const modes[] = {"Ratio", "Fixed", nullptr};
   char const* const keys[] = {"Poly", "Mono", nullptr};
   char const* const glides[] = {"Retain", "Follow", nullptr};

   // A DX7 break point is 0..99 from A-1, which is MIDI key 21.
   constexpr int break_point_key = 21;

   // Operator n's parameters take ids 100 n and up, so an id says which
   // operator it is. A DX7 INIT VOICE sounds OP1 alone.
   void add_operator(
      std::vector<parameter>& list, std::vector<std::string>& names, int op)
   {
      auto id = [op](int field)
      {
         return parameter::id_type(100 * (op + 1) + field);
      };
      auto name = [&](char const* what) -> char const*
      {
         return names.emplace_back(
            "OP" + std::to_string(op + 1) + " " + what).c_str();
      };
      auto tag = "OP" + std::to_string(op + 1);
      auto module = names.emplace_back(tag).c_str();

      using f = dexter_controller;
      list.push_back(parameter{id(f::level), name("Level"), op == 0? 99 : 0}
         .range(0, 99).module(module));
      list.push_back(parameter{id(f::fixed), name("Mode"), 0, modes}
         .module(module));
      list.push_back(parameter{id(f::coarse), name("Coarse"), 1}
         .range(0, 31).module(module));
      list.push_back(parameter{id(f::fine), name("Fine"), 0}
         .range(0, 99).module(module));
      list.push_back(parameter{id(f::detune), name("Detune"), 0}
         .range(-7, 7).module(module));

      char const* rates[] = {"R1", "R2", "R3", "R4"};
      char const* levels[] = {"L1", "L2", "L3", "L4"};
      for (int i = 0; i != 4; ++i)
         list.push_back(parameter{id(f::rate + i), name(rates[i]), 99}
            .range(0, 99).module(module));
      for (int i = 0; i != 4; ++i)
         list.push_back(parameter{id(f::env_level + i), name(levels[i])
          , i == 3? 0 : 99}.range(0, 99).module(module));

      list.push_back(
         parameter{id(f::break_point), name("Break Point")
          , q::midi_1_0::note(break_point_key + 39)}
         .range(break_point_key, break_point_key + 99).module(module));
      list.push_back(parameter{id(f::left_depth), name("Left Depth"), 0}
         .range(0, 99).module(module));
      list.push_back(parameter{id(f::right_depth), name("Right Depth"), 0}
         .range(0, 99).module(module));
      list.push_back(parameter{id(f::left_curve), name("Left Curve")
       , 0, curves}.module(module));
      list.push_back(parameter{id(f::right_curve), name("Right Curve")
       , 0, curves}.module(module));
      list.push_back(parameter{id(f::rate_scaling), name("Rate Scaling"), 0}
         .range(0, 7).module(module));
      list.push_back(parameter{id(f::amp_mod_sens), name("Amp Mod Sens"), 0}
         .range(0, 3).module(module));
      list.push_back(
         parameter{id(f::velocity_sens), name("Velocity Sens"), 0}
         .range(0, 7).module(module));
      list.push_back(parameter{id(f::on), name("On"), true}
         .dont_save().live().module(module));
   }

   std::vector<parameter> make_parameters()
   {
      // The names are made here, so they live as long as the list does,
      // for the life of the plugin. Reserved, so none of them moves.
      static std::vector<std::string> names;
      names.reserve(dexter_controller::num_operators
         * (dexter_controller::op_size + 1));

      std::vector<parameter> list;
      list.reserve(dexter_controller::num_params);

      list.push_back(parameter{1, "Algorithm", 1}.range(1, 32));
      list.push_back(parameter{2, "Feedback", 0}.range(0, 7));
      list.push_back(parameter{3, "Pitch R1", 99}.range(0, 99));
      list.push_back(parameter{4, "Pitch R2", 99}.range(0, 99));
      list.push_back(parameter{5, "Pitch R3", 99}.range(0, 99));
      list.push_back(parameter{6, "Pitch R4", 99}.range(0, 99));
      list.push_back(parameter{7, "Pitch L1", 50}.range(0, 99));
      list.push_back(parameter{8, "Pitch L2", 50}.range(0, 99));
      list.push_back(parameter{9, "Pitch L3", 50}.range(0, 99));
      list.push_back(parameter{10, "Pitch L4", 50}.range(0, 99));
      list.push_back(parameter{11, "LFO Speed", 35}.range(0, 99));
      list.push_back(parameter{12, "LFO Delay", 0}.range(0, 99));
      list.push_back(parameter{13, "LFO Pitch Depth", 0}.range(0, 99));
      list.push_back(parameter{14, "LFO Amp Depth", 0}.range(0, 99));
      list.push_back(parameter{15, "LFO Key Sync", true});
      list.push_back(parameter{16, "LFO Wave", 0, waves});
      list.push_back(parameter{17, "Pitch Mod Sens", 3}.range(0, 7));
      list.push_back(parameter{18, "Osc Key Sync", true});
      list.push_back(
         parameter{19, "Transpose", 0}.range(-24, 24).unit("st"));

      // The plugin's own, beyond the voice, read as they are used rather
      // than compiled into the patch
      list.push_back(parameter{20, "Volume", 0_dB}.range(-60.0, 0.0).live());
      list.push_back(parameter{21, "Keys", 0, keys}.live());
      list.push_back(parameter{22, "Portamento", 0, glides}.live());
      list.push_back(parameter{23, "Glide", 0}.range(0, 99).live());

      for (int op = 0; op != dexter_controller::num_operators; ++op)
         add_operator(list, names, op);
      return list;
   }
}

parameter_list dexter_controller::parameters() const
{
   static std::vector<parameter> const params = make_parameters();
   return {params.data(), params.data() + params.size()};
}

///////////////////////////////////////////////////////////////////////////////
// The voice, read back from the parameters. OP1 is op[0], as in dx_patch.
///////////////////////////////////////////////////////////////////////////////
q::dx_patch dexter_controller::patch() const
{
   auto value = [this](int index)
   {
      return std::uint8_t(std::lround(get_parameter(index)));
   };

   q::dx_patch p;
   for (int i = 0; i != num_operators; ++i)
   {
      auto at = [i](int field) { return op_index(i, field); };
      auto& o = p.op[i];
      o.output_level = value(at(level));
      o.fixed = value(at(fixed)) != 0;
      o.coarse = value(at(coarse));
      o.fine = value(at(fine));
      o.detune = std::uint8_t(std::lround(get_parameter(at(detune))) + 7);
      for (int s = 0; s != 4; ++s)
      {
         o.env.rate[s] = value(at(rate + s));
         o.env.level[s] = value(at(env_level + s));
      }
      o.break_point = value(at(break_point)) - break_point_key;
      o.left_depth = value(at(left_depth));
      o.right_depth = value(at(right_depth));
      o.left_curve = value(at(left_curve));
      o.right_curve = value(at(right_curve));
      o.rate_scaling = value(at(rate_scaling));
      o.amp_mod_sens = value(at(amp_mod_sens));
      o.velocity_sens = value(at(velocity_sens));
   }

   for (int s = 0; s != 4; ++s)
   {
      p.pitch_env.rate[s] = value(pitch_rate_id + s);
      p.pitch_env.level[s] = value(pitch_level_id + s);
   }
   p.algorithm.algorithm = value(algorithm_id);
   p.algorithm.feedback = value(feedback_id);
   p.lfo.speed = value(lfo_speed_id);
   p.lfo.delay = value(lfo_delay_id);
   p.lfo.key_sync = value(lfo_sync_id) != 0;
   p.lfo.wave = value(lfo_wave_id);
   p.pitch_mod_depth = value(lfo_pitch_depth_id);
   p.amp_mod_depth = value(lfo_amp_depth_id);
   p.pitch_mod_sens = value(pitch_mod_sens_id);
   p.osc_key_sync = value(osc_sync_id) != 0;
   p.transpose =
      std::uint8_t(std::lround(get_parameter(transpose_id)) + 24);
   return p;
}

q::decibel dexter_controller::volume() const
{
   return get_parameter<q::decibel>(volume_id);
}

bool dexter_controller::mono() const
{
   return get_parameter<int>(mono_id) == 1;
}

bool dexter_controller::porta_follow() const
{
   return get_parameter<int>(porta_mode_id) == 1;
}

int dexter_controller::glide() const
{
   return get_parameter<int>(glide_id);
}

bool dexter_controller::enabled(int op) const
{
   return get_parameter<bool>(op_index(op, on));
}

///////////////////////////////////////////////////////////////////////////////
// A voice as a preset, in the layout controller::state writes: the plugin,
// the state version, and each saved parameter by id and name.
///////////////////////////////////////////////////////////////////////////////
dexter_controller::json
dexter_controller::state_of(q::dx_patch const& p) const
{
   std::array<double, num_params> v;
   auto params = parameters();
   for (int i = 0; i != num_params; ++i)
      v[i] = params[i].init();

   for (int i = 0; i != num_operators; ++i)
   {
      auto at = [i](int field) { return op_index(i, field); };
      auto const& o = p.op[i];
      v[at(level)] = o.output_level;
      v[at(fixed)] = o.fixed? 1 : 0;
      v[at(coarse)] = o.coarse;
      v[at(fine)] = o.fine;
      v[at(detune)] = int(o.detune) - 7;
      for (int s = 0; s != 4; ++s)
      {
         v[at(rate + s)] = o.env.rate[s];
         v[at(env_level + s)] = o.env.level[s];
      }
      v[at(break_point)] = o.break_point + break_point_key;
      v[at(left_depth)] = o.left_depth;
      v[at(right_depth)] = o.right_depth;
      v[at(left_curve)] = o.left_curve;
      v[at(right_curve)] = o.right_curve;
      v[at(rate_scaling)] = o.rate_scaling;
      v[at(amp_mod_sens)] = o.amp_mod_sens;
      v[at(velocity_sens)] = o.velocity_sens;
   }

   for (int s = 0; s != 4; ++s)
   {
      v[pitch_rate_id + s] = p.pitch_env.rate[s];
      v[pitch_level_id + s] = p.pitch_env.level[s];
   }
   v[algorithm_id] = p.algorithm.algorithm;
   v[feedback_id] = p.algorithm.feedback;
   v[lfo_speed_id] = p.lfo.speed;
   v[lfo_delay_id] = p.lfo.delay;
   v[lfo_sync_id] = p.lfo.key_sync? 1 : 0;
   v[lfo_wave_id] = p.lfo.wave;
   v[lfo_pitch_depth_id] = p.pitch_mod_depth;
   v[lfo_amp_depth_id] = p.amp_mod_depth;
   v[pitch_mod_sens_id] = p.pitch_mod_sens;
   v[osc_sync_id] = p.osc_key_sync? 1 : 0;
   v[transpose_id] = int(p.transpose) - 24;

   // What a preset leaves out, as save_preset does
   auto j = state();
   j.erase("view");
   j.erase("preset");
   auto& list = j["params"];
   list = json::array();
   for (int i = 0; i != num_params; ++i)
   {
      if (!params[i].is_saved())
         continue;
      list.push_back({
         {"id", params[i].id()}
       , {"name", params[i].name()}
       , {"value", v[i]}
      });
   }
   return j;
}

///////////////////////////////////////////////////////////////////////////////
// Importing a DX7 .syx file
///////////////////////////////////////////////////////////////////////////////
namespace
{
   // A DX7 name pads with spaces: "BRASS   1" reads as "BRASS 1".
   std::string tidy(char const* name)
   {
      std::string out;
      for (auto const* c = name; *c; ++c)
      {
         if (*c != ' ' || (!out.empty() && out.back() != ' '))
            out += *c;
      }
      while (!out.empty() && out.back() == ' ')
         out.pop_back();
      return out.empty()? "Untitled" : out;
   }
}

dexter_controller::name_list
dexter_controller::import_voices(bytes syx, std::string const& file)
{
   std::vector<q::dx_patch> voices;
   if (auto bank = q::dx_cartridge(syx))
      voices.assign(bank->begin(), bank->end());
   else if (auto voice = q::dx_voice(syx))
      voices.push_back(*voice);
   else
      return {};

   name_list names;
   named_states states;
   auto taken = [&](std::string const& n)
   {
      return has_preset(n)
         || std::find(names.begin(), names.end(), n) != names.end();
   };

   for (auto const& v : voices)
   {
      auto name = tidy(v.name);
      if (taken(name))
         name += " (" + file + ")";
      auto const base = name;
      for (int n = 2; taken(name); ++n)
         name = base + " " + std::to_string(n);

      names.push_back(name);
      states.emplace_back(name, state_of(v));
   }

   add_presets(states);
   return names;
}
