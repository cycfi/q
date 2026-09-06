/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_BASE_PLUGIN_HPP_SEPTEMBER_6_2026)
#define QPLUG_BASE_PLUGIN_HPP_SEPTEMBER_6_2026

#include <qplug/parameter.hpp>
#include <qplug/data_stream.hpp>
#include <q/support/audio_stream.hpp>
#include <elements/base_view.hpp>
#include <infra/iterator_range.hpp>
#include <infra/support.hpp>
#include <cstdint>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Plugin identity, as the host sees it
   ////////////////////////////////////////////////////////////////////////////
   struct plugin_info
   {
      char const*             id;
      char const*             name;
      char const*             vendor;
      char const*             url;
      char const*             manual_url;
      char const*             support_url;
      char const*             version;
      char const*             description;
      char const* const*      features;      // null terminated
      elements::extent        view_size;     // the size the GUI opens with
   };

   ////////////////////////////////////////////////////////////////////////////
   // A channel layout the plugin can run in: N inputs to M outputs on its
   // one main port each way. A plugin lists the layouts it supports; the
   // host picks one before activation.
   ////////////////////////////////////////////////////////////////////////////
   struct channel_config
   {
      std::uint32_t           inputs;
      std::uint32_t           outputs;
   };

   using channel_config_list = iterator_range<channel_config const*>;

   ////////////////////////////////////////////////////////////////////////////
   // The base plugin
   //
   // The host adapter. This header knows nothing about any plugin format.
   // Each format supplies one translation unit that implements the members
   // below and calls back into the virtuals (see src/clap/base_plugin.cpp).
   ////////////////////////////////////////////////////////////////////////////
   struct base_plugin_impl;

   class base_plugin : non_copyable
   {
   public:

      using in_channels = q::audio_stream_base::in_channels;
      using out_channels = q::audio_stream_base::out_channels;
      using parameter_list = iterator_range<parameter const*>;

                              base_plugin();
      virtual                 ~base_plugin();

   protected:

      virtual bool            init() { return true; }
      virtual bool            activate(std::uint32_t sps
                               , std::uint32_t min_frames
                               , std::uint32_t max_frames) = 0;
      virtual void            deactivate() = 0;
      virtual void            on_main_thread() {}

      // Called on the audio thread. Must not block nor allocate.
      virtual bool            start_processing() { return true; }
      virtual void            stop_processing() {}
      virtual void            reset() = 0;
      virtual void            process(in_channels const& in
                               , out_channels const& out) = 0;

      // The layouts on offer, the one in use, and the host's choice. The
      // choice is made on the main thread while the plugin is inactive.
      virtual channel_config_list
                              channel_configs() const = 0;
      virtual channel_config  channels() const = 0;
      virtual bool            set_channels(channel_config config) = 0;

      virtual parameter_list  parameters() const = 0;
      virtual double          get_parameter(int id) const = 0;
      virtual void            set_parameter(int id, double value) = 0;

      virtual bool            save_state(ostream& out) const = 0;
      virtual bool            load_state(istream& in) = 0;

      // The GUI, all main thread. `parent` is the host's native view, an
      // NSView* on macOS, passed untyped so this header stays free of
      // platform types.
      virtual bool            has_view() const = 0;
      virtual bool            create_view() = 0;
      virtual elements::extent
                              view_size() const = 0;
      virtual elements::view_limits
                              view_limits() const = 0;
      virtual bool            resize_view(elements::extent size) = 0;
      virtual bool            attach_view(void* parent) = 0;
      virtual void            detach_view() = 0;
      virtual void            show_view(bool show) = 0;
      virtual bool            scale_view(double scale) = 0;

      // Parameter edits made in the GUI, forwarded to the host. Main thread.
      void                    begin_edit(int id);
      void                    edit_parameter(int id, double value);
      void                    end_edit(int id);

      // The GUI asks the host for a new size. Main thread.
      bool                    request_view_resize(elements::extent size);

   private:

      friend struct base_plugin_impl;
      base_plugin_impl*       _impl;
   };
}

#endif
