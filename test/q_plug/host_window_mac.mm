/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "host_window.hpp"
#import <Cocoa/Cocoa.h>

namespace q_plug_test
{
   char const* window_api()
   {
      return CLAP_WINDOW_API_COCOA;
   }

   clap_window_t open_window(int width, int height)
   {
      [NSApplication sharedApplication];
      [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

      NSWindow* win = [[NSWindow alloc]
         initWithContentRect:NSMakeRect(100, 100, width, height)
         styleMask:NSWindowStyleMaskTitled
         backing:NSBackingStoreBuffered
         defer:NO];
      [win setReleasedWhenClosed:NO];
      [win orderFront:nil];

      clap_window_t w{};
      w.api = CLAP_WINDOW_API_COCOA;
      w.cocoa = (__bridge void*) [win contentView];
      return w;
   }

   void close_window(clap_window_t const& w)
   {
      NSView* view = (__bridge NSView*) w.cocoa;
      [[view window] close];
   }

   void run_events(int ms)
   {
      NSDate* end = [NSDate dateWithTimeIntervalSinceNow:ms / 1000.0];
      while ([end timeIntervalSinceNow] > 0)
      {
         @autoreleasepool
         {
            NSEvent* e = [NSApp nextEventMatchingMask:NSEventMaskAny
               untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]
               inMode:NSDefaultRunLoopMode dequeue:YES];
            if (e)
               [NSApp sendEvent:e];
         }
      }
   }
}
