/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_BASE_PLUGIN_HPP_SEPTEMBER_6_2026)
#define QPLUG_BASE_PLUGIN_HPP_SEPTEMBER_6_2026

#include <qplug/parameter.hpp>
#include <qplug/data_stream.hpp>
#include <q/support/audio_stream.hpp>
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
   };

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

      virtual std::uint32_t   inputs() const = 0;
      virtual std::uint32_t   outputs() const = 0;

      virtual parameter_list  parameters() const = 0;
      virtual double          get_parameter(int id) const = 0;
      virtual void            set_parameter(int id, double value) = 0;

      virtual bool            save_state(ostream& out) const = 0;
      virtual bool            load_state(istream& in) = 0;

   private:

      friend struct base_plugin_impl;
      base_plugin_impl*       _impl;
   };
}

#endif
