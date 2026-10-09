/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// A host's side of an editor: a native top-level window for a plugin to
// embed into, and the loop that delivers its events. One implementation per
// platform: host_window_win.cpp, host_window_mac.mm, host_window_x11.cpp.
#if !defined(Q_PLUG_TEST_HOST_WINDOW_HPP_OCTOBER_9_2026)
#define Q_PLUG_TEST_HOST_WINDOW_HPP_OCTOBER_9_2026

#include <clap/clap.h>

namespace q_plug_test
{
   // The window API the plugins embed into here.
   char const* window_api();

   // A visible top-level window of the given size, its handle in the form
   // clap_window_t carries for window_api().
   clap_window_t  open_window(int width, int height);
   void           close_window(clap_window_t const& w);

   // Deliver the window system's events for about the given time.
   void           run_events(int ms);
}

#endif
