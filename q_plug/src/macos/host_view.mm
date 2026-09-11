/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#import <Cocoa/Cocoa.h>
#include <qplug/host_view.hpp>

namespace cycfi::qplug::detail
{
   // An NSView is content in its own right and needs no parent, so the
   // plugin builds its content and learns its limits as soon as the host
   // asks for an editor, before it hands over a parent.
   extern bool const unparented_view_ok = true;

   // Born at the size the host will show it at. Made from a null handle
   // instead, Elements falls back to a 100 by 100 square, and the host
   // that reads the frame before the view is resized shows that square
   // for a moment: the window opens small and springs to size.
   plugin_view* make_view(void*, elements::extent size)
   {
      return new plugin_view(size);
   }

   // The host has a parent for us now: make the view fill it and follow
   // it.
   void add_subview(void* parent, void* child)
   {
      NSView* parent_ = (__bridge NSView*) parent;
      NSView* child_ = (__bridge NSView*) child;
      child_.frame = parent_.bounds;
      child_.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
      [parent_ addSubview : child_];
   }

   // Hosts deal in points here, as Cocoa does: nothing to scale.
   float pixel_scale(plugin_view const*)
   {
      return 1.0f;
   }
}
