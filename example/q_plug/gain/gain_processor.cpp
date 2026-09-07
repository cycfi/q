/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_processor.hpp"
#include <q/support/decibel.hpp>

using namespace cycfi::q::literals;

namespace
{
   // A parameter change slews at this rate instead of stepping, which
   // would click.
   constexpr auto smoothing = 4_Hz;
}

gain_processor::gain_processor(gain_controller& ctl)
 : _ctl(ctl)
{}

// The stream is known here, and stays put until the next activation.
void gain_processor::activate()
{
   _gain_lp.cutoff(smoothing, sps());
}

// A transport jump: take up the current gain rather than sliding to it.
void gain_processor::reset()
{
   _gain_lp = gain();
}

void gain_processor::process(in_channels const& in, out_channels const& out)
{
   auto target = gain();
   auto frames = in.frames.size();

   for (std::size_t f = 0; f != frames; ++f)
   {
      auto g = _gain_lp(target);
      for (std::size_t ch = 0; ch != in.size(); ++ch)
         out[ch].begin()[f] = in[ch].begin()[f] * g;
   }
}

// The bottom of the fader is silence.
float gain_processor::gain() const
{
   auto db = _ctl.volume();
   auto silence = q::dB(_ctl.volume_param()._min);
   return db <= silence ? 0.0f : q::lin_float(db);
}
