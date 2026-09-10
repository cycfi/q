/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/controller.hpp>
#include <qplug/base_plugin.hpp>
#include <qplug/log.hpp>
#include <artist/resources.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>

namespace cycfi::qplug
{
   namespace fs = std::filesystem;
   using json = controller::json;

   ////////////////////////////////////////////////////////////////////////////
   // The state
   //
   // A state is one JSON object:
   //
   //    {
   //       "plugin": "com.qplug.delay",
   //       "version": 1,
   //       "params": [
   //          {"id": 1, "name": "Delay", "value": 0.35},
   //          {"id": 2, "name": "Feedback", "value": 50.0}
   //       ]
   //    }
   //
   // plus whatever the plugin's save_extra adds. The plugin id keeps a
   // state from another plugin out; the version is the plugin's own state
   // version from info(), so it can tell an old layout of its values from
   // the current one. A value is found by id, then by name, so a plugin
   // may add, remove, reorder or rename its parameters and an old state
   // still loads: an entry it no longer has is skipped, and a parameter
   // the state does not carry takes its default. A state written by a
   // later version of the plugin is refused rather than misread.
   ////////////////////////////////////////////////////////////////////////////
   json controller::state() const
   {
      auto params = parameters();
      json j;
      j["plugin"] = info().id;
      j["version"] = info().state_version;

      auto& entries = j["params"] = json::array();
      for (int i = 0; i != _size; ++i)
      {
         auto const& p = params[i];
         if (!p._save_in_preset)
            continue;
         entries.push_back({
            {"id", p._id}, {"name", p._name}, {"value", get_parameter(i)}
         });
      }

      j["view"] = {{"scale", _view_scale}};
      save_extra(j);
      return j;
   }

   bool controller::state(json const& j)
   {
      if (!j.is_object())
         return false;

      auto plugin = j.value("plugin", "");
      if (plugin != info().id)
      {
         QPLUG_LOG(app, "state: from {}, not this plugin", plugin);
         return false;
      }

      auto version = j.value("version", std::uint32_t(0));
      if (version > info().state_version)
      {
         QPLUG_LOG(app, "state: version {} is newer than {}"
          , version, info().state_version);
         return false;
      }

      auto entries = j.find("params");
      if (entries == j.end() || !entries->is_array())
         return false;

      // Whatever the state does not carry starts from its default.
      auto params = parameters();
      for (int i = 0; i != _size; ++i)
         if (params[i]._save_in_preset)
            set_parameter(i, params[i]._init);

      for (auto const& e : *entries)
      {
         if (!e.is_object() || !e.contains("value"))
            continue;

         int index = -1;
         if (e.contains("id"))
            index = index_of(e["id"].get<parameter::id_type>());
         if (index < 0 && e.contains("name"))
            index = index_of(e["name"].get<std::string>());
         if (index < 0)
            continue;

         auto const& p = params[index];
         auto value = e["value"].get<double>();
         set_parameter(index, std::clamp(value, p._min, p._max));
      }

      // A state without it, an older one or a preset, leaves the zoom
      // where it is.
      if (auto v = j.find("view"); v != j.end() && v->is_object())
         _view_scale = v->value("scale", _view_scale);

      load_extra(j, version);
      update_models();
      return true;
   }

   // The host's stream carries the JSON as text.
   bool controller::save_state(ostream& out) const
   {
      auto text = state().dump();
      auto const* data = text.data();
      auto left = std::int64_t(text.size());
      while (left > 0)
      {
         auto n = out.write(data, left);
         if (n <= 0)
            return false;
         data += n;
         left -= n;
      }
      return true;
   }

   bool controller::load_state(istream& in)
   {
      std::string text;
      char buf[4096];
      while (true)
      {
         auto n = in.read(buf, sizeof(buf));
         if (n < 0)
            return false;
         if (n == 0)
            break;
         text.append(buf, std::size_t(n));
      }

      auto j = json::parse(text, nullptr, false);
      if (j.is_discarded())
      {
         QPLUG_LOG(app, "state: not JSON");
         return false;
      }
      return state(j);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Presets
   //
   // A preset file is one JSON object, preset name to state. The factory
   // file is factory_presets.json among the plugin's resources, read
   // once; the user file is presets.json in a directory of the user's own
   // for this plugin, read once and written whenever a preset is saved or
   // deleted. Both are read on first use, so a plugin without presets
   // pays nothing.
   ////////////////////////////////////////////////////////////////////////////
   namespace
   {
      // Where the user's presets go, per platform, per vendor and plugin.
      fs::path user_presets_dir()
      {
         auto env = [](char const* name) -> fs::path
         {
            if (auto v = std::getenv(name))
               return v;
            return {};
         };

#if defined(_WIN32)
         auto base = env("APPDATA");
#elif defined(__APPLE__)
         auto base = env("HOME") / "Library" / "Application Support";
#else
         auto base = env("XDG_CONFIG_HOME");
         if (base.empty())
            base = env("HOME") / ".config";
#endif
         if (base.empty())
            return {};
         return base / info().vendor / info().name;
      }

      bool read_presets(fs::path const& file, std::map<std::string, json>& into)
      {
         std::error_code ec;
         if (file.empty() || !fs::exists(file, ec))
            return false;

         std::ifstream in(file);
         auto j = json::parse(in, nullptr, false);
         if (!j.is_object())
         {
            QPLUG_LOG(app, "presets: {} is not a JSON object", file.string());
            return false;
         }

         for (auto& [name, state] : j.items())
            if (state.is_object())
               into[name] = std::move(state);
         return true;
      }
   }

   struct controller::presets
   {
      presets()
      {
         auto factory = artist::find_file("factory_presets.json");
         if (!factory.empty())
            read_presets(factory, _factory);

         _user_file = user_presets_dir() / "presets.json";
         read_presets(_user_file, _user);
      }

      bool write_user() const
      {
         std::error_code ec;
         fs::create_directories(_user_file.parent_path(), ec);
         if (ec)
            return false;

         json j = json::object();
         for (auto const& [name, state] : _user)
            j[name] = state;

         std::ofstream out(_user_file);
         out << j.dump(3) << '\n';
         return bool(out);
      }

      std::map<std::string, json>   _factory;
      std::map<std::string, json>   _user;
      fs::path                      _user_file;
   };

   controller::controller() = default;
   controller::~controller() = default;

   controller::presets& controller::get_presets() const
   {
      if (!_presets)
         _presets = std::make_unique<presets>();
      return *_presets;
   }

   std::vector<std::string> controller::preset_names() const
   {
      auto& p = get_presets();
      std::vector<std::string> names;
      for (auto const& [name, state] : p._factory)
         names.push_back(name);
      for (auto const& [name, state] : p._user)
         if (!p._factory.count(name))
            names.push_back(name);
      return names;
   }

   bool controller::has_preset(std::string_view name) const
   {
      auto& p = get_presets();
      std::string key{name};
      return p._user.count(key) || p._factory.count(key);
   }

   bool controller::is_factory_preset(std::string_view name) const
   {
      return get_presets()._factory.count(std::string{name}) != 0;
   }

   // The host is told about a preset the plugin loaded itself. state()
   // alone only sets the values, which is right when it is the host that
   // is loading, but a preset chosen in the editor is the plugin's own
   // edit: unannounced, the host's automation would still hold the values
   // the preset replaced.
   void controller::send_edits()
   {
      if (!_sink)
         return;

      auto params = parameters();
      for (int i = 0; i != _size; ++i)
      {
         if (!params[i]._save_in_preset)
            continue;
         _sink->begin_edit(i);
         _sink->edit_parameter(i, get_parameter(i));
         _sink->end_edit(i);
      }
   }

   bool controller::load_preset(std::string_view name)
   {
      auto& p = get_presets();
      std::string key{name};

      json const* j = nullptr;
      if (auto i = p._user.find(key); i != p._user.end())
         j = &i->second;
      else if (auto i = p._factory.find(key); i != p._factory.end())
         j = &i->second;

      if (!j || !state(*j))
         return false;

      send_edits();
      return true;
   }

   bool controller::save_preset(std::string_view name)
   {
      // The state, less what belongs to the session rather than the
      // sound: a preset that zoomed the window would be a surprise.
      auto j = state();
      j.erase("view");

      auto& p = get_presets();
      p._user[std::string{name}] = std::move(j);
      return p.write_user();
   }

   bool controller::delete_preset(std::string_view name)
   {
      auto& p = get_presets();
      if (!p._user.erase(std::string{name}))
         return false;
      return p.write_user();
   }

   ////////////////////////////////////////////////////////////////////////////
   int controller::index_of(parameter::id_type id) const
   {
      auto params = parameters();
      for (int i = 0; i != _size; ++i)
         if (params[i]._id == id)
            return i;
      return -1;
   }

   int controller::index_of(std::string_view name) const
   {
      auto params = parameters();
      for (int i = 0; i != _size; ++i)
         if (params[i]._name == name)
            return i;
      return -1;
   }
}
