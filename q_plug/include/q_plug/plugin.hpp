/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PLUGIN_HPP_SEPTEMBER_6_2026)
#define QPLUG_PLUGIN_HPP_SEPTEMBER_6_2026

#include <qplug/base_plugin.hpp>
#include <qplug/processor.hpp>
#include <qplug/controller.hpp>
#include <qplug/presenter.hpp>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Client supplied. The controller is made first; it is the hub the other
   // two are given. Neither the processor nor the presenter sees the other.
   ////////////////////////////////////////////////////////////////////////////
   controller_ptr    make_controller();
   processor_ptr     make_processor(controller& ctl);
   presenter_ptr     make_presenter(controller& ctl);
   plugin_info const& info();

   ////////////////////////////////////////////////////////////////////////////
   // The plugin
   //
   // The one concrete base_plugin. It composes a processor, a controller and
   // a presenter, none of which is ever null, and forwards each host callback
   // to the one that owns it.
   ////////////////////////////////////////////////////////////////////////////
   class plugin : public base_plugin
   {
   public:
                              plugin();

   protected:

      bool                    activate(std::uint32_t sps
                               , std::uint32_t min_frames
                               , std::uint32_t max_frames) override;
      void                    deactivate() override;
      void                    reset() override;
      void                    process(in_channels const& in
                               , out_channels const& out) override;

      std::uint32_t           inputs() const override;
      std::uint32_t           outputs() const override;

      parameter_list          parameters() const override;
      double                  get_parameter(int id) const override;
      void                    set_parameter(int id, double value) override;

      bool                    save_state(ostream& out) const override;
      bool                    load_state(istream& in) override;

   private:

      controller_ptr          _controller;
      processor_ptr           _processor;
      presenter_ptr           _presenter;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline plugin::plugin()
    : _controller(make_controller())
    , _processor(make_processor(*_controller))
    , _presenter(make_presenter(*_controller))
   {}

   inline bool plugin::activate(std::uint32_t sps
    , std::uint32_t, std::uint32_t max_frames)
   {
      _processor->activate(sps, max_frames);
      return true;
   }

   inline void plugin::deactivate()
   {
      _processor->deactivate();
   }

   inline void plugin::reset()
   {
      _processor->reset();
   }

   inline void plugin::process(in_channels const& in, out_channels const& out)
   {
      _processor->process(in, out);
   }

   inline std::uint32_t plugin::inputs() const
   {
      return _processor->inputs();
   }

   inline std::uint32_t plugin::outputs() const
   {
      return _processor->outputs();
   }

   inline plugin::parameter_list plugin::parameters() const
   {
      return _controller->parameters();
   }

   inline double plugin::get_parameter(int id) const
   {
      return _controller->get_parameter(id);
   }

   inline void plugin::set_parameter(int id, double value)
   {
      _controller->set_parameter(id, value);
   }

   inline bool plugin::save_state(ostream& out) const
   {
      return _controller->save_state(out);
   }

   inline bool plugin::load_state(istream& in)
   {
      return _controller->load_state(in);
   }
}

#endif
