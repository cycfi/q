/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/plugin.hpp>
#include <qplug/clap/midi_events.hpp>
#include <qplug/log.hpp>
#include <clap/clap.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <mutex>
#include <vector>

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
      // The one windowing API we embed into, per platform.
      constexpr char const* native_window_api =
#if defined(_WIN32)
         CLAP_WINDOW_API_WIN32;
#else
         CLAP_WINDOW_API_COCOA;
#endif

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
      // A GUI edit waiting to be sent to the host in process() or flush().
      struct edit
      {
         enum kind_t { begin, value, end };
         kind_t               kind;
         clap_id              id;
         double               val;
      };

                              base_plugin_impl(base_plugin& self
                               , clap_plugin_descriptor_t const* desc);

      static base_plugin&     self(clap_plugin_t const* p);
      static base_plugin_impl& impl(base_plugin& p) { return *p._impl; }
      static clap_plugin_t const* handle(base_plugin& p);
      static void             set_host(base_plugin& p
                               , clap_host_t const* host);

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

      static void             port_info(channel_config config, bool is_input
                               , clap_audio_port_info_t* info);

      // clap.params. Hosts speak parameter ids; the plugin's list is
      // indexed. find() maps an id to its index, or -1.
      static int              find(base_plugin& p, clap_id id);
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

      // clap.note-ports
      static uint32_t         note_ports_count(clap_plugin_t const* p
                               , bool is_input);
      static bool             note_ports_get(clap_plugin_t const* p
                               , uint32_t index, bool is_input
                               , clap_note_port_info_t* info);
      void                    push_edit(edit const& e);
      void                    send_edits(clap_output_events_t const* out);

      // clap.state
      static bool             state_save(clap_plugin_t const* p
                               , clap_ostream_t const* stream);
      static bool             state_load(clap_plugin_t const* p
                               , clap_istream_t const* stream);

      // clap.gui
      static bool             gui_is_api_supported(clap_plugin_t const* p
                               , char const* api, bool is_floating);
      static bool             gui_get_preferred_api(clap_plugin_t const* p
                               , char const** api, bool* is_floating);
      static bool             gui_create(clap_plugin_t const* p
                               , char const* api, bool is_floating);
      static void             gui_destroy(clap_plugin_t const* p);
      static bool             gui_set_scale(clap_plugin_t const* p
                               , double scale);
      static bool             gui_get_size(clap_plugin_t const* p
                               , uint32_t* width, uint32_t* height);
      static bool             gui_can_resize(clap_plugin_t const* p);
      static bool             gui_get_resize_hints(clap_plugin_t const* p
                               , clap_gui_resize_hints_t* hints);
      static bool             gui_adjust_size(clap_plugin_t const* p
                               , uint32_t* width, uint32_t* height);
      static bool             gui_set_size(clap_plugin_t const* p
                               , uint32_t width, uint32_t height);
      static bool             gui_set_parent(clap_plugin_t const* p
                               , clap_window_t const* window);
      static bool             gui_set_transient(clap_plugin_t const* p
                               , clap_window_t const* window);
      static void             gui_suggest_title(clap_plugin_t const* p
                               , char const* title);
      static bool             gui_show(clap_plugin_t const* p);
      static bool             gui_hide(clap_plugin_t const* p);

      static clap_plugin_audio_ports_t const   s_audio_ports;
      static clap_plugin_note_ports_t const     s_note_ports;
      static clap_plugin_params_t const        s_params;
      static clap_plugin_state_t const         s_state;
      static clap_plugin_gui_t const           s_gui;

      clap_plugin_t           _plugin;
      clap_host_t const*      _host = nullptr;
      clap_host_params_t const* _host_params = nullptr;
      clap_host_gui_t const*  _host_gui = nullptr;

      // Edits go main thread to audio thread. The audio thread only
      // try_locks, so it never blocks; edits it cannot take now go next time.
      std::mutex              _edits_mutex;
      std::vector<edit>       _edits;
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

   // The plugin side works in list indices; the host gets parameter ids.
   void base_plugin::begin_edit(int index)
   {
      auto id = parameters()[index]._id;
      QPLUG_LOG(input, "begin edit {}", id);
      _impl->push_edit({base_plugin_impl::edit::begin, id, 0.0});
   }

   void base_plugin::edit_parameter(int index, double value)
   {
      auto id = parameters()[index]._id;
      QPLUG_LOG(input, "edit {} = {}", id, value);
      _impl->push_edit({base_plugin_impl::edit::value, id, value});
   }

   void base_plugin::end_edit(int index)
   {
      auto id = parameters()[index]._id;
      QPLUG_LOG(input, "end edit {}", id);
      _impl->push_edit({base_plugin_impl::edit::end, id, 0.0});
   }

   bool base_plugin::request_view_resize(elements::extent size)
   {
      auto width = uint32_t(size.x);
      auto height = uint32_t(size.y);
      return _impl->_host_gui
         && _impl->_host_gui->request_resize(_impl->_host, width, height);
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

   void base_plugin_impl::set_host(base_plugin& p, clap_host_t const* host)
   {
      p._impl->_host = host;
   }

   bool base_plugin_impl::init(clap_plugin_t const* p)
   {
      auto& plug = self(p);
      auto& im = impl(plug);
      log_init(p->desc->name);
      QPLUG_LOG(app, "init: host {} {} ({})"
       , im._host ? im._host->name : "none"
       , im._host ? im._host->version : ""
       , im._host ? im._host->vendor : "");
      if (im._host)
      {
         im._host_params = static_cast<clap_host_params_t const*>(
            im._host->get_extension(im._host, CLAP_EXT_PARAMS));
         im._host_gui = static_cast<clap_host_gui_t const*>(
            im._host->get_extension(im._host, CLAP_EXT_GUI));
      }
      auto ok = plug.init();
      QPLUG_LOG(app, "init: {}", ok ? "ok" : "failed");
      return ok;
   }

   void base_plugin_impl::destroy(clap_plugin_t const* p)
   {
      QPLUG_LOG(app, "destroy");
      delete &self(p);
   }

   bool base_plugin_impl::activate(clap_plugin_t const* p, double sps
    , uint32_t min_frames, uint32_t max_frames)
   {
      auto& plug = self(p);
      auto ch = plug.channels();
      QPLUG_LOG(app, "activate: {} Hz, {} to {} frames, {} in {} out"
       , sps, min_frames, max_frames, ch.inputs, ch.outputs);
      return plug.activate(uint32_t(sps), min_frames, max_frames);
   }

   void base_plugin_impl::deactivate(clap_plugin_t const* p)
   {
      QPLUG_LOG(app, "deactivate");
      self(p).deactivate();
   }

   bool base_plugin_impl::start_processing(clap_plugin_t const* p)
   {
      QPLUG_LOG(app, "start_processing");
      return self(p).start_processing();
   }

   void base_plugin_impl::stop_processing(clap_plugin_t const* p)
   {
      QPLUG_LOG(app, "stop_processing");
      self(p).stop_processing();
   }

   void base_plugin_impl::reset(clap_plugin_t const* p)
   {
      QPLUG_LOG(app, "reset");
      self(p).reset();
   }

   clap_process_status base_plugin_impl::process(clap_plugin_t const* p
    , clap_process_t const* proc)
   {
      auto& plug = self(p);
      if (proc->in_events)
         apply_events(plug, proc->in_events);
      if (proc->out_events)
         impl(plug).send_edits(proc->out_events);

      auto const& ao = proc->audio_outputs[0];
      auto frames = proc->frames_count;

      // An instrument has no audio input port, so there is no buffer to
      // point at: it gets an empty range rather than a null one.
      float const** in_data = nullptr;
      std::uint32_t in_count = 0;
      if (proc->audio_inputs_count != 0 && proc->audio_inputs)
      {
         in_data = const_cast<float const**>(proc->audio_inputs[0].data32);
         in_count = proc->audio_inputs[0].channel_count;
      }

      base_plugin::in_channels in(in_data, in_count, frames);
      base_plugin::out_channels out(ao.data32, ao.channel_count, frames);

      plug.process(in, out);
      return CLAP_PROCESS_CONTINUE;
   }

   void const*
   base_plugin_impl::extension(clap_plugin_t const* p, char const* id)
   {
      if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
         return &s_audio_ports;
      if (!std::strcmp(id, CLAP_EXT_NOTE_PORTS) && self(p).has_midi_input())
         return &s_note_ports;
      if (!std::strcmp(id, CLAP_EXT_PARAMS))
         return &s_params;
      if (!std::strcmp(id, CLAP_EXT_STATE))
         return &s_state;
      if (!std::strcmp(id, CLAP_EXT_GUI) && self(p).has_view())
         return &s_gui;
      return nullptr;
   }

   void base_plugin_impl::on_main_thread(clap_plugin_t const* p)
   {
      self(p).on_main_thread();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.audio-ports
   ////////////////////////////////////////////////////////////////////////////
   // An instrument declares no input channels, and a port of no channels
   // is not a port: it reports none on that side.
   uint32_t base_plugin_impl::ports_count(clap_plugin_t const* p
    , bool is_input)
   {
      auto config = self(p).channels();
      auto channels = is_input? config.inputs : config.outputs;
      return channels == 0? 0 : 1;
   }

   namespace
   {
      // Anything but mono or stereo is "arbitrary audio" to CLAP: one
      // channel per string, for instance, with no spatial meaning.
      char const* port_type(std::uint32_t channels)
      {
         if (channels == 1)
            return CLAP_PORT_MONO;
         if (channels == 2)
            return CLAP_PORT_STEREO;
         return nullptr;
      }
   }

   void base_plugin_impl::port_info(channel_config config, bool is_input
    , clap_audio_port_info_t* info)
   {
      auto channels = is_input ? config.inputs : config.outputs;
      info->id = 0;
      info->channel_count = channels;
      info->flags = CLAP_AUDIO_PORT_IS_MAIN;
      info->in_place_pair = CLAP_INVALID_ID;
      info->port_type = port_type(channels);
      std::snprintf(info->name, sizeof(info->name), "%s"
       , is_input ? "Input" : "Output");
   }

   bool base_plugin_impl::ports_get(clap_plugin_t const* p, uint32_t index
    , bool is_input, clap_audio_port_info_t* info)
   {
      if (index != 0)
         return false;
      port_info(self(p).channels(), is_input, info);
      return true;
   }

   clap_plugin_audio_ports_t const base_plugin_impl::s_audio_ports =
   {
      ports_count,
      ports_get
   };

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.note-ports
   //
   // One input port, no output. Every dialect is accepted, since each ends
   // in the same Q message, and MIDI 2.0 is preferred because it is the one
   // that loses nothing: 16 bit velocity, 32 bit controllers, and per note
   // messages a host would otherwise spend a channel to approximate. MPE is
   // not claimed here; a plugin that answers it says so when it wires Q's
   // mpe_reader.
   ////////////////////////////////////////////////////////////////////////////
   uint32_t base_plugin_impl::note_ports_count(clap_plugin_t const*
    , bool is_input)
   {
      return is_input? 1 : 0;
   }

   bool base_plugin_impl::note_ports_get(clap_plugin_t const*, uint32_t index
    , bool is_input, clap_note_port_info_t* info)
   {
      if (index != 0 || !is_input)
         return false;

      info->id = 0;
      info->supported_dialects =
         CLAP_NOTE_DIALECT_MIDI
       | CLAP_NOTE_DIALECT_MIDI2
       | CLAP_NOTE_DIALECT_CLAP;
      info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI2;
      std::snprintf(info->name, sizeof(info->name), "%s", "Notes");
      return true;
   }

   clap_plugin_note_ports_t const base_plugin_impl::s_note_ports =
   {
      note_ports_count,
      note_ports_get
   };

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.params
   ////////////////////////////////////////////////////////////////////////////
   int base_plugin_impl::find(base_plugin& p, clap_id id)
   {
      auto params = p.parameters();
      for (std::size_t i = 0; i != params.size(); ++i)
         if (params[i]._id == id)
            return int(i);
      return -1;
   }

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
      info->id = param._id;
      info->flags = 0;
      if (param._can_automate)
         info->flags |= CLAP_PARAM_IS_AUTOMATABLE;
      if (param.stepped())
         info->flags |= CLAP_PARAM_IS_STEPPED;
      if (param._type == parameter::enum_)
         info->flags |= CLAP_PARAM_IS_ENUM;
      if (param._hidden)
         info->flags |= CLAP_PARAM_IS_HIDDEN;
      if (param._bypass)
         info->flags |= CLAP_PARAM_IS_BYPASS;
      if (param._periodic)
         info->flags |= CLAP_PARAM_IS_PERIODIC;
      info->min_value = param._min;
      info->max_value = param._max;
      info->default_value = param._init;
      info->cookie = nullptr;
      std::snprintf(info->name, sizeof(info->name), "%s", param._name);
      std::snprintf(info->module, sizeof(info->module), "%s", param._module);
      return true;
   }

   bool base_plugin_impl::params_value(clap_plugin_t const* p, clap_id id
    , double* value)
   {
      auto& plug = self(p);
      auto index = find(plug, id);
      if (index < 0)
         return false;
      *value = plug.get_parameter(index);
      return true;
   }

   bool base_plugin_impl::params_to_text(clap_plugin_t const* p, clap_id id
    , double value, char* text, uint32_t size)
   {
      auto& plug = self(p);
      auto index = find(plug, id);
      if (index < 0)
         return false;
      return plug.parameters()[index].to_text(value, text, size);
   }

   bool base_plugin_impl::params_from_text(clap_plugin_t const* p, clap_id id
    , char const* text, double* value)
   {
      auto& plug = self(p);
      auto index = find(plug, id);
      if (index < 0)
         return false;
      return plug.parameters()[index].from_text(text, *value);
   }

   void base_plugin_impl::params_flush(clap_plugin_t const* p
    , clap_input_events_t const* in, clap_output_events_t const* out)
   {
      auto& plug = self(p);
      apply_events(plug, in);
      if (out)
         impl(plug).send_edits(out);
   }

   void base_plugin_impl::apply_events(base_plugin& plug
    , clap_input_events_t const* in)
   {
      auto n = in->size(in);
      bool changed = false;

      for (uint32_t i = 0; i != n; ++i)
      {
         auto hdr = in->get(in, i);
         if (hdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
            continue;

         // The host delivers these in sample order, and the offset into
         // the block is what a message carries as its time.
         auto const time = std::size_t(hdr->time);

         switch (hdr->type)
         {
            case CLAP_EVENT_PARAM_VALUE:
            {
               auto ev =
                  reinterpret_cast<clap_event_param_value_t const*>(hdr);
               auto index = find(plug, ev->param_id);
               if (index >= 0)
               {
                  plug.set_parameter(index, ev->value);
                  changed = true;
               }
               break;
            }

            case CLAP_EVENT_MIDI:
            {
               auto ev = reinterpret_cast<clap_event_midi_t const*>(hdr);
               plug.midi(to_raw_message(*ev), time);
               break;
            }

            case CLAP_EVENT_MIDI2:
            {
               auto ev = reinterpret_cast<clap_event_midi2_t const*>(hdr);
               plug.midi(to_packet(*ev), time);
               break;
            }

            case CLAP_EVENT_NOTE_ON:
            case CLAP_EVENT_NOTE_OFF:
            case CLAP_EVENT_NOTE_CHOKE:
            {
               auto ev = reinterpret_cast<clap_event_note_t const*>(hdr);
               q::midi_1_0::raw_message msg;
               if (to_raw_message(*ev, msg))
                  plug.midi(msg, time);
               break;
            }
         }
      }

      // The models are updated on the main thread; ask the host for it.
      auto& im = impl(plug);
      if (changed && im._host)
         im._host->request_callback(im._host);
   }

   void base_plugin_impl::push_edit(edit const& e)
   {
      {
         std::lock_guard<std::mutex> lock(_edits_mutex);
         _edits.push_back(e);
      }
      if (_host_params)
         _host_params->request_flush(_host);
   }

   void base_plugin_impl::send_edits(clap_output_events_t const* out)
   {
      std::unique_lock<std::mutex> lock(_edits_mutex, std::try_to_lock);
      if (!lock.owns_lock())
         return;

      for (auto const& e : _edits)
      {
         if (e.kind == edit::value)
         {
            clap_event_param_value_t ev{};
            ev.header.size = sizeof(ev);
            ev.header.time = 0;
            ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            ev.header.type = CLAP_EVENT_PARAM_VALUE;
            ev.header.flags = 0;
            ev.param_id = e.id;
            ev.cookie = nullptr;
            ev.note_id = -1;
            ev.port_index = -1;
            ev.channel = -1;
            ev.key = -1;
            ev.value = e.val;
            out->try_push(out, &ev.header);
         }
         else
         {
            clap_event_param_gesture_t ev{};
            ev.header.size = sizeof(ev);
            ev.header.time = 0;
            ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            ev.header.type = e.kind == edit::begin
               ? CLAP_EVENT_PARAM_GESTURE_BEGIN
               : CLAP_EVENT_PARAM_GESTURE_END;
            ev.header.flags = 0;
            ev.param_id = e.id;
            out->try_push(out, &ev.header);
         }
      }
      _edits.clear();
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
      auto ok = self(p).save_state(out);
      QPLUG_LOG(app, "save state: {}", ok ? "ok" : "failed");
      return ok;
   }

   bool base_plugin_impl::state_load(clap_plugin_t const* p
    , clap_istream_t const* stream)
   {
      clap_istream_adapter in(stream);
      auto ok = self(p).load_state(in);
      QPLUG_LOG(app, "load state: {}", ok ? "ok" : "failed");
      return ok;
   }

   clap_plugin_state_t const base_plugin_impl::s_state =
   {
      state_save,
      state_load
   };

   ////////////////////////////////////////////////////////////////////////////
   // Impl: clap.gui. Embedded Cocoa only, fixed size.
   ////////////////////////////////////////////////////////////////////////////
   bool base_plugin_impl::gui_is_api_supported(clap_plugin_t const*
    , char const* api, bool is_floating)
   {
      return !is_floating && !std::strcmp(api, native_window_api);
   }

   bool base_plugin_impl::gui_get_preferred_api(clap_plugin_t const*
    , char const** api, bool* is_floating)
   {
      *api = native_window_api;
      *is_floating = false;
      return true;
   }

   bool base_plugin_impl::gui_create(clap_plugin_t const* p
    , char const* api, bool is_floating)
   {
      auto ok = gui_is_api_supported(p, api, is_floating)
         && self(p).create_view();
      QPLUG_LOG(window, "gui create: api {}, floating {}: {}"
       , api, is_floating, ok ? "ok" : "refused");
      return ok;
   }

   void base_plugin_impl::gui_destroy(clap_plugin_t const* p)
   {
      QPLUG_LOG(window, "gui destroy");
      self(p).detach_view();
   }

   bool base_plugin_impl::gui_set_scale(clap_plugin_t const* p, double scale)
   {
      auto ok = self(p).scale_view(scale);
      QPLUG_LOG(window, "gui set scale {}: {}", scale, ok ? "ok" : "refused");
      return ok;
   }

   bool base_plugin_impl::gui_get_size(clap_plugin_t const* p
    , uint32_t* width, uint32_t* height)
   {
      auto size = self(p).view_size();
      *width = uint32_t(size.x);
      *height = uint32_t(size.y);
      QPLUG_LOG(window, "gui get size: {}x{}", *width, *height);
      return true;
   }

   // Resizing: the content's limits decide. A view whose minimum and
   // maximum differ is resizable, within them. Elements says "no limit"
   // with a huge float; clamp that to what a host's integers can hold.
   namespace
   {
      struct integer_limits
      {
         uint32_t min_w, min_h, max_w, max_h;
      };

      integer_limits to_integers(elements::view_limits l)
      {
         constexpr float largest = 1 << 15;
         return {
            uint32_t(std::min(l.min.x, largest))
          , uint32_t(std::min(l.min.y, largest))
          , uint32_t(std::min(l.max.x, largest))
          , uint32_t(std::min(l.max.y, largest))
         };
      }
   }

   bool base_plugin_impl::gui_can_resize(clap_plugin_t const* p)
   {
      auto l = to_integers(self(p).view_limits());
      auto ok = l.min_w != l.max_w || l.min_h != l.max_h;
      QPLUG_LOG(window, "gui can resize: {} ({}x{} to {}x{})"
       , ok, l.min_w, l.min_h, l.max_w, l.max_h);
      return ok;
   }

   bool base_plugin_impl::gui_get_resize_hints(clap_plugin_t const* p
    , clap_gui_resize_hints_t* hints)
   {
      auto l = to_integers(self(p).view_limits());
      hints->can_resize_horizontally = l.min_w != l.max_w;
      hints->can_resize_vertically = l.min_h != l.max_h;
      hints->preserve_aspect_ratio = false;
      hints->aspect_ratio_width = 1;
      hints->aspect_ratio_height = 1;
      return true;
   }

   bool base_plugin_impl::gui_adjust_size(clap_plugin_t const* p
    , uint32_t* width, uint32_t* height)
   {
      auto l = to_integers(self(p).view_limits());
      auto w = *width, h = *height;
      *width = std::clamp(*width, l.min_w, l.max_w);
      *height = std::clamp(*height, l.min_h, l.max_h);
      QPLUG_LOG(window, "gui adjust size {}x{} to {}x{}"
       , w, h, *width, *height);
      return true;
   }

   bool base_plugin_impl::gui_set_size(clap_plugin_t const* p
    , uint32_t width, uint32_t height)
   {
      auto ok = self(p).resize_view({float(width), float(height)});
      QPLUG_LOG(window, "gui set size {}x{}: {}"
       , width, height, ok ? "ok" : "refused");
      return ok;
   }

   bool base_plugin_impl::gui_set_parent(clap_plugin_t const* p
    , clap_window_t const* window)
   {
      if (std::strcmp(window->api, native_window_api) != 0)
      {
         QPLUG_LOG(window, "gui set parent: api {} refused", window->api);
         return false;
      }
      auto ok = self(p).attach_view(window->ptr);
      QPLUG_LOG(window, "gui set parent {}: {}"
       , window->ptr, ok ? "attached" : "failed");
      return ok;
   }

   bool base_plugin_impl::gui_set_transient(clap_plugin_t const*
    , clap_window_t const*)
   {
      return false;
   }

   void base_plugin_impl::gui_suggest_title(clap_plugin_t const*, char const*)
   {}

   bool base_plugin_impl::gui_show(clap_plugin_t const* p)
   {
      QPLUG_LOG(window, "gui show");
      self(p).show_view(true);
      return true;
   }

   bool base_plugin_impl::gui_hide(clap_plugin_t const* p)
   {
      QPLUG_LOG(window, "gui hide");
      self(p).show_view(false);
      return true;
   }

   clap_plugin_gui_t const base_plugin_impl::s_gui =
   {
      gui_is_api_supported,
      gui_get_preferred_api,
      gui_create,
      gui_destroy,
      gui_set_scale,
      gui_get_size,
      gui_can_resize,
      gui_get_resize_hints,
      gui_adjust_size,
      gui_set_size,
      gui_set_parent,
      gui_set_transient,
      gui_suggest_title,
      gui_show,
      gui_hide
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
       , clap_host_t const* host, char const* id)
      {
         if (std::strcmp(id, descriptor().id) != 0)
            return nullptr;
         auto* p = new plugin();
         base_plugin_impl::set_host(*p, host);
         return base_plugin_impl::handle(*p);
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
