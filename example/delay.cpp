/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/fx.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/audio_file.hpp>

///////////////////////////////////////////////////////////////////////////////
// Load an audio file and process it with delay with some feedback.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct delay_processor : q::audio_stream
{
   delay_processor(
      std::vector<float> const& in
    , q::duration delay
    , float feedback
    , std::uint32_t const sps
   )
    : audio_stream(sps, 0, 2)
    , _in(in)
    , _delay(delay, sps)
    , _feedback(feedback)
   {}

   virtual void process(out_channels const& out) override
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame = 0; frame != out.frames(); ++frame, ++_index)
      {
         // Get the next input sample
         auto s = (_index < _in.size())? _in[_index] : 0.0f;

         // Add the signal with the delayed signal
         _y = s + _delay();

         // Feed back the result to the delay
         _delay.push(_y * _feedback);

         // Output
         right[frame] = left[frame] = _y;
      }
   }

   std::vector<float> const&  _in;
   q::delay                   _delay;
   float                      _feedback;
   std::size_t                _index = 0;
   float                      _y = 0.0f;
};

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = q::wav_reader{"audio_files/Low E.wav"};
   std::uint32_t const sps = src.sps();
   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Process audio

   delay_processor proc{ in, 300_ms, 0.7, sps };

   proc.start();
   q::sleep(15_s);
   proc.stop();

   return 0;
}
