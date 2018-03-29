#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q_aux/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

auto constexpr sps = 44100;

int main()
{
   // Synthesize a 440 Hz sine wave
   auto synth = q::sin(440_Hz, sps);
   auto buff = std::array<float, 4096>{};
   for (auto& val : buff)
      val = synth();

   // Write to a wav file
   auto wav = q::audio_file::writer{
      "test.wav", q::audio_file::wav, q::audio_file::_16_bits
    , 1, sps // mono, 44100 sps
   };
   wav.write(buff);

   return 0;
}
