/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/plugin.hpp>
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace qplug = cycfi::qplug;

namespace
{
   ////////////////////////////////////////////////////////////////////////////
   // The gain plugin
   ////////////////////////////////////////////////////////////////////////////
   class gain : public qplug::plugin
   {
   public:
                              gain(descriptor const* desc
                               , clap_host_t const* host);

   protected:

      bool                    init() override;
      process_status          process(clap_process_t const& proc) override;
      void const*             extension(char const* id) override;

   private:

      static constexpr clap_id volume_id = 0;
      static constexpr double  volume_min = 0.0;
      static constexpr double  volume_max = 2.0;
      static constexpr double  volume_def = 1.0;

      using ports_info = clap_audio_port_info_t;
      using param_info = clap_param_info_t;
      using in_events = clap_input_events_t;
      using out_events = clap_output_events_t;

      static gain&            self(clap_plugin_t const* p);
      void                    apply_events(in_events const* in);

      // clap.audio-ports
      static uint32_t         ports_count(clap_plugin_t const*, bool);
      static bool             ports_get(clap_plugin_t const*, uint32_t index
                               , bool is_input, ports_info* info);

      // clap.params
      static uint32_t         params_count(clap_plugin_t const*);
      static bool             params_info(clap_plugin_t const*, uint32_t index
                               , param_info* info);
      static bool             params_value(clap_plugin_t const* p, clap_id id
                               , double* value);
      static bool             params_to_text(clap_plugin_t const*, clap_id id
                               , double value, char* text, uint32_t size);
      static bool             params_from_text(clap_plugin_t const*, clap_id id
                               , char const* text, double* value);
      static void             params_flush(clap_plugin_t const* p
                               , in_events const* in, out_events const* out);

      // clap.state
      static bool             state_save(clap_plugin_t const* p
                               , clap_ostream_t const* stream);
      static bool             state_load(clap_plugin_t const* p
                               , clap_istream_t const* stream);

      static clap_plugin_audio_ports_t const  s_audio_ports;
      static clap_plugin_params_t const       s_params;
      static clap_plugin_state_t const        s_state;

      double                  _volume;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Lifecycle
   ////////////////////////////////////////////////////////////////////////////
   gain::gain(descriptor const* desc, clap_host_t const* host)
    : plugin(desc, host)
    , _volume(volume_def)
   {}

   bool gain::init()
   {
      _volume = volume_def;
      return true;
   }

   gain::process_status gain::process(clap_process_t const& proc)
   {
      if (proc.in_events)
         apply_events(proc.in_events);

      auto frames = proc.frames_count;
      auto g = float(_volume);

      for (uint32_t ch = 0; ch < 2; ++ch)
      {
         float const* in = proc.audio_inputs[0].data32[ch];
         float* out = proc.audio_outputs[0].data32[ch];
         for (uint32_t f = 0; f < frames; ++f)
            out[f] = in[f] * g;
      }

      return CLAP_PROCESS_CONTINUE;
   }

   void const* gain::extension(char const* id)
   {
      if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
         return &s_audio_ports;
      if (!std::strcmp(id, CLAP_EXT_PARAMS))
         return &s_params;
      if (!std::strcmp(id, CLAP_EXT_STATE))
         return &s_state;
      return nullptr;
   }

   gain& gain::self(clap_plugin_t const* p)
   {
      return static_cast<gain&>(plugin::self(p));
   }

   void gain::apply_events(in_events const* in)
   {
      auto n = in->size(in);
      for (uint32_t i = 0; i < n; ++i)
      {
         auto hdr = in->get(in, i);
         if (hdr->type == CLAP_EVENT_PARAM_VALUE
            && hdr->space_id == CLAP_CORE_EVENT_SPACE_ID)
         {
            auto ev = reinterpret_cast<clap_event_param_value_t const*>(hdr);
            if (ev->param_id == volume_id)
               _volume = ev->value;
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // clap.audio-ports
   ////////////////////////////////////////////////////////////////////////////
   uint32_t gain::ports_count(clap_plugin_t const*, bool)
   {
      return 1;
   }

   bool gain::ports_get(clap_plugin_t const*, uint32_t index
    , bool is_input, ports_info* info)
   {
      if (index != 0)
         return false;

      info->id = 0;
      info->channel_count = 2;
      info->flags = CLAP_AUDIO_PORT_IS_MAIN;
      info->port_type = CLAP_PORT_STEREO;
      info->in_place_pair = CLAP_INVALID_ID;
      std::snprintf(info->name, sizeof(info->name), "%s"
       , is_input ? "Input" : "Output");
      return true;
   }

   clap_plugin_audio_ports_t const gain::s_audio_ports =
   {
      ports_count,
      ports_get
   };

   ////////////////////////////////////////////////////////////////////////////
   // clap.params
   ////////////////////////////////////////////////////////////////////////////
   uint32_t gain::params_count(clap_plugin_t const*)
   {
      return 1;
   }

   bool gain::params_info(clap_plugin_t const*, uint32_t index
    , param_info* info)
   {
      if (index != 0)
         return false;

      info->id = volume_id;
      info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
      info->min_value = volume_min;
      info->max_value = volume_max;
      info->default_value = volume_def;
      info->cookie = nullptr;
      std::snprintf(info->name, sizeof(info->name), "Volume");
      info->module[0] = 0;
      return true;
   }

   bool gain::params_value(clap_plugin_t const* p, clap_id id, double* value)
   {
      if (id != volume_id)
         return false;
      *value = self(p)._volume;
      return true;
   }

   bool gain::params_to_text(clap_plugin_t const*, clap_id id
    , double value, char* text, uint32_t size)
   {
      if (id != volume_id)
         return false;
      std::snprintf(text, size, "%.3f", value);
      return true;
   }

   bool gain::params_from_text(clap_plugin_t const*, clap_id id
    , char const* text, double* value)
   {
      if (id != volume_id)
         return false;
      *value = std::atof(text);
      return true;
   }

   void gain::params_flush(clap_plugin_t const* p
    , in_events const* in, out_events const*)
   {
      self(p).apply_events(in);
   }

   clap_plugin_params_t const gain::s_params =
   {
      params_count,
      params_info,
      params_value,
      params_to_text,
      params_from_text,
      params_flush
   };

   ////////////////////////////////////////////////////////////////////////////
   // clap.state
   ////////////////////////////////////////////////////////////////////////////
   bool gain::state_save(clap_plugin_t const* p, clap_ostream_t const* stream)
   {
      auto& v = self(p)._volume;
      auto written = stream->write(stream, &v, sizeof(v));
      return written == int64_t(sizeof(v));
   }

   bool gain::state_load(clap_plugin_t const* p, clap_istream_t const* stream)
   {
      double v = volume_def;
      auto n = stream->read(stream, &v, sizeof(v));
      if (n != int64_t(sizeof(v)))
         return false;
      self(p)._volume = v;
      return true;
   }

   clap_plugin_state_t const gain::s_state =
   {
      state_save,
      state_load
   };

   ////////////////////////////////////////////////////////////////////////////
   // Descriptor and factory
   ////////////////////////////////////////////////////////////////////////////
   char const* const features[] =
   {
      CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
      CLAP_PLUGIN_FEATURE_STEREO,
      nullptr
   };

   clap_plugin_descriptor_t const desc =
   {
      CLAP_VERSION,
      "com.qplug.gain",
      "QPlug Gain",
      "QPlug",
      "",                        // url
      "",                        // manual_url
      "",                        // support_url
      "0.1.0",
      "Simple stereo gain plugin",
      features
   };

   uint32_t factory_count(clap_plugin_factory_t const*)
   {
      return 1;
   }

   clap_plugin_descriptor_t const*
   factory_descriptor(clap_plugin_factory_t const*, uint32_t index)
   {
      return index == 0 ? &desc : nullptr;
   }

   clap_plugin_t const* factory_create(clap_plugin_factory_t const*
    , clap_host_t const* host, char const* id)
   {
      if (std::strcmp(id, desc.id) != 0)
         return nullptr;
      return (new gain(&desc, host))->handle();
   }

   clap_plugin_factory_t const factory =
   {
      factory_count,
      factory_descriptor,
      factory_create
   };
}

///////////////////////////////////////////////////////////////////////////////
// Entry functions. The entry shim (qplug_entry.cpp) links these by name to
// build the exported clap_entry for each plugin format.
///////////////////////////////////////////////////////////////////////////////
extern "C" bool qplug_entry_init(char const* /*plugin_path*/)
{
   return true;
}

extern "C" void qplug_entry_deinit()
{}

extern "C" void const* qplug_entry_get_factory(char const* factory_id)
{
   if (!std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
      return &factory;
   return nullptr;
}
