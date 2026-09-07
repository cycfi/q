/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#import <Cocoa/Cocoa.h>
#include <elements/view.hpp>

namespace cycfi::qplug::detail
{
   // An NSView is content in its own right and needs no parent, so the
   // plugin builds its content and learns its limits as soon as the host
   // asks for an editor, before it hands over a parent.
   extern bool const unparented_view_ok = true;

   elements::view* make_view(void*, elements::extent)
   {
      return new elements::view(elements::host_view_handle{});
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
}
