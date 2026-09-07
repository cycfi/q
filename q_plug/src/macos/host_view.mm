/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#import <Cocoa/Cocoa.h>

namespace cycfi::qplug::detail
{
   // The Elements view was made without a parent, so the plugin could
   // build its content and learn its limits before the host asked for a
   // parent. Now the host has one: make the view fill it and follow it.
   void add_subview(void* parent, void* child)
   {
      NSView* parent_ = (__bridge NSView*) parent;
      NSView* child_ = (__bridge NSView*) child;
      child_.frame = parent_.bounds;
      child_.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
      [parent_ addSubview : child_];
   }
}
