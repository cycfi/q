#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q_io/audio_stream.hpp>
#include <q/fx/biquad.hpp>

///////////////////////////////////////////////////////////////////////////////
// Load an audio file and filter the low and top end out
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct filter_processor : q::port_audio_stream
{
   filter_processor(
      q::wav_memory& wav
    , q::frequency hpfFreq
    , q::frequency lpfFreq
   )
    : port_audio_stream(0, 2, wav.sps())
    , _wav(wav)
    , _hpf(hpfFreq, wav.sps())
    , _lpf(lpfFreq, wav.sps())
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames())
      {
         // Get the next input sample
         auto s = _wav()[0];

         // Mix the highpassed and lowpassed signals
         _y = _hpf(s),
         _y = _lpf(_y);

         // Output
         left[frame] = _y;
         right[frame] = _y;
      }
   }

   q::wav_memory&    _wav;
   q::highpass      _hpf;
   q::lowpass       _lpf;
   float             _y = 0.0f;
};

int main()
{
   q::wav_memory     wav{ "audio_files/Low E.wav" };
   filter_processor   proc{ wav, 1_kHz, 2_kHz };

   if (proc.is_valid())
   {
      proc.start();
      q::sleep(q::duration(wav.length()) / wav.sps());
      proc.stop();
   }

   return 0;
}