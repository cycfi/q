/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CONTROLLER_HPP_SEPTEMBER_6_2026)
#define QPLUG_CONTROLLER_HPP_SEPTEMBER_6_2026

#include <qplug/parameter.hpp>
#include <qplug/data_stream.hpp>
#include <elements/model.hpp>
#include <infra/iterator_range.hpp>
#include <nlohmann/json_fwd.hpp>
#include <atomic>
#include <cstdint>
#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Where GUI edits go. The plugin installs one; it reaches the host.
   ////////////////////////////////////////////////////////////////////////////
   struct edit_sink
   {
      virtual                 ~edit_sink() = default;
      virtual void            begin_edit(int index) = 0;
      virtual void            edit_parameter(int index, double value) = 0;
      virtual void            end_edit(int index) = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The controller
   //
   // A controller declares its parameters and nothing else has to be
   // written: the base holds each one twice, an atomic the audio thread
   // reads and writes, and a model the GUI links to. Parameters are
   // addressed by their index in the list `parameters()` returns.
   ////////////////////////////////////////////////////////////////////////////
   class controller
   {
   public:

      using parameter = cycfi::qplug::parameter;
      using parameter_list = iterator_range<parameter const*>;
      using model_type = elements::value_model<double>;
      using json = nlohmann::json;

                              controller();
      virtual                 ~controller();

      virtual parameter_list  parameters() const = 0;

      // The value, and the model a control links to. Both main thread,
      // except get_parameter, which the audio thread also reads.
      virtual double          get_parameter(int index) const;
      model_type&             model(int index) { return _params[index].model; }

      // The same value as the type the plugin thinks in: a q::decibel, a
      // q::frequency, a note, a bool. A Debug build asserts that the type
      // matches the parameter's kind.
                              template <typename T>
      T                       get_parameter(int index) const;

      // Called on the audio thread when the host changes a parameter.
      virtual void            set_parameter(int index, double value);

                              template <typename T>
      void                    set_parameter(int index, T value);

      // Called on the main thread afterwards; brings the models up to date
      // and so the GUI with them.
      void                    update_models();

      // Edits made in the GUI, main thread. The presenter calls these.
      void                    begin_edit(int index);
      virtual void            edit_parameter(int index, double value);
      void                    end_edit(int index);

                              template <typename T>
      void                    edit_parameter(int index, T value);

      // The index of the parameter with this id or name, -1 if none.
      int                     index_of(parameter::id_type id) const;
      int                     index_of(std::string_view name) const;

      // The state: the parameters that ask to be saved, each by id and
      // name, with the plugin's id and state version, as JSON. What the
      // host keeps in a session and what a preset holds are the same
      // thing. See controller.cpp for the layout and the rules.
      json                    state() const;
      bool                    state(json const& j);
      bool                    save_state(ostream& out) const;
      bool                    load_state(istream& in);

      // Presets: named states. Factory presets come with the plugin, in
      // factory_presets.json among its resources; user presets live in a
      // file of the user's own, per plugin. A user preset may shadow a
      // factory one by name; only user presets can be saved or deleted.
      std::vector<std::string> preset_names() const;
      bool                    has_preset(std::string_view name) const;
      bool                    is_factory_preset(std::string_view name) const;
      bool                    load_preset(std::string_view name);
      bool                    save_preset(std::string_view name);
      bool                    delete_preset(std::string_view name);

   protected:

      // A plugin with more than its parameters to keep adds it to the
      // state here, and takes it back with the version the state was
      // written with, so it can read an older layout of its own.
      virtual void            save_extra(json& j) const {}
      virtual void            load_extra(json const& j
                               , std::uint32_t version) {}

   private:

      friend class plugin;

      // Called once by the plugin, on the main thread, before anything
      // else runs. parameters() is virtual, so the constructor cannot.
      void                    init(edit_sink& s);

      struct entry
      {
         std::atomic<double>  value;
         model_type           model;
      };

      struct presets;
      presets&                get_presets() const;

      std::unique_ptr<entry[]> _params;
      int                     _size = 0;
      edit_sink*              _sink = nullptr;
      mutable std::unique_ptr<presets> _presets;
   };

   using controller_ptr = std::unique_ptr<controller>;
      // Tell the host every value the plugin just moved on its own.
      void                    send_edits();


   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline void controller::init(edit_sink& s)
   {
      _sink = &s;
      auto params = parameters();
      _size = int(params.size());
      _params = std::make_unique<entry[]>(_size);
      for (int i = 0; i != _size; ++i)
      {
         _params[i].value.store(params[i]._init, std::memory_order_relaxed);
         _params[i].model = params[i]._init;
      }
   }

   inline double controller::get_parameter(int index) const
   {
      return _params[index].value.load(std::memory_order_relaxed);
   }

   inline void controller::set_parameter(int index, double value)
   {
      _params[index].value.store(value, std::memory_order_relaxed);
   }

   template <typename T>
   inline T controller::get_parameter(int index) const
   {
      using traits = parameter_traits<T>;
      assert(traits::matches(parameters()[index]._type));
      return traits::get(get_parameter(index));
   }

   template <typename T>
   inline void controller::set_parameter(int index, T value)
   {
      using traits = parameter_traits<T>;
      assert(traits::matches(parameters()[index]._type));
      set_parameter(index, traits::set(value));
   }

   template <typename T>
   inline void controller::edit_parameter(int index, T value)
   {
      using traits = parameter_traits<T>;
      assert(traits::matches(parameters()[index]._type));
      edit_parameter(index, traits::set(value));
   }

   // A model assignment always notifies, so assign only what changed:
   // otherwise every host callback redraws the GUI, and a value the host
   // echoes back mid-gesture fights the control the user is dragging.
   inline void controller::update_models()
   {
      for (int i = 0; i != _size; ++i)
      {
         auto value = get_parameter(i);
         if (_params[i].model.get() != value)
            _params[i].model = value;
      }
   }

   inline void controller::begin_edit(int index)
   {
      if (_sink)
         _sink->begin_edit(index);
   }

   inline void controller::edit_parameter(int index, double value)
   {
      set_parameter(index, value);
      _params[index].model = value;
      if (_sink)
         _sink->edit_parameter(index, value);
   }

   inline void controller::end_edit(int index)
   {
      if (_sink)
         _sink->end_edit(index);
   }
}

#endif
