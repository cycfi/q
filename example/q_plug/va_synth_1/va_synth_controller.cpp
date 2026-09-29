/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "va_synth_controller.hpp"

using parameter_list = va_synth_controller::parameter_list;
using namespace cycfi::q::literals;

///////////////////////////////////////////////////////////////////////////////
// The ids are the parameters' identity for the life of the plugin and are
// never reused nor renumbered; the enum above is the plugin's own index into
// this list.
//
// A time runs on a logarithmic taper, since the ear hears time in ratios:
// the step from 10 to 20 ms is the step from 100 to 200 ms, and a linear
// control would spend nine tenths of its travel above 100 ms. A level does
// not, because decibels are already a ratio.
//
// Every range spans four decades, and every slider carries four major
// divisions. On a log taper that puts a tick every decade, so the five
// labels read 1 ms, 10 ms, 100 ms, 1 s, 10 s with no arithmetic for the
// reader, and every slider on the panel is marked the same way.
///////////////////////////////////////////////////////////////////////////////
parameter_list va_synth_controller::parameters() const
{
   // Id 4 was a sustain rate, retired when the sustain became a hold. An
   // id is a parameter's identity for the life of the plugin, so it is
   // left unused rather than given to something else.
   static parameter params[] =
   {
      parameter{1, "Attack", 20_ms}
         .range((1_ms).rep, (10_s).rep).log().unit("s")
    , parameter{2, "Decay", 300_ms}
         .range((1_ms).rep, (10_s).rep).log().unit("s")
    , parameter{3, "Sustain", 50.0}.range(0.0, 100.0).unit("%")
    , parameter{5, "Release", 500_ms}
         .range((1_ms).rep, (10_s).rep).log().unit("s")

      // Ids 6 to 12 belong to the filter, which arrives in stage 2. This
      // one is numbered past them so a stage 1 preset reads in stage 2.
    , parameter{13, "Velocity", 100.0}.range(0.0, 100.0).unit("%")
    , parameter{18, "Volume", 0_dB}.range(-60.0, 0.0)
   };

   return { params };
}
