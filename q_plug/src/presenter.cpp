/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/presenter.hpp>
#include <qplug/log.hpp>
#include <qplug/host_view.hpp>
#include <algorithm>
#include <chrono>

namespace cycfi::qplug
{
   namespace
   {
      // Opening an editor should feel instant. These say where the time
      // goes when it does not.
      struct stopwatch
      {
         double ms() const
         {
            // Qualified: cycfi::q::duration is a type name in scope here.
            return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - _start).count();
         }

         std::chrono::steady_clock::time_point _start =
            std::chrono::steady_clock::now();
      };

      void destroy_view(detail::plugin_view* v)
      {
         delete v;
      }
   }

   presenter::presenter(controller& ctl)
    : _ctl(ctl)
    , _view(nullptr, &destroy_view)
   {}

   presenter::~presenter()
   {
      detach();
   }

   bool presenter::create(elements::extent size_)
   {
      if (_view || !detail::unparented_view_ok)
         return true;
      return build(nullptr, size_);
   }

   bool presenter::build(void* parent, elements::extent size_)
   {
      stopwatch total;

      // The first view in the process also registers the fonts.
      stopwatch making;
      _view.reset(detail::make_view(parent, size_));
      QPLUG_LOG(window, "view made in {:.1f} ms", making.ms());

      // One dispatcher for every bound control's gestures: the view has a
      // single on_tracking, so each binding cannot have its own.
      _view->on_tracking =
         [this](elements::element& e, elements::element::tracking state)
         {
            for (auto const& g : _gestures)
            {
               auto el = g.element.lock();
               if (el.get() != &e)
                  continue;
               if (state == elements::element::begin_tracking)
                  _ctl.begin_edit(g.index);
               else if (state == elements::element::end_tracking)
                  _ctl.end_edit(g.index);
            }
         };

      // Zoom, the same in every plugin: the action key, Command on macOS
      // and Control elsewhere, with plus or minus, a tenth at a time.
      // Scaling the content changes its limits, and the view asks the
      // host for a window to match on its next draw.
      _view->on_key =
         [this](elements::key_info const& k)
         {
            using elements::key_code;
            using elements::key_action;

            if (k.action == key_action::release
             || !(k.modifiers & elements::mod_action))
               return false;

            if (k.key == key_code::equal || k.key == key_code::kp_add)
               return zoom(zoom() + zoom_step);
            if (k.key == key_code::minus || k.key == key_code::kp_subtract)
               return zoom(zoom() - zoom_step);
            return false;
         };

      stopwatch content;
      on_attach(*_view);
      QPLUG_LOG(window, "content built in {:.1f} ms", content.ms());
      _view->scale(zoom());

      // When the content's limits change, keep the host's window inside
      // them. The host answers with set_size.
      _view->on_change_limits =
         [this](elements::view_limits l)
         {
            auto s = _view->size();
            auto w = std::clamp(s.x, l.min.x, l.max.x);
            auto h = std::clamp(s.y, l.min.y, l.max.y);
            if (w != s.x || h != s.y)
               request_resize({w, h});
         };

      stopwatch sizing;
      resize(size_);
      QPLUG_LOG(window, "sized in {:.1f} ms, create: {:.1f} ms"
       , sizing.ms(), total.ms());
      return true;
   }

   bool presenter::attach(void* parent, elements::extent size_)
   {
      if (!parent)
         return false;
      if (!_view && !build(parent, size_))
         return false;

      stopwatch attaching;
      detail::add_subview(parent, _view->host());
      _view->refresh();
      QPLUG_LOG(window, "attach: {:.1f} ms", attaching.ms());
      return true;
   }

   void presenter::detach()
   {
      if (_view)
      {
         on_detach();
         _gestures.clear();
         _proxies.clear();
         _view.reset();
      }
   }

   void presenter::show(bool show)
   {
      if (_view && show)
         _view->refresh();
   }

   bool presenter::zoom(float scale_)
   {
      scale_ = std::clamp(scale_, zoom_min, zoom_max);
      if (scale_ == zoom())
         return true;
      _ctl.view_scale(scale_);
      if (_view)
         _view->scale(scale_);
      return true;
   }

   elements::extent presenter::size() const
   {
      return _view ? _view->size() : elements::extent{0, 0};
   }

   elements::view_limits presenter::limits() const
   {
      return _view ? _view->limits() : elements::view_limits{};
   }

   bool presenter::request_resize(elements::extent size_)
   {
      return _sink && _sink->request_resize(size_);
   }

   // The host is told whether the size it asked for is the size it got.
   // A view with fixed limits refuses anything else, which is what the
   // clap.gui contract asks for: a host that wants a size it can have
   // calls adjust_size first.
   bool presenter::resize(elements::extent size_)
   {
      if (!_view)
         return false;
      auto l = _view->limits();
      auto w = std::clamp(size_.x, l.min.x, l.max.x);
      auto h = std::clamp(size_.y, l.min.y, l.max.y);
      _view->size(elements::extent{w, h});
      return w == size_.x && h == size_.y;
   }
}
