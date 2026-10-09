/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Two different plugins in one host, as a DAW chains them: each built
// product loaded from its .clap, an instance of each made, and both editors
// open at once in windows of the host's. Each plugin carries its own copy of
// Elements and Artist, so anything one copy keeps for the whole process (a
// window class, a factory) can collide with the other's, or outlive its
// library. Then both are unloaded, and one is loaded again, as a host does
// when a project is closed and reopened. A failure here can be a hang (a
// message box) or a crash at unload, so ctest gives the test a timeout.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <clap/clap.h>
#include "host_window.hpp"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
# include <windows.h>
#else
# include <dlfcn.h>
#endif

using namespace q_plug_test;

namespace
{
   struct plugin
   {
      std::string                         path;
      void*                               lib = nullptr;
      clap_plugin_entry_t const*          entry = nullptr;
      clap_plugin_t const*                plug = nullptr;
      clap_plugin_gui_t const*            gui = nullptr;
      clap_plugin_timer_support_t const*  timer = nullptr;
      clap_host_t                         host{};
      clap_id                             timer_id = CLAP_INVALID_ID;
      clap_window_t                       window{};
   };

   // A built product, where QPlug puts it: in products, or products/CLAP
   // on Windows. The lookup validate.sh makes.
   std::string find_clap(char const* name)
   {
      std::string const dir = Q_PLUG_PRODUCTS;
      for (auto sub : {"/", "/CLAP/"})
      {
         auto path = dir + sub + name + ".clap";
         if (std::filesystem::exists(path))
            return path;
      }
      return {};
   }

   // The library inside a .clap: the file itself on Windows and Linux, the
   // binary inside the bundle on macOS.
   std::string binary(std::string const& clap)
   {
#if defined(__APPLE__)
      auto slash = clap.find_last_of('/');
      auto stem = clap.substr(slash + 1, clap.size() - slash - 1 - 5);
      return clap + "/Contents/MacOS/" + stem;
#else
      return clap;
#endif
   }

   void* load_library(std::string const& path)
   {
#if defined(_WIN32)
      return LoadLibraryA(path.c_str());
#else
      return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
   }

   void* symbol(void* lib, char const* name)
   {
#if defined(_WIN32)
      return reinterpret_cast<void*>(
         GetProcAddress(static_cast<HMODULE>(lib), name));
#else
      return dlsym(lib, name);
#endif
   }

   void unload_library(void* lib)
   {
#if defined(_WIN32)
      FreeLibrary(static_cast<HMODULE>(lib));
#else
      dlclose(lib);
#endif
   }

   // The host's timer, which a plugin's editor uses where the window
   // system has no thread of its own to deliver its events (X11).
   clap_host_timer_support_t const host_timer =
   {
      [](clap_host_t const* h, std::uint32_t, clap_id* id) -> bool
      {
         auto p = static_cast<plugin*>(h->host_data);
         p->timer_id = 1;
         *id = p->timer_id;
         return true;
      },
      [](clap_host_t const* h, clap_id) -> bool
      {
         static_cast<plugin*>(h->host_data)->timer_id = CLAP_INVALID_ID;
         return true;
      }
   };

   bool load(plugin& p)
   {
      p.lib = load_library(binary(p.path));
      if (!p.lib)
         return false;
      p.entry = static_cast<clap_plugin_entry_t const*>(
         symbol(p.lib, "clap_entry"));
      if (!p.entry || !p.entry->init(p.path.c_str()))
         return false;

      auto factory = static_cast<clap_plugin_factory_t const*>(
         p.entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
      if (!factory || factory->get_plugin_count(factory) == 0)
         return false;
      auto desc = factory->get_plugin_descriptor(factory, 0);

      p.host.clap_version = CLAP_VERSION;
      p.host.host_data = &p;
      p.host.name = "q_plug two plugins test";
      p.host.vendor = "QPlug";
      p.host.url = "";
      p.host.version = "1.0";
      p.host.get_extension = [](clap_host_t const*, char const* id)
         -> void const*
      {
         if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT))
            return &host_timer;
         return nullptr;
      };
      p.host.request_restart = [](clap_host_t const*) {};
      p.host.request_process = [](clap_host_t const*) {};
      p.host.request_callback = [](clap_host_t const*) {};

      p.plug = factory->create_plugin(factory, &p.host, desc->id);
      if (!p.plug || !p.plug->init(p.plug))
         return false;
      p.gui = static_cast<clap_plugin_gui_t const*>(
         p.plug->get_extension(p.plug, CLAP_EXT_GUI));
      p.timer = static_cast<clap_plugin_timer_support_t const*>(
         p.plug->get_extension(p.plug, CLAP_EXT_TIMER_SUPPORT));
      return p.gui != nullptr;
   }

   bool open_editor(plugin& p)
   {
      if (!p.gui->create(p.plug, window_api(), false))
         return false;
      std::uint32_t w = 0, h = 0;
      if (!p.gui->get_size(p.plug, &w, &h) || w == 0 || h == 0)
         return false;
      p.window = open_window(int(w), int(h));
      return p.gui->set_parent(p.plug, &p.window)
         && p.gui->show(p.plug);
   }

   void close_editor(plugin& p)
   {
      p.gui->hide(p.plug);
      p.gui->destroy(p.plug);
      close_window(p.window);
   }

   void unload(plugin& p)
   {
      if (p.plug)
         p.plug->destroy(p.plug);
      if (p.entry)
         p.entry->deinit();
      if (p.lib)
         unload_library(p.lib);
      p = plugin{p.path};
   }

   // Events for the windows, and the timer ticks the editors asked for.
   void run(std::vector<plugin*> const& ps, int ms)
   {
      for (int t = 0; t < ms; t += 16)
      {
         run_events(16);
         for (auto p : ps)
            if (p->timer && p->timer_id != CLAP_INVALID_ID)
               p->timer->on_timer(p->plug, p->timer_id);
      }
   }
}

TEST_CASE("Two different plugins open their editors in one host")
{
   plugin gain{find_clap("QPlug Gain")};
   plugin dexter{find_clap("Dexter")};
   REQUIRE(!gain.path.empty());
   REQUIRE(!dexter.path.empty());

   REQUIRE(load(gain));
   REQUIRE(load(dexter));

   REQUIRE(open_editor(gain));
   REQUIRE(open_editor(dexter));
   run({&gain, &dexter}, 500);

   close_editor(dexter);
   close_editor(gain);
   run({&gain, &dexter}, 100);

   unload(dexter);
   unload(gain);

   // Loaded again after it was unloaded, as when a project is reopened:
   // whatever the first load left registered must not be in the way.
   REQUIRE(load(gain));
   REQUIRE(open_editor(gain));
   run({&gain}, 300);
   close_editor(gain);
   unload(gain);
}
