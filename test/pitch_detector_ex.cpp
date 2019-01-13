/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/low_pass.hpp>
#include <q/fx/biquad.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/waveshaper.hpp>

#include <vector>
#include <iostream>
#include <fstream>

#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

void process(
   std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Prepare output file

   std::ofstream csv("results/frequencies_" + name + ".csv");

   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Output
   constexpr auto n_channels = 5;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector          pd{ lowest_freq, highest_freq, sps, -30_dB };
   auto const&                bits = pd.bits();
   auto const&                edges = pd.edges();
   q::auto_correlator         bacf{ bits };
   auto                       min_period = float(highest_freq.period()) * sps;

   q::peak_envelope_follower  env{ 30_ms, sps };
   q::one_pole_lowpass        lp{ highest_freq, sps };
   q::one_pole_lowpass        lp2{ lowest_freq, sps };
   q::lowpass                 lp3 = { highest_freq, sps, 0.70710678 };

   constexpr float            slope = 1.0f/4;
   constexpr float            makeup_gain = 4;
   q::compressor              comp{ -18_dB, slope };
   q::clip                    clip;

   float                      onset_threshold = float(-30_dB);
   float                      release_threshold = float(-60_dB);
   float                      threshold = onset_threshold;

   int ii = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;      // input
      auto ch2 = pos+1;    // zero crossings
      auto ch3 = pos+2;    // bacf
      auto ch4 = pos+3;    // frequency
      auto ch5 = pos+4;    // predicted frequency

      float time = i / float(sps);

      auto s = in[i];

      // Bandpass filter
      s = lp(s);
      // s = lp3(s);
      s -= lp2(s);

      // Envelope
      auto e = env(std::abs(s));

      if (e > threshold)
      {
         // Compressor + makeup-gain + hard clip
         auto gain = float(comp(e)) * makeup_gain;
         s = clip(s * gain);
         threshold = release_threshold;
      }
      else
      {
         s = 0.0f;
         threshold = onset_threshold;
      }

      out[ch1] = s;

      // Pitch Detect
      bool ready = pd(s);

      out[ch2] = -0.8;  // placeholder for bitset bits
      out[ch3] = 0.0f;  // placeholder for autocorrelation results

      if (ready)
      {
         auto frame = edges.frame() + (edges.window_size() / 2);
         auto extra = frame - edges.window_size();
         auto size = bits.size();

         // Print the bitset bits
         {
            auto out_i = (&out[ch2] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size; ++i)
            {
               *out_i = bits.get(i) * 0.8;
               out_i += n_channels;
            }
         }

         // Print the autocorrelation results
         {
            auto weight = 2.0 / size;
            auto out_i = (&out[ch3] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size/2; ++i)
            {
               if (i > min_period)
                  *out_i = 1.0f - (bacf(i) * weight);
               out_i += n_channels;
            }
         }
         csv << pd.frequency() << ", " << pd.periodicity() << time << std::endl;
      }

      // Print the frequency
      {
         auto f = pd.frequency() / double(highest_freq);
         auto fi = int(i - bits.size());
         if (fi >= 0)
            out[(fi * n_channels) + 3] = f;
      }

      // Print the predicted frequency
      {
         auto f = pd.predict_frequency() / double(highest_freq);
         out[ch5] = f;
      }
   }

   csv.close();

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav{
      "results/pitch_detect_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

void process(std::string name, q::frequency lowest_freq)
{
   process(name, lowest_freq * 0.8, lowest_freq * 5);
}

#define ALL_TESTS 0
#define LOW_FREQUENCY_TESTS 1
#define PHRASE_TESTS 1
#define STACCATO_TESTS 1

int main()
{
   using namespace notes;

#if LOW_FREQUENCY_TESTS==1 || ALL_TESTS==1

   process("-2a-F#", low_fs);
   process("-2b-F#-12th", low_fs);
   process("-2c-F#-24th", low_fs);

   process("-1a-Low-B", low_b);
   process("-1b-Low-B-12th", low_b);
   process("-1c-Low-B-24th", low_b);

#endif
#if ALL_TESTS==1

   process("sin_440", d);

   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("1c-Low-E-24th", low_e);

   process("2a-A", a);
   process("2b-A-12th", a);
   process("2c-A-24th", a);

   process("3a-D", d);
   process("3b-D-12th", d);
   process("3c-D-24th", d);

   process("4a-G", g);
   process("4b-G-12th", g);
   process("4c-G-24th", g);

   process("5a-B", b);
   process("5b-B-12th", b);
   process("5c-B-24th", b);

   process("6a-High-E", high_e);
   process("6b-High-E-12th", high_e);
   process("6c-High-E-24th", high_e);

#endif
#if PHRASE_TESTS==1 || ALL_TESTS==1

   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Slide G", g);
   process("Bend-Slide G", g);

#if STACCATO_TESTS==1 || ALL_TESTS==1

   process("GLines1", g);
   process("GLines2", g);
   process("GLines3", g);
   process("SingleStaccato", g);
   process("GStaccato", g);
   process("ShortStaccato", g);

#endif
#endif

   return 0;
}

