/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_io/audio_file.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;

////////////////////////////////////////////////////////////////////////////
// wav_writer and wav_reader against each other. Every count the reader
// gives, length, position and the target of seek, is in samples across
// all channels, interleaved, the same unit read and write take.
////////////////////////////////////////////////////////////////////////////
namespace
{
   // Writes an interleaved ramp and returns what it wrote.
   std::vector<float> write_ramp(
      std::string const& path, std::uint32_t channels, float sps
    , std::size_t frames)
   {
      std::vector<float> data(frames * channels);
      for (std::size_t i = 0; i != frames; ++i)
         for (std::uint32_t ch = 0; ch != channels; ++ch)
            data[i*channels + ch] = (ch & 1? -1.0f : 1.0f) * float(i) / frames;

      q::wav_writer wav{path, channels, sps};
      REQUIRE(bool(wav));
      CHECK(wav.num_channels() == channels);
      CHECK(wav.sps() == sps);
      CHECK(wav.write(data) == data.size());
      return data;                              // the file closes here
   }
}

TEST_CASE("A mono file reads back what was written")
{
   auto const path = "results/audio_file_mono.wav";
   auto const written = write_ramp(path, 1, 48000.0f, 1000);

   q::wav_reader wav{path};
   REQUIRE(bool(wav));
   CHECK(wav.sps() == 48000.0f);
   CHECK(wav.num_channels() == 1);
   CHECK(wav.length() == 1000);
   CHECK(wav.position() == 0);

   std::vector<float> read(1000);
   CHECK(wav.read(read) == 1000);
   CHECK(read == written);                      // 32 bit float, so exact
   CHECK(wav.position() == 1000);

   // At the end there is nothing more.
   CHECK(wav.read(read) == 0);
}

TEST_CASE("A stereo file interleaves its channels")
{
   auto const path = "results/audio_file_stereo.wav";
   auto const written = write_ramp(path, 2, 44100.0f, 500);

   q::wav_reader wav{path};
   REQUIRE(bool(wav));
   CHECK(wav.sps() == 44100.0f);
   CHECK(wav.num_channels() == 2);
   CHECK(wav.length() == 1000);                 // samples, both channels

   std::vector<float> read(1000);
   CHECK(wav.read(read) == 1000);
   CHECK(read == written);
   CHECK(read[2*10] == 10.0f/500);              // frame 10, left
   CHECK(read[2*10+1] == -10.0f/500);           // frame 10, right
}

TEST_CASE("seek and restart move the read position, in samples")
{
   auto const path = "results/audio_file_seek.wav";
   auto const written = write_ramp(path, 2, 48000.0f, 300);

   q::wav_reader wav{path};
   REQUIRE(bool(wav));

   // Into the middle: sample 200 is frame 100, left.
   REQUIRE(wav.seek(200));
   CHECK(wav.position() == 200);
   float one[2];
   CHECK(wav.read(one, 2) == 2);
   CHECK(one[0] == written[200]);
   CHECK(one[1] == written[201]);
   CHECK(wav.position() == 202);

   // Back to the start, and the first frame again.
   REQUIRE(wav.restart());
   CHECK(wav.position() == 0);
   CHECK(wav.read(one, 2) == 2);
   CHECK(one[0] == written[0]);
   CHECK(one[1] == written[1]);

   // A partial read at the tail returns what is left.
   REQUIRE(wav.seek(596));
   std::vector<float> tail(10);
   CHECK(wav.read(tail) == 4);
}

TEST_CASE("A file that does not exist reads as nothing")
{
   q::wav_reader wav{"results/no_such_file.wav"};
   CHECK(!bool(wav));
   CHECK(wav.sps() == 0);
   CHECK(wav.num_channels() == 0);
   CHECK(wav.length() == 0);
   std::vector<float> buf(8);
   CHECK(wav.read(buf) == 0);
   CHECK(!wav.seek(0));
   CHECK(!wav.restart());
}
