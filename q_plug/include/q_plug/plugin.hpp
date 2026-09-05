/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PLUGIN_HPP_SEPTEMBER_5_2026)
#define QPLUG_PLUGIN_HPP_SEPTEMBER_5_2026

#include <clap/clap.h>
#include <cstdint>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // The plugin
   ////////////////////////////////////////////////////////////////////////////
   class plugin
   {
   public:

      using descriptor = clap_plugin_descriptor_t;
      using process_status = clap_process_status;

                              plugin(descriptor const* desc
                               , clap_host_t const* host);
                              plugin(plugin const&) = delete;
      virtual                 ~plugin() = default;

      plugin&                 operator=(plugin const&) = delete;

      clap_plugin_t const*    handle() const { return &_plugin; }
      clap_host_t const*      host() const { return _host; }

   protected:

      virtual bool            init() { return true; }
      virtual bool            activate(double sps, std::uint32_t min_frames
                               , std::uint32_t max_frames);
      virtual void            deactivate() {}

      // Called on the audio thread. Must not block nor allocate.
      virtual bool            start_processing() { return true; }
      virtual void            stop_processing() {}
      virtual void            reset() {}
      virtual process_status  process(clap_process_t const& proc);

      virtual void            on_main_thread() {}
      virtual void const*     extension(char const* /*id*/) { return nullptr; }

      static plugin&          self(clap_plugin_t const* p);

   private:

      static bool             clap_init(clap_plugin_t const* p);
      static void             clap_destroy(clap_plugin_t const* p);
      static bool             clap_activate(clap_plugin_t const* p, double sps
                               , std::uint32_t min_frames
                               , std::uint32_t max_frames);
      static void             clap_deactivate(clap_plugin_t const* p);
      static bool             clap_start_processing(clap_plugin_t const* p);
      static void             clap_stop_processing(clap_plugin_t const* p);
      static void             clap_reset(clap_plugin_t const* p);
      static process_status   clap_process(clap_plugin_t const* p
                               , clap_process_t const* proc);
      static void const*      clap_extension(clap_plugin_t const* p
                               , char const* id);
      static void             clap_on_main_thread(clap_plugin_t const* p);

      clap_plugin_t           _plugin;
      clap_host_t const*      _host;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline plugin::plugin(descriptor const* desc, clap_host_t const* host)
    : _host(host)
   {
      _plugin.desc = desc;
      _plugin.plugin_data = this;
      _plugin.init = clap_init;
      _plugin.destroy = clap_destroy;
      _plugin.activate = clap_activate;
      _plugin.deactivate = clap_deactivate;
      _plugin.start_processing = clap_start_processing;
      _plugin.stop_processing = clap_stop_processing;
      _plugin.reset = clap_reset;
      _plugin.process = clap_process;
      _plugin.get_extension = clap_extension;
      _plugin.on_main_thread = clap_on_main_thread;
   }

   inline bool plugin::activate(double, std::uint32_t, std::uint32_t)
   {
      return true;
   }

   inline plugin::process_status plugin::process(clap_process_t const&)
   {
      return CLAP_PROCESS_SLEEP;
   }

   inline plugin& plugin::self(clap_plugin_t const* p)
   {
      return *static_cast<plugin*>(p->plugin_data);
   }

   inline bool plugin::clap_init(clap_plugin_t const* p)
   {
      return self(p).init();
   }

   inline void plugin::clap_destroy(clap_plugin_t const* p)
   {
      delete &self(p);
   }

   inline bool plugin::clap_activate(clap_plugin_t const* p, double sps
    , std::uint32_t min_frames, std::uint32_t max_frames)
   {
      return self(p).activate(sps, min_frames, max_frames);
   }

   inline void plugin::clap_deactivate(clap_plugin_t const* p)
   {
      self(p).deactivate();
   }

   inline bool plugin::clap_start_processing(clap_plugin_t const* p)
   {
      return self(p).start_processing();
   }

   inline void plugin::clap_stop_processing(clap_plugin_t const* p)
   {
      self(p).stop_processing();
   }

   inline void plugin::clap_reset(clap_plugin_t const* p)
   {
      self(p).reset();
   }

   inline plugin::process_status
   plugin::clap_process(clap_plugin_t const* p, clap_process_t const* proc)
   {
      return self(p).process(*proc);
   }

   inline void const*
   plugin::clap_extension(clap_plugin_t const* p, char const* id)
   {
      return self(p).extension(id);
   }

   inline void plugin::clap_on_main_thread(clap_plugin_t const* p)
   {
      self(p).on_main_thread();
   }
}

#endif
