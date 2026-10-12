/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "looper_processor.hpp"
#include <q/support/decibel.hpp>

using namespace cycfi::q::literals;

looper_processor::looper_processor(looper_controller& ctl)
 : _ctl(ctl)
{}

// A volume change slews at this rate instead of stepping, which would
// click.
void looper_processor::activate()
{
   _gain_lp.cutoff(4_Hz, float(sps()));
}

void looper_processor::reset()
{
   _gain_lp = gain();
   _pos = 0.0;
}

// The sample plays once, or loops, and pausing holds its place. A new
// sample, or a rewind, starts from the top. The sample is taken once per
// block and read only within it: the main thread frees an old one once a
// block has started with a newer one.
void looper_processor::process(in_channels const&
 , out_channels const& out)
{
   auto s = _ctl.sample.receive();
   auto const rewind = _ctl.take_rewind();
   if (s != _last || rewind)
   {
      _last = s;
      _pos = 0.0;
      _ended = false;
   }
   if (!_ctl.playing())
      _ended = false;

   // A sample recorded at another rate is stepped through at its own.
   auto play = s && s->length() != 0 && _ctl.playing() && !_ended;
   auto const loop = _ctl.looping();
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
         if (_pos >= s->length())
         {
            if (loop)
            {
               _pos -= s->length();
            }
            else
            {
               play = false;
               end();
            }
         }
      }

      auto const g = _gain_lp(target);
      left[frame] = l * g;
      right[frame] = r * g;
   }
}

// Played once, to the end: back to the top, silent, and Play off. The
// host takes edits on the main thread, so the controller makes that one
// there. Until it lands, Play still reads on, and the sample stays silent.
void looper_processor::end()
{
   _pos = 0.0;
   _ended = true;
   auto ctl = &_ctl;
   _ctl.post([ctl] { ctl->stop(); });
}

// The bottom of the dial is silence.
float looper_processor::gain() const
{
   auto db = _ctl.volume();
   return db <= looper_controller::silence ? 0.0f : q::lin_float(db);
}
