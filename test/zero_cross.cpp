#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <vector>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/1-Low E.aif"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Detect zero-crossings

   std::vector<float> out(src.length() * 2);
   auto i = out.begin();

   q::zero_cross zc{ 0.001, 2600_Hz, sps };
   for (auto s : in)
   {
      *i++ = s;
      *i++ = zc(s);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "zero_cross.wav", audio_file::wav, audio_file::_16_bits
    , 2, sps
   };
   wav.write(out);

   return 0;
}
