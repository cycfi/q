/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CONTROLLER_HPP_SEPTEMBER_6_2026)
#define QPLUG_CONTROLLER_HPP_SEPTEMBER_6_2026

#include <qplug/parameter.hpp>
#include <qplug/data_stream.hpp>
#include <infra/iterator_range.hpp>
#include <memory>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Where GUI edits go. The plugin installs one; it reaches the host.
   ////////////////////////////////////////////////////////////////////////////
   struct edit_sink
   {
      virtual                 ~edit_sink() = default;
      virtual void            begin_edit(int id) = 0;
      virtual void            edit_parameter(int id, double value) = 0;
      virtual void            end_edit(int id) = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The controller
   ////////////////////////////////////////////////////////////////////////////
   class controller
   {
   public:

      using parameter_list = iterator_range<parameter const*>;

      virtual                 ~controller() = default;

      virtual parameter_list  parameters() const = 0;
      virtual double          get_parameter(int id) const = 0;

      // Called on the audio thread when the host changes a parameter.
      virtual void            set_parameter(int id, double value) = 0;

      // Called on the main thread afterwards; bring the models up to date.
      virtual void            update_models() {}

      // Edits made in the GUI, main thread. The presenter calls these.
      void                    begin_edit(int id);
      virtual void            edit_parameter(int id, double value);
      void                    end_edit(int id);
      void                    sink(edit_sink& s) { _sink = &s; }

      // Default: every parameter value, in order, as a double.
      virtual bool            save_state(ostream& out) const;
      virtual bool            load_state(istream& in);

   private:

      edit_sink*              _sink = nullptr;
   };

   using controller_ptr = std::unique_ptr<controller>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline void controller::begin_edit(int id)
   {
      if (_sink)
         _sink->begin_edit(id);
   }

   inline void controller::edit_parameter(int id, double value)
   {
      set_parameter(id, value);
      if (_sink)
         _sink->edit_parameter(id, value);
   }

   inline void controller::end_edit(int id)
   {
      if (_sink)
         _sink->end_edit(id);
   }

   inline bool controller::save_state(ostream& out) const
   {
      auto n = int(parameters().size());
      for (int id = 0; id != n; ++id)
      {
         double v = get_parameter(id);
         if (out.write(&v, sizeof(v)) != std::int64_t(sizeof(v)))
            return false;
      }
      return true;
   }

   inline bool controller::load_state(istream& in)
   {
      auto n = int(parameters().size());
      for (int id = 0; id != n; ++id)
      {
         double v;
         if (in.read(&v, sizeof(v)) != std::int64_t(sizeof(v)))
            return false;
         set_parameter(id, v);
      }
      update_models();
      return true;
   }
}

#endif
