/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_processor.hpp"
#include <q/support/decibel.hpp>

namespace q = cycfi::q;

gain_processor::gain_processor(gain_controller& ctl)
 : _ctl(ctl)
{}

void gain_processor::process(in_channels const& in, out_channels const& out)
{
   // The bottom of the fader is silence.
   auto db = _ctl.volume();
   auto g = db <= -70.0 ? 0.0f : q::lin_float(q::dB(db));
   auto frames = in.frames.size();

   for (std::size_t ch = 0; ch != in.size(); ++ch)
   {
      auto i = in[ch].begin();
      auto o = out[ch].begin();
      for (std::size_t f = 0; f != frames; ++f)
         o[f] = i[f] * g;
   }
}
