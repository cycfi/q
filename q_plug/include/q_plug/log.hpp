/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_LOG_HPP_SEPTEMBER_6_2026)
#define Q_PLUG_LOG_HPP_SEPTEMBER_6_2026

#include <elements/support/log.hpp>

// Logging goes through Elements' quill setup: one file per plugin, named
// after it, in ~/Library/Logs/<name> on macOS (or the host's sandbox
// container when that is not writable; the banner says where). Hosts give
// a plugin no environment, so Debug builds raise the categories q_plug uses
// to Info in log_init. Categories: app for the host lifecycle and
// parameters, window for the GUI, input for edits made in the GUI.
//
//    Q_PLUG_LOG(app, "activate {} Hz, {} frames", sps, max_frames);

#define Q_PLUG_LOG(cat, ...) \
   LOG_INFO(cycfi::elements::logger(cycfi::elements::log_cat::cat) \
    , __VA_ARGS__)

namespace cycfi::q_plug
{
   inline void log_init(char const* name)
   {
      elements::log_init(name);
#if !defined(NDEBUG)
      using elements::log_cat;
      for (auto cat : {log_cat::app, log_cat::window, log_cat::input})
         elements::logger(cat)->set_log_level(quill::LogLevel::Info);
#endif
   }
}

#endif
