#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <q/pitch_detector.hpp>
#include <q/pitch_detector.hpp>
#include <vector>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

void process(std::string name, q::frequency lowest_freq, q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/" + name + ".aif"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );

   ////////////////////////////////////////////////////////////////////////////
   // Process
   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);
   auto i = out.begin();

   q::one_pole_lowpass        lp{ highest_freq, sps };
   q::peak                    pk{ 0.7, 0.001 };
   q::peak_envelope_follower  env{ highest_freq.period() * 10, sps };
   q::bacf<>                  bacf{ lowest_freq, highest_freq, sps };

   // Correlation input/output iterator
   std::uint16_t const*       corr_i = nullptr;
   float                      corr_max_count = 0;
   float*                     corr_o = nullptr;
   std::size_t                corr_count = 0;
   std::size_t                corr_size = bacf.size() / 2;

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;

      // Low pass
      s = lp(s);
      *i++ = s;

      // Peaks
      auto p = pk(s, env(s));
      *i++ = p * 0.8;

      // Placeholder for correlation
      bool proc = bacf(p);

      if (corr_o == nullptr && !bacf.is_start())
         corr_o = &*i;
      i++;

      if (proc)
      {
         corr_count = 0;
         auto const& info = bacf.result();
         corr_i = info.correlation.data();
         corr_max_count = info.max_count;
      }

      if (corr_i && corr_o && corr_max_count)
      {
         *corr_o = *corr_i++ / corr_max_count;
         corr_o += n_channels;
         if (++corr_count == corr_size)
         {
            corr_i = nullptr;
            corr_o = nullptr;
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/bacf_" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E", 70_Hz, 400_Hz);


   // process("2-Low E 2th", 329.64_Hz);
   // process("3-A", 440.00_Hz);
   // process("4-A 12th", 440.00_Hz);
   // process("5-D", 587.32_Hz);
   // process("6-D 12th", 587.32_Hz);
   // process("7-G", 784.00_Hz);
   // process("8-G 12th", 784.00_Hz);
   // process("9-B", 987.76_Hz);
   // process("10-B 12th", 987.76_Hz);
   process("11-High E", 200_Hz, 1500_Hz);
   // process("12-High E 12th", 1318.52_Hz);
   return 0;
}

