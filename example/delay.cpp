/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/delay.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/audio_file.hpp>

///////////////////////////////////////////////////////////////////////////////
// Load an audio file and process it with delay with some feedback.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct delay_processor : q::port_audio_stream
{
   delay_processor(
      q::wav_memory& wav
    , q::duration delay
    , float feedback
   )
    : port_audio_stream(0, 2, wav.sps())
    , _wav(wav)
    , _delay(delay, wav.sps())
    , _feedback(feedback)
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames())
      {
         // Get the next input sample
         auto s = _wav()[0];

         // Mix the signal and the delayed signal
         _y = s + _delay();

         // Feed back the result to the delay
         _delay.push(_y * _feedback);

         // Output
         left[frame] = s;
         right[frame] = _y;
      }
   }

   q::wav_memory&    _wav;
   q::delay          _delay;
   float             _feedback;
   float             _y = 0.0f;
};

int main()
{
   q::wav_memory     wav{ "audio_files/Low E.wav" };
   delay_processor   proc{ wav, 350_ms, 0.85f };

   if (proc.is_valid())
   {
      proc.start();
      q::sleep(q::duration(wav.length()) / wav.sps());
      proc.stop();
   }

   return 0;
}
