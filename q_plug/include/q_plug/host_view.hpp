/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_HOST_VIEW_HPP_SEPTEMBER_12_2026)
#define QPLUG_HOST_VIEW_HPP_SEPTEMBER_12_2026

#include <elements/view.hpp>
#include <functional>

namespace cycfi::qplug::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // The plugin's view: Elements' view with a hook on the keyboard, so
   // the presenter can take a key before the content sees it. That is
   // where a shortcut every plugin shares, zoom say, is handled once.
   ////////////////////////////////////////////////////////////////////////////
   class plugin_view : public elements::view
   {
   public:

      using elements::view::view;
      using key_function = std::function<bool(elements::key_info const&)>;

      bool           key(elements::key_info const& k) override
      {
         if (on_key && on_key(k))
            return true;
         return elements::view::key(k);
      }

      key_function   on_key;
   };

   // Platform side, in macos/host_view.mm and windows/host_view.cpp.

   // Whether a view may exist before the host gives us a parent. A Cocoa
   // view may, so the content is built and measured as soon as the host
   // asks for an editor. A Win32 child window may not, so there the view
   // waits for the parent to arrive in attach.
   extern bool const unparented_view_ok;

   // Makes the view. parent is null when it is made early.
   plugin_view*   make_view(void* parent, elements::extent size_);

   // Puts the view into the host's, where that is a separate step.
   void           add_subview(void* parent, void* child);

   // Physical pixels per logical unit in the window API the host speaks:
   // the display's DPI over 96 on Windows, where hosts deal in pixels, and
   // 1 on macOS, where they deal in points. Taken from the view's own
   // window once there is one, the same figure Elements scales by, and
   // from the system before.
   float          pixel_scale(plugin_view const* view);
}

#endif
