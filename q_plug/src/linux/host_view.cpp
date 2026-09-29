/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/host_view.hpp>
#include <X11/Xlib.h>
#include <cstdint>

// From Elements' X11 host, which every plugin compiles into itself.
namespace cycfi::elements
{
   Display* get_display();
   void     dispatch_event(XEvent& ev);
   void     poll_views();
   double   display_scale();
   double   window_scale(::Window w);
}

namespace cycfi::qplug::detail
{
   // An X11 child window is made inside its parent, so the view waits for
   // the parent to arrive in attach, as on Windows.
   extern bool const unparented_view_ok = false;

   plugin_view* make_view(void* parent, elements::extent)
   {
      auto const window = static_cast<unsigned long>(
         reinterpret_cast<std::uintptr_t>(parent));
      auto* view = new plugin_view(window);
      view->parent = window;
      return view;
   }

   // Nothing to do: the view was made as a child of the host's window.
   void add_subview(void*, void*)
   {}

   // The monitor under the host's window, the figure Elements scales the
   // view by; the desktop's before there is a window.
   float pixel_scale(plugin_view const* view)
   {
      if (view && view->parent)
         return float(elements::window_scale(view->parent));
      return float(elements::display_scale());
   }

   int event_fd()
   {
      return ConnectionNumber(elements::get_display());
   }

   void pump_events()
   {
      auto* d = elements::get_display();
      while (XPending(d))
      {
         XEvent ev;
         XNextEvent(d, &ev);
         elements::dispatch_event(ev);
      }
      elements::poll_views();
   }
}
