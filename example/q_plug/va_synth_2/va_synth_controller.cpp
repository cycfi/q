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

      // The filter. Cutoff is heard in ratios, so it runs on a log taper
      // like the times; its range is the audible band rather than four
      // whole decades, so its labels are the frequencies themselves and
      // no travel is spent below hearing. Resonance is a ratio too, four
      // octaves of it. Depth is in octaves, which is what the ear counts,
      // so it is the one control here that is plainly linear.
    , parameter{6, "Cutoff", 300_Hz}
         .range((20_Hz).rep, (20_kHz).rep).log()
    , parameter{7, "Resonance", 2.0}.range(0.5, 8.0).log()
    , parameter{8, "Env Depth", 4.0}.range(0.0, 8.0).unit("oct")

      // The filter's own contour, on the same tapers as the amplifier's.
      // Its default is the shape a filter usually wants and an amplifier
      // usually does not: open at once, fall away, stay down.
    , parameter{9, "Filter Attack", 5_ms}
         .range((1_ms).rep, (10_s).rep).log().unit("s")
    , parameter{10, "Filter Decay", 200_ms}
         .range((1_ms).rep, (10_s).rep).log().unit("s")
    , parameter{11, "Filter Sustain", 50.0}.range(0.0, 100.0).unit("%")
    , parameter{12, "Filter Release", 300_ms}
         .range((1_ms).rep, (10_s).rep).log().unit("s")

      // Velocity, to the loudness and to the contour. 13 is stage 1's.
    , parameter{13, "Velocity", 100.0}.range(0.0, 100.0).unit("%")
    , parameter{14, "Filter Velocity", 50.0}.range(0.0, 100.0).unit("%")
   };

   return { params };
}
