/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "sampler_processor.hpp"
#include <q/support/decibel.hpp>

using namespace cycfi::q::literals;

sampler_processor::sampler_processor(sampler_controller& ctl)
 : _ctl(ctl)
{}

// A volume change slews at this rate instead of stepping, which would
// click.
void sampler_processor::activate()
{
   _gain_lp.cutoff(4_Hz, float(sps()));
}

void sampler_processor::reset()
{
   _gain_lp = gain();
   _pos = 0.0;
}

// The sample plays once, to its end, and pausing holds its place. A new
// sample, or a rewind, starts from the top. The sample is taken once per
// block and read only within it: the main thread frees an old one once a
// block has started with a newer one.
void sampler_processor::process(in_channels const&
 , out_channels const& out)
{
   auto s = _ctl.sample.receive();
   auto const rewind = _ctl.take_rewind();
   if (s != _last || rewind)
   {
      _last = s;
      _pos = 0.0;
   }

   // A sample recorded at another rate is stepped through at its own.
   auto play = s && _pos < s->length() && _ctl.playing();
   auto const step = play? s->sps / float(sps()) : 0.0f;
   auto const target = gain();

   auto left = out[0];
   auto right = out[1];
   for (auto frame : out.frames)
   {
      auto l = 0.0f;
      auto r = 0.0f;
      if (play)
      {
         auto const i = std::size_t(_pos) * s->channels;
         l = s->data[i];
         r = s->channels > 1? s->data[i + 1] : l;
         _pos += step;
         play = _pos < s->length();
      }

      auto const g = _gain_lp(target);
      left[frame] = l * g;
      right[frame] = r * g;
   }
}

// The bottom of the dial is silence.
float sampler_processor::gain() const
{
   auto db = _ctl.volume();
   return db <= sampler_controller::silence ? 0.0f : q::lin_float(db);
}
