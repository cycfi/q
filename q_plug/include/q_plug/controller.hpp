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
   // The controller
   ////////////////////////////////////////////////////////////////////////////
   class controller
   {
   public:

      using parameter_list = iterator_range<parameter const*>;

      virtual                 ~controller() = default;

      virtual parameter_list  parameters() const = 0;
      virtual double          get_parameter(int id) const = 0;
      virtual void            set_parameter(int id, double value) = 0;

      // Default: every parameter value, in order, as a double.
      virtual bool            save_state(ostream& out) const;
      virtual bool            load_state(istream& in);
   };

   using controller_ptr = std::unique_ptr<controller>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
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
      return true;
   }
}

#endif
