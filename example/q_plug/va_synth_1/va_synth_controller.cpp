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
///////////////////////////////////////////////////////////////////////////////
parameter_list va_synth_controller::parameters() const
{
   static parameter params[] =
   {
      parameter{1, "Attack", 20_ms}
         .range((1_ms).rep, (1_s).rep).log().unit("s")
    , parameter{2, "Decay", 300_ms}
         .range((1_ms).rep, (4_s).rep).log().unit("s")
    , parameter{3, "Sustain Level", -12_dB}.range(-60.0, 0.0)
    , parameter{4, "Sustain Rate", 5_s}
         .range((100_ms).rep, (20_s).rep).log().unit("s")
    , parameter{5, "Release", 500_ms}
         .range((1_ms).rep, (4_s).rep).log().unit("s")
   };

   return { params };
}
