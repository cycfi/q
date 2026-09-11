/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/host_view.hpp>

namespace cycfi::qplug::detail
{
   // A Win32 child window needs its parent when it is created, so the
   // view is made in attach, once the host has handed us its window.
   extern bool const unparented_view_ok = false;

   plugin_view* make_view(void* parent, elements::extent)
   {
      return new plugin_view(
         static_cast<elements::host_view_handle>(parent));
   }

   // Nothing to do: the view was made as a child of the host's window.
   void add_subview(void*, void*)
   {}

   float pixel_scale(plugin_view const* view)
   {
      auto const hwnd = view? (HWND) view->host() : nullptr;
      auto const dpi = hwnd? GetDpiForWindow(hwnd) : GetDpiForSystem();
      return dpi? dpi / 96.0f : 1.0f;
   }
}
