/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/presenter.hpp>
#include <elements/view.hpp>
#include <algorithm>

namespace cycfi::qplug
{
   namespace detail
   {
      // Platform side, in host_view.mm on macOS: put the Elements view
      // into the host's parent view.
      void add_subview(void* parent, void* child);
   }

   namespace
   {
      void destroy_view(elements::view* v)
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
      if (_view)
         return true;

      // Unparented for now; the host hands us its view in attach.
      _view.reset(new elements::view(elements::host_view_handle{}));

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

      on_attach(*_view);

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

      resize(size_);
      return true;
   }

   bool presenter::attach(void* parent, elements::extent size_)
   {
      if (!parent || !create(size_))
         return false;
      detail::add_subview(parent, _view->host());
      _view->refresh();
      return true;
   }

   void presenter::detach()
   {
      if (_view)
      {
         on_detach();
         _binder.clear();
         _gestures.clear();
         _view.reset();
      }
   }

   void presenter::show(bool show)
   {
      if (_view && show)
         _view->refresh();
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

   bool presenter::resize(elements::extent size_)
   {
      if (!_view)
         return false;
      auto l = _view->limits();
      auto w = std::clamp(size_.x, l.min.x, l.max.x);
      auto h = std::clamp(size_.y, l.min.y, l.max.y);
      _view->size(elements::extent{w, h});
      return true;
   }
}
