/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "host_window.hpp"
#include <X11/Xlib.h>
#include <chrono>
#include <thread>

namespace q_plug_test
{
   namespace
   {
      // The host's own connection. A plugin opens its own and delivers its
      // events itself, on the host's timer.
      Display* display()
      {
         static Display* d = XOpenDisplay(nullptr);
         return d;
      }
   }

   char const* window_api()
   {
      return CLAP_WINDOW_API_X11;
   }

   clap_window_t open_window(int width, int height)
   {
      clap_window_t w{};
      w.api = CLAP_WINDOW_API_X11;
      auto d = display();
      if (!d)
         return w;
      auto win = XCreateSimpleWindow(d, DefaultRootWindow(d)
       , 0, 0, width, height, 0, 0, 0);
      XMapWindow(d, win);
      XSync(d, false);
      w.x11 = win;
      return w;
   }

   void close_window(clap_window_t const& w)
   {
      if (auto d = display())
      {
         XDestroyWindow(d, w.x11);
         XSync(d, false);
      }
   }

   void run_events(int ms)
   {
      using clock = std::chrono::steady_clock;
      auto const end = clock::now() + std::chrono::milliseconds(ms);
      auto d = display();
      while (clock::now() < end)
      {
         while (d && XPending(d))
         {
            XEvent ev;
            XNextEvent(d, &ev);
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
   }
}
