/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/plugin.hpp>
#include <clap/clap.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// The one translation unit that knows CLAP. It implements base_plugin's
// members, translates every host call into a neutral virtual, and provides
// the descriptor, factory and entry point.

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Stream adapters
   ////////////////////////////////////////////////////////////////////////////
   namespace
   {
      struct clap_ostream_adapter : ostream
      {
                              clap_ostream_adapter(clap_ostream_t const* s)
                               : _s(s) {}

         std::int64_t         write(void const* data
                               , std::int64_t size) override
                              {
                                 return _s->write(_s, data, size);
                              }

         clap_ostream_t const* _s;
      };

      struct clap_istream_adapter : istream
      {
                              clap_istream_adapter(clap_istream_t const* s)
                               : _s(s) {}

         std::int64_t         read(void* data, std::int64_t size) override
                              {
                                 return _s->read(_s, data, size);
                              }

         clap_istream_t const* _s;
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // The impl: owns the clap_plugin_t and hosts the thunks
   ////////////////////////////////////////////////////////////////////////////
   struct base_plugin_impl
   {
                              base_plugin_impl(base_plugin& self
                               , clap_plugin_descriptor_t const* desc);

      static base_plugin&     self(clap_plugin_t const* p);
      static clap_plugin_t const* handle(base_plugin& p);

      // clap_plugin
      static bool             init(clap_plugin_t const* p);
      static void             destroy(clap_plugin_t const* p);
      static bool             activate(clap_plugin_t const* p, double sps
                               , uint32_t min_frames, uint32_t max_frames);
      static void             deactivate(clap_plugin_t const* p);
      static bool             start_processing(clap_plugin_t const* p);
      static void             stop_processing(clap_plugin_t const* p);
      static void             reset(clap_plugin_t const* p);
      static clap_process_status
                              process(clap_plugin_t const* p
                               , clap_process_t const* proc);
      static void const*      extension(clap_plugin_t const* p
                               , char const* id);
      static void             on_main_thread(clap_plugin_t const* p);

      // clap.audio-ports
      static uint32_t         ports_count(clap_plugin_t const* p
                               , bool is_input);
      static bool             ports_get(clap_plugin_t const* p, uint32_t index
                               , bool is_input, clap_audio_port_info_t* info);

      // clap.params
      static uint32_t         params_count(clap_plugin_t const* p);
      static bool             params_info(clap_plugin_t const* p
                               , uint32_t index, clap_param_info_t* info);
      static bool             params_value(clap_plugin_t const* p, clap_id id
                               , double* value);
      static bool             params_to_text(clap_plugin_t const* p, clap_id id
                               , double value, char* text, uint32_t size);
      static bool             params_from_text(clap_plugin_t const* p
                               , clap_id id, char const* text, double* value);
      static void             params_flush(clap_plugin_t const* p
                               , clap_input_events_t const* in
                               , clap_output_events_t const* out);
      static void             apply_events(base_plugin& p
                               , clap_input_events_t const* in);

      // clap.state
      static bool             state_save(clap_plugin_t const* p
                               , clap_ostream_t const* stream);
      static bool             state_load(clap_plugin_t const* p
                               , clap_istream_t const* stream);

      static clap_plugin_audio_ports_t const   s_audio_ports;
      static clap_plugin_params_t const        s_params;
      static clap_plugin_state_t const         s_state;

      clap_plugin_t           _plugin;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Descriptor, built once from the client's plugin_info
   ////////////////////////////////////////////////////////////////////////////
   namespace
   {
      clap_plugin_descriptor_t const& descriptor()
      {
         static clap_plugin_descriptor_t const desc = []
         {
            auto const& i = info();
            clap_plugin_descriptor_t d{};
            d.clap_version = CLAP_VERSION;
            d.id = i.id;
            d.name = i.name;
            d.vendor = i.vendor;
            d.url = i.url;
            d.manual_url = i.manual_url;
            d.support_url = i.support_url;
            d.version = i.version;
            d.description = i.description;
            d.features = i.features;
            return d;
         }();
         return desc;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // base_plugin members
   ////////////////////////////////////////////////////////////////////////////
   base_plugin::base_plugin()
    : _impl(new base_plugin_impl(*this, &descriptor()))
   {}

   base_plugin::~base_plugin()
   {
      delete _impl;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Impl: lifecycle
   ////////////////////////////////////////////////////////////////////////////
   base_plugin_impl::base_plugin_impl(base_plugin& self
    , clap_plugin_descriptor_t const* desc)
   {
      _plugin.desc = desc;
      _plugin.plugin_data = &self;
      _plugin.init = init;
      _plugin.destroy = destroy;
      _plugin.activate = activate;
      _plugin.deactivate = deactivate;
      _plugin.start_processing = start_processing;
      _plugin.stop_processing = stop_processing;
      _plugin.reset = reset;
      _plugin.process = process;
      _plugin.get_extension = extension;
      _plugin.on_main_thread = on_main_thread;
   }

   base_plugin& base_plugin_impl::self(clap_plugin_t const* p)
   {
      return *static_cast<base_plugin*>(p->plugin_data);
   }

   clap_plugin_t const* base_plugin_impl::handle(base_plugin& p)
   {
      return &p._impl->_plugin;
   }

   bool base_plugin_impl::init(clap_plugin_t const* p)
   {
      return self(p).init();
   }

   void base_plugin_impl::destroy(clap_plugin_t const* p)
   {
      delete &self(p);
   }

   bool base_plugin_impl::activate(clap_plugin_t const* p, double sps
    , uint32_t min_frames, uint32_t max_frames)
   {
      return self(p).activate(uint32_t(sps), min_frames, max_frames);
   }

   void base_plugin_impl::deactivate(clap_plugin_t const* p)
   {
      self(p).deactivate();
   }

   bool base_plugin_impl::start_processing(clap_plugin_t const* p)
   {
      return self(p).start_processing();
   }

   void base_plugin_impl::stop_processing(clap_plugin_t const* p)
   {
      self(p).stop_processing();
   }

   void base_plugin_impl::reset(clap_plugin_t const* p)
   {
      self(p).reset();
   }

   clap_process_status base_plugin_impl::process(clap_plugin_t const* p
    , clap_process_t const* proc)
   {
      auto& plug = self(p);
      if (proc->in_events)
         apply_events(plug, proc->in_events);

      auto const& ai = proc->audio_inputs[0];
      auto const& ao = proc->audio_outputs[0];
      auto frames = proc->frames_count;

      base_plugin::in_channels in(
         const_cast<float const**>(ai.data32), ai.channel_count, frames);
      base_plugin::out_channels out(ao.data32, ao.channel_count, frames);

      plug.process(in, out);
      return CLAP_PROCESS_CONTINUE;
   }

   void const*
   base_plugin_impl::extension(clap_plugin_t const*, char const* id)
   {
      if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
         return &s_audio_ports;
      if (!std::strcmp(id, CLAP_EXT_PARAMS))
         return &s_params;
      if (!std::strcmp(id, CLAP_EXT_STATE))
         return &s_state;
      return nullptr;
   }

   void base_plugin_impl::on_main_thread(clap_plugin_t const* p)
   {
      self(p).on_main_thread();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.audio-ports
   ////////////////////////////////////////////////////////////////////////////
   uint32_t base_plugin_impl::ports_count(clap_plugin_t const*, bool)
   {
      return 1;
   }

   bool base_plugin_impl::ports_get(clap_plugin_t const* p, uint32_t index
    , bool is_input, clap_audio_port_info_t* info)
   {
      if (index != 0)
         return false;

      auto& plug = self(p);
      info->id = 0;
      info->channel_count = is_input ? plug.inputs() : plug.outputs();
      info->flags = CLAP_AUDIO_PORT_IS_MAIN;
      info->port_type = info->channel_count == 2 ? CLAP_PORT_STEREO
                                                 : CLAP_PORT_MONO;
      info->in_place_pair = CLAP_INVALID_ID;
      std::snprintf(info->name, sizeof(info->name), "%s"
       , is_input ? "Input" : "Output");
      return true;
   }

   clap_plugin_audio_ports_t const base_plugin_impl::s_audio_ports =
   {
      ports_count,
      ports_get
   };

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.params
   ////////////////////////////////////////////////////////////////////////////
   uint32_t base_plugin_impl::params_count(clap_plugin_t const* p)
   {
      return uint32_t(self(p).parameters().size());
   }

   bool base_plugin_impl::params_info(clap_plugin_t const* p, uint32_t index
    , clap_param_info_t* info)
   {
      auto params = self(p).parameters();
      if (index >= params.size())
         return false;

      auto const& param = params[index];
      info->id = index;
      info->flags = CLAP_PARAM_IS_MODULATABLE;
      if (param._can_automate)
         info->flags |= CLAP_PARAM_IS_AUTOMATABLE;
      info->min_value = param._min;
      info->max_value = param._max;
      info->default_value = param._init;
      info->cookie = nullptr;
      std::snprintf(info->name, sizeof(info->name), "%s", param._name);
      info->module[0] = 0;
      return true;
   }

   bool base_plugin_impl::params_value(clap_plugin_t const* p, clap_id id
    , double* value)
   {
      auto& plug = self(p);
      if (id >= plug.parameters().size())
         return false;
      *value = plug.get_parameter(int(id));
      return true;
   }

   bool base_plugin_impl::params_to_text(clap_plugin_t const* p, clap_id id
    , double value, char* text, uint32_t size)
   {
      if (id >= self(p).parameters().size())
         return false;
      std::snprintf(text, size, "%.3f", value);
      return true;
   }

   bool base_plugin_impl::params_from_text(clap_plugin_t const* p, clap_id id
    , char const* text, double* value)
   {
      if (id >= self(p).parameters().size())
         return false;
      *value = std::atof(text);
      return true;
   }

   void base_plugin_impl::params_flush(clap_plugin_t const* p
    , clap_input_events_t const* in, clap_output_events_t const*)
   {
      apply_events(self(p), in);
   }

   void base_plugin_impl::apply_events(base_plugin& plug
    , clap_input_events_t const* in)
   {
      auto count = plug.parameters().size();
      auto n = in->size(in);
      for (uint32_t i = 0; i != n; ++i)
      {
         auto hdr = in->get(in, i);
         if (hdr->type == CLAP_EVENT_PARAM_VALUE
            && hdr->space_id == CLAP_CORE_EVENT_SPACE_ID)
         {
            auto ev = reinterpret_cast<clap_event_param_value_t const*>(hdr);
            if (ev->param_id < count)
               plug.set_parameter(int(ev->param_id), ev->value);
         }
      }
   }

   clap_plugin_params_t const base_plugin_impl::s_params =
   {
      params_count,
      params_info,
      params_value,
      params_to_text,
      params_from_text,
      params_flush
   };

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.state
   ////////////////////////////////////////////////////////////////////////////
   bool base_plugin_impl::state_save(clap_plugin_t const* p
    , clap_ostream_t const* stream)
   {
      clap_ostream_adapter out(stream);
      return self(p).save_state(out);
   }

   bool base_plugin_impl::state_load(clap_plugin_t const* p
    , clap_istream_t const* stream)
   {
      clap_istream_adapter in(stream);
      return self(p).load_state(in);
   }

   clap_plugin_state_t const base_plugin_impl::s_state =
   {
      state_save,
      state_load
   };

   ////////////////////////////////////////////////////////////////////////////
   // Factory
   ////////////////////////////////////////////////////////////////////////////
   namespace
   {
      uint32_t factory_count(clap_plugin_factory_t const*)
      {
         return 1;
      }

      clap_plugin_descriptor_t const*
      factory_descriptor(clap_plugin_factory_t const*, uint32_t index)
      {
         return index == 0 ? &descriptor() : nullptr;
      }

      clap_plugin_t const* factory_create(clap_plugin_factory_t const*
       , clap_host_t const*, char const* id)
      {
         if (std::strcmp(id, descriptor().id) != 0)
            return nullptr;
         return base_plugin_impl::handle(*new plugin());
      }

      clap_plugin_factory_t const factory =
      {
         factory_count,
         factory_descriptor,
         factory_create
      };
   }
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
      return &cycfi::qplug::factory;
   return nullptr;
}
