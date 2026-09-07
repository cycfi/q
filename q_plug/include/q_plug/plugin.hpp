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
   // a presenter, and forwards each host callback to the one that owns it.
   // A headless plugin returns nullptr from make_presenter: no view, and
   // the host is told so.
   ////////////////////////////////////////////////////////////////////////////
   class plugin : public base_plugin, edit_sink, view_sink
   {
   public:
                              plugin();

   protected:

      bool                    activate(std::uint32_t sps
                               , std::uint32_t min_frames
                               , std::uint32_t max_frames) override;
      void                    deactivate() override;
      void                    on_main_thread() override;
      void                    reset() override;
      void                    process(in_channels const& in
                               , out_channels const& out) override;

      channel_config          channels() const override;

      parameter_list          parameters() const override;
      double                  get_parameter(int id) const override;
      void                    set_parameter(int id, double value) override;

      bool                    save_state(ostream& out) const override;
      bool                    load_state(istream& in) override;

      bool                    has_view() const override;
      bool                    create_view() override;
      elements::extent        view_size() const override;
      elements::view_limits   view_limits() const override;
      bool                    resize_view(elements::extent size) override;
      bool                    attach_view(void* parent) override;
      void                    detach_view() override;
      void                    show_view(bool show) override;
      bool                    scale_view(double scale) override;

      // edit_sink: the controller's GUI edits, on to the host
      void                    begin_edit(int id) override;
      void                    edit_parameter(int id, double value) override;
      void                    end_edit(int id) override;

      // view_sink: the presenter's requests, on to the host
      bool                    request_resize(elements::extent size) override;

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
   {
      _controller->init(static_cast<edit_sink&>(*this));
      if (_presenter)
         _presenter->sink(static_cast<view_sink&>(*this));
   }

   inline bool plugin::activate(std::uint32_t sps
    , std::uint32_t, std::uint32_t max_frames)
   {
      _processor->_sps = sps;
      _processor->_max_frames = max_frames;
      _processor->activate();
      _processor->reset();
      return true;
   }

   inline void plugin::deactivate()
   {
      _processor->deactivate();
   }

   inline void plugin::on_main_thread()
   {
      _controller->update_models();
   }

   inline void plugin::reset()
   {
      _processor->reset();
   }

   inline void plugin::process(in_channels const& in, out_channels const& out)
   {
      _processor->process(in, out);
   }

   inline channel_config plugin::channels() const
   {
      return _processor->channels();
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

   inline bool plugin::has_view() const
   {
      return bool(_presenter);
   }

   inline bool plugin::create_view()
   {
      return _presenter && _presenter->create(info().view_size);
   }

   // The live view's size once it exists, the opening size before that.
   inline elements::extent plugin::view_size() const
   {
      auto s = _presenter ? _presenter->size() : elements::extent{0, 0};
      return (s.x > 0 && s.y > 0) ? s : info().view_size;
   }

   inline elements::view_limits plugin::view_limits() const
   {
      return _presenter ? _presenter->limits() : elements::view_limits{};
   }

   inline bool plugin::resize_view(elements::extent size)
   {
      return _presenter && _presenter->resize(size);
   }

   inline bool plugin::attach_view(void* parent)
   {
      return _presenter && _presenter->attach(parent, info().view_size);
   }

   inline void plugin::detach_view()
   {
      if (_presenter)
         _presenter->detach();
   }

   inline void plugin::show_view(bool show)
   {
      if (_presenter)
         _presenter->show(show);
   }

   inline bool plugin::scale_view(double scale)
   {
      return scale == 1.0;
   }

   inline void plugin::begin_edit(int id)
   {
      base_plugin::begin_edit(id);
   }

   inline void plugin::edit_parameter(int id, double value)
   {
      base_plugin::edit_parameter(id, value);
   }

   inline void plugin::end_edit(int id)
   {
      base_plugin::end_edit(id);
   }

   inline bool plugin::request_resize(elements::extent size)
   {
      return request_view_resize(size);
   }
}

#endif
