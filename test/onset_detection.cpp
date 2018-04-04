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
   // Onset detection

   std::vector<float> out(src.length() * 2);
   auto i = out.begin();

   q::dc_block dc_blk{ 1_Hz, sps };
   q::peak_envelope_follower env{ 1_s, sps };
   q::onset ons{ 50_ms, sps };

   for (auto s : in)
   {
      s = dc_blk(s);
      *i++ = s;
      *i++ = ons(s, env(std::abs(s)));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "onset.wav", audio_file::wav, audio_file::_16_bits
    , 2, sps
   };
   wav.write(out);

   return 0;
}
