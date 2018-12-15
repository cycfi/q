/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/square.hpp>
#include <q/synth/envelope.hpp>
#include <q/fx/low_pass.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/fx/special.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/midi_stream.hpp>

///////////////////////////////////////////////////////////////////////////////
// Synthesize a bandwidth limited square wave with ADSR envelope that
// controls an amplifier and resonant filter. Control the note-on and note-
// off using MIDI.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;
namespace midi = q::midi;

struct square_synth : q::audio_stream
{
   square_synth(q::envelope::config env_cfg, std::size_t sps)
    : audio_stream(sps, 0, 2)
    , env(env_cfg, sps)
    , filter(0.5, 0.8)
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames())
      {
         // Set the filter frequency
         auto cutoff = env();
         filter.cutoff(cutoff);

         // Synthesize the square wave
         auto val = q::square(phase++);

         // Apply the envelope (amplifier and filter) with soft clip
         val = clip(filter(val) * env());

         // Output
         right[frame] = left[frame] = val;
      }
   }

   q::phase_iterator phase;            // The phase iterator
   q::envelope       env;              // The envelope
   q::reso_filter    filter;           // The resonant filter
   q::soft_clip      clip;             // Soft clip
};

struct midi_processor : midi::processor
{
   using midi::processor::operator();

   midi_processor(square_synth& synth, std::uint32_t sps)
    : _synth(synth)
    , _sps(sps)
   {}

   void operator()(midi::note_on msg, std::size_t time)
   {
      _key = msg.key();
      auto freq = midi::note_frequency(msg.key());
      _synth.phase.set(freq, _sps);
      _synth.env.trigger(float(msg.velocity()) / 128);
   }

   void operator()(midi::note_off msg, std::size_t time)
   {
      if (msg.key() == _key)
         _synth.env.release();
   }

   std::uint8_t   _key;
   square_synth&  _synth;
   std::uint32_t  _sps;
};

int main()
{
   q::midi_input_stream::set_default_device(1);

   auto env_cfg = q::envelope::config
   {
      100_ms    // attack rate
    , 3000_ms   // decay rate
    , -12_dB    // sustain level
    , 5_s       // sustain rate
    , 0.5_s     // release rate
   };

   square_synth synth{ env_cfg, 44100 };
   q::midi_input_stream stream;
   midi_processor proc{ synth, 44100 };

   synth.start();
   if (stream.is_valid())
   {
      while (true)
         stream.process(proc);
   }
   synth.stop();

   return 0;
}
