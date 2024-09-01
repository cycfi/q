/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/delay.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/audio_file.hpp>
#include <q/utility/sleep.hpp>

///////////////////////////////////////////////////////////////////////////////
// Load an audio file and process it with delay with some feedback.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct delay_processor : q::audio_stream
{
   delay_processor(
      float* wav
    , float sps
    , q::duration delay
    , float feedback
   )
    : audio_stream(0, 2, sps)
    , _wav(wav)
    , _delay(delay, sps)
    , _feedback(feedback)
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames)
      {
         // Get the next input sample
         auto s = *_wav++;

         // Mix the signal and the delayed signal
         _y = s + _delay();

         // Feed back the result to the delay
         _delay.push(_y * _feedback);

         // Output
         left[frame] = s;
         right[frame] = _y;
      }
   }

   float*   _wav;
   q::delay _delay;
   float    _feedback;
   float    _y = 0.0f;
};

int main()
{
   q::wav_reader wav{"audio_files/Low E.wav"};
   if (wav)
   {
      std::vector<float> in(wav.length());
      wav.read(in);

      delay_processor proc{in.data(), wav.sps(), 350_ms, 0.85f};

      if (proc.is_valid())
      {
         proc.start();
         q::sleep(q::duration(wav.length()) / wav.sps());
         proc.stop();
      }
   }

   return 0;
}
