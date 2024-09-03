/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/fx/dynamic.hpp>
#include <q/synth/saw_osc.hpp>
#include <q_io/audio_file.hpp>
#include <array>
#include "test.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 44100;
constexpr auto size = sps;
constexpr auto buffer_size = size;

constexpr auto n_channels = 1;
using in_buffer = std::array<float, buffer_size>;
using out_buffer = std::array<float, buffer_size*n_channels>;

///////////////////////////////////////////////////////////////////////////////
void test_compressor(in_buffer const& ramp)
{
   out_buffer out;

   auto comp = q::compressor{ -6_dB, 1.0/4 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * lin_float(comp(q::lin_to_db(s)));
   }

   {
      q::wav_writer wav(
         "results/curve_compressor.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("curve_compressor", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void test_soft_knee_compressor(in_buffer const& ramp)
{
   out_buffer out;

   auto comp = q::soft_knee_compressor{ -6_dB, 3_dB, 1.0/4 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * lin_float(comp(q::lin_to_db(s)));
   }

   {
      q::wav_writer wav(
         "results/curve_soft_knee_compressor.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("curve_soft_knee_compressor", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void test_expander(in_buffer const& ramp)
{
   out_buffer out;

   auto exp = q::expander{ -6_dB, 4/1.0 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * lin_float(exp(q::lin_to_db(s)));
   }

   {
      q::wav_writer wav(
         "results/curve_expander.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("curve_expander", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void test_compressor_expander(in_buffer const& ramp)
{
   out_buffer out;

   auto comp = q::compressor{ -6_dB, 1.0/4 };
   auto exp = q::expander{ -12_dB, 4/1.0 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      auto env_db= q::lin_to_db(s);
      out[ch1] = s * lin_float(comp(env_db) + exp(env_db));
   }

   {
      q::wav_writer wav(
         "results/curve_compressor_expander.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("curve_compressor_expander", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void test_agc(in_buffer const& ramp)
{
   out_buffer out;

   auto agc = q::agc{ 12_dB };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * lin_float(agc(q::lin_to_db(s), -6_dB));
   }

   {
      q::wav_writer wav(
         "results/curve_agc.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("curve_agc", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void test_limiter(in_buffer const& ramp)
{
   out_buffer out;

   auto lim = q::compressor{ -6_dB, 1.0/100 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * lin_float(lim(q::lin_to_db(s)));
   }

   {
      q::wav_writer wav(
         "results/curve_limiter.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("curve_limiter", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void test_compressor_limiter(in_buffer const& ramp)
{
   out_buffer out;

   auto comp = q::compressor{ -9_dB, 1.0/2 };
   auto lim = q::compressor{ -6_dB, 1.0/50 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      auto env_db = q::lin_to_db(s);
      auto comp_db = comp(env_db);
      auto lim_db = lim(env_db);

      out[ch1] = s * lin_float(std::min(comp_db, lim_db));
   }

   {
      q::wav_writer wav(
         "results/compressor_limiter.wav", n_channels, sps
      );
      wav.write(out);
   }
   compare_golden("compressor_limiter", 1e-6);
}

///////////////////////////////////////////////////////////////////////////////
void gen_ramp(in_buffer& ramp)
{
   const auto f = q::phase(1_Hz, sps);       // The synth frequency
   auto ph = q::phase();                     // Our phase accumulator

   for (auto i = 0; i != size; ++i)
   {
      ramp[i] = (q::basic_saw(ph) + 1) / 2;
      ph += f;
   }
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_compressor")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_compressor(ramp);
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_soft_knee_compressor")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_soft_knee_compressor(ramp);
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_expander")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_expander(ramp);
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_compressor_expander")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_compressor_expander(ramp);
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_agc")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_agc(ramp);
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_limiter")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_limiter(ramp);
}

///////////////////////////////////////////////////////////////////////////////
TEST_CASE("TEST_compressor_limiter")
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);
   test_compressor_limiter(ramp);
}



