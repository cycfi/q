/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/delay.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/audio_file.hpp>
#include "example.hpp"

///////////////////////////////////////////////////////////////////////////////
// Same as delay.cpp, but taking inut from audio interface (channel 1)
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct delay_processor : q::port_audio_stream
{
   delay_processor(
      int device_id
    , q::duration delay
    , float feedback
   )
    : port_audio_stream(q::audio_device::get(device_id), 1, 2)
    , _delay(delay, sampling_rate())
    , _feedback(feedback)
   {}

   void process(in_channels const& in, out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      auto ch0 = in[0];
      for (auto frame : out.frames())
      {
         // Get the next input sample
         auto s = ch0[frame];

         // Mix the signal and the delayed signal
         _y = s + _delay();

         // Feed back the result to the delay
         _delay.push(_y * _feedback);

         // Output
         left[frame] = s;
         right[frame] = _y;
      }
   }

   q::delay          _delay;
   float             _feedback;
   float             _y = 0.0f;
};

int main()
{
   auto audio_device_id = get_audio_device();
   auto proc = delay_processor{ audio_device_id, 500_ms, 0.4f };

   if (proc.is_valid())
   {
      proc.start();
      while (running)
         q::sleep(1_s);
      proc.stop();
   }

   return 0;
}
