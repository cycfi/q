/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "delay_processor.hpp"

using namespace cycfi::q::literals;

namespace
{
   // The delay time slews at this rate, so moving the control glides the
   // echoes rather than stepping them.
   constexpr auto smoothing = 4_Hz;
}

delay_processor::delay_processor(delay_controller& ctl)
 : _ctl(ctl)
{}

void delay_processor::activate()
{
   _delay = q::delay{delay_controller::max_delay, float(sps())};
   _delay_lp.cutoff(smoothing, sps());
}

void delay_processor::reset()
{
   _delay.clear();
   _delay_lp = delay_samples();
}

void delay_processor::process(in_channels const& in, out_channels const& out)
{
   auto target = delay_samples();
   auto feedback = float(_ctl.feedback() / 100.0);
   auto frames = in.frames.size();

   auto i = in[0].begin();
   auto o = out[0].begin();

   for (std::size_t f = 0; f != frames; ++f)
   {
      // Mix the signal with the delayed signal, and feed the result back
      // into the delay line.
      auto s = i[f];
      auto y = s + _delay(_delay_lp(target));
      _delay.push(y * feedback);
      o[f] = y;
   }
}

float delay_processor::delay_samples() const
{
   return float(as_double(_ctl.delay()) * sps());
}
