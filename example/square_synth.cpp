/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/square.hpp>
#include <q/synth/envelope.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/fx/special.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/midi_stream.hpp>
#include "example.hpp"

///////////////////////////////////////////////////////////////////////////////
// Synthesize a bandwidth limited square wave with ADSR envelope that
// controls an amplifier and a resonant filter. Control the note-on and note-
// off using MIDI.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;
namespace midi = q::midi;

struct my_square_synth : q::port_audio_stream
{
   my_square_synth(q::envelope::config env_cfg, int device_id)
    : port_audio_stream(q::audio_device::get(device_id), 0, 2)
    , env(env_cfg, this->sampling_rate())
    , filter(0.5, 0.8)
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames())
      {
         // Generate the ADSR envelope
         auto env_ = env();

         // Set the filter frequency
         filter.cutoff(env_);

         // Synthesize the square wave
         auto val = q::square(phase++);

         // Apply the envelope (amplifier and filter) with soft clip
         val = clip(filter(val) * env_);

         // Output
         right[frame] = left[frame] = val;
      }
   }

   q::phase_iterator phase;            // The phase iterator
   q::envelope       env;              // The envelope
   q::reso_filter    filter;           // The resonant filter
   q::soft_clip      clip;             // Soft clip
};

struct my_midi_processor : midi::processor
{
   using midi::processor::operator();

   my_midi_processor(my_square_synth& synth)
    : _synth(synth)
   {}

   void operator()(midi::note_on msg, std::size_t time)
   {
      _key = msg.key();
      auto freq = midi::note_frequency(_key);
      _synth.phase.set(freq, _synth.sampling_rate());
      _synth.env.trigger(float(msg.velocity()) / 128);
   }

   void operator()(midi::note_off msg, std::size_t time)
   {
      if (msg.key() == _key)
         _synth.env.release();
   }

   std::uint8_t      _key;
   my_square_synth&  _synth;
};

int main()
{
   q::midi_input_stream::set_default_device(get_midi_device());
   auto audio_device_id = get_audio_device();

   auto env_cfg = q::envelope::config
   {
      100_ms      // attack rate
    , 1_s         // decay rate
    , -12_dB      // sustain level
    , 5_s         // sustain rate
    , 1_s         // release rate
   };

   my_square_synth synth{ env_cfg, audio_device_id };
   q::midi_input_stream stream;
   my_midi_processor proc{ synth };

   if (!stream.is_valid())
      return -1;

   synth.start();
   while (running)
      stream.process(proc);
   synth.stop();

   return 0;
}
