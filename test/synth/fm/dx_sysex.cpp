/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/fm/dx_sysex.hpp>
#include <cstring>
#include <vector>

namespace q = cycfi::q;

namespace
{
   // Patches made here, every field set to something of its own, so a
   // field read from the wrong place shows.
   q::dx_patch make_patch(int seed)
   {
      q::dx_patch p;
      for (int i = 0; i != 6; ++i)
      {
         auto& o = p.op[i];
         auto const k = seed + i;
         o.output_level = std::uint8_t((k * 7) % 100);
         o.fixed = (k % 2) == 1;
         o.coarse = std::uint8_t((k * 3) % 32);
         o.fine = std::uint8_t((k * 11) % 100);
         o.detune = std::uint8_t(k % 15);
         for (int s = 0; s != 4; ++s)
         {
            o.env.rate[s] = std::uint8_t((k * 13 + s * 17) % 100);
            o.env.level[s] = std::uint8_t((k * 19 + s * 23) % 100);
         }
         o.break_point = std::uint8_t((k * 29) % 100);
         o.left_depth = std::uint8_t((k * 31) % 100);
         o.right_depth = std::uint8_t((k * 37) % 100);
         o.left_curve = std::uint8_t(k % 4);
         o.right_curve = std::uint8_t((k + 1) % 4);
         o.rate_scaling = std::uint8_t(k % 8);
         o.amp_mod_sens = std::uint8_t((k + 2) % 4);
         o.velocity_sens = std::uint8_t((k + 3) % 8);
      }
      for (int s = 0; s != 4; ++s)
      {
         p.pitch_env.rate[s] = std::uint8_t((seed * 41 + s * 7) % 100);
         p.pitch_env.level[s] = std::uint8_t((seed * 43 + s * 9) % 100);
      }
      p.algorithm.algorithm = std::uint8_t(seed % 32 + 1);
      p.algorithm.feedback = std::uint8_t(seed % 8);
      p.osc_key_sync = (seed % 2) == 0;
      p.lfo.speed = std::uint8_t((seed * 47) % 100);
      p.lfo.delay = std::uint8_t((seed * 53) % 100);
      p.pitch_mod_depth = std::uint8_t((seed * 59) % 100);
      p.amp_mod_depth = std::uint8_t((seed * 61) % 100);
      p.lfo.key_sync = (seed % 3) == 0;
      p.lfo.wave = std::uint8_t(seed % 6);
      p.pitch_mod_sens = std::uint8_t((seed + 5) % 8);
      p.transpose = std::uint8_t((seed * 67) % 49);
      std::snprintf(p.name, sizeof p.name, "VOICE %-4d", seed);
      return p;
   }

   // The packing a DX7 writes, as the decoder's notes describe it
   void pack(q::dx_patch const& p, std::uint8_t* v)
   {
      for (int k = 0; k != 6; ++k)
      {
         auto const& o = p.op[5 - k];
         auto* b = v + 17 * k;
         for (int s = 0; s != 4; ++s)
         {
            b[s] = o.env.rate[s];
            b[4 + s] = o.env.level[s];
         }
         b[8] = o.break_point;
         b[9] = o.left_depth;
         b[10] = o.right_depth;
         b[11] = std::uint8_t(o.left_curve | (o.right_curve << 2));
         b[12] = std::uint8_t(o.rate_scaling | (o.detune << 3));
         b[13] = std::uint8_t(o.amp_mod_sens | (o.velocity_sens << 2));
         b[14] = o.output_level;
         b[15] = std::uint8_t((o.fixed? 1 : 0) | (o.coarse << 1));
         b[16] = o.fine;
      }
      auto* g = v + 102;
      for (int s = 0; s != 4; ++s)
      {
         g[s] = p.pitch_env.rate[s];
         g[4 + s] = p.pitch_env.level[s];
      }
      g[8] = std::uint8_t(p.algorithm.algorithm - 1);
      g[9] = std::uint8_t(p.algorithm.feedback | (p.osc_key_sync << 3));
      g[10] = p.lfo.speed;
      g[11] = p.lfo.delay;
      g[12] = p.pitch_mod_depth;
      g[13] = p.amp_mod_depth;
      g[14] = std::uint8_t((p.lfo.key_sync? 1 : 0) | (p.lfo.wave << 1)
         | (p.pitch_mod_sens << 4));
      g[15] = p.transpose;
      std::memcpy(v + 118, p.name, 10);
   }

   std::vector<std::uint8_t> cartridge(int first_seed)
   {
      std::vector<std::uint8_t> d{0xF0, 0x43, 0x00, 0x09, 0x20, 0x00};
      d.resize(6 + 4096);
      for (int i = 0; i != 32; ++i)
         pack(make_patch(first_seed + i), d.data() + 6 + 128 * i);
      int sum = 0;
      for (std::size_t i = 6; i != d.size(); ++i)
         sum += d[i];
      d.push_back(std::uint8_t(-sum & 0x7F));
      d.push_back(0xF7);
      return d;
   }

   std::vector<std::uint8_t> single(q::dx_patch const& p)
   {
      std::vector<std::uint8_t> d{0xF0, 0x43, 0x00, 0x00, 0x01, 0x1B};
      for (int k = 0; k != 6; ++k)
      {
         auto const& o = p.op[5 - k];
         for (int s = 0; s != 4; ++s)
            d.push_back(o.env.rate[s]);
         for (int s = 0; s != 4; ++s)
            d.push_back(o.env.level[s]);
         for (auto x : {o.break_point, o.left_depth, o.right_depth
          , o.left_curve, o.right_curve, o.rate_scaling, o.amp_mod_sens
          , o.velocity_sens, o.output_level, std::uint8_t(o.fixed)
          , o.coarse, o.fine, o.detune})
            d.push_back(x);
      }
      for (int s = 0; s != 4; ++s)
         d.push_back(p.pitch_env.rate[s]);
      for (int s = 0; s != 4; ++s)
         d.push_back(p.pitch_env.level[s]);
      for (auto x : {std::uint8_t(p.algorithm.algorithm - 1)
       , p.algorithm.feedback, std::uint8_t(p.osc_key_sync), p.lfo.speed
       , p.lfo.delay, p.pitch_mod_depth, p.amp_mod_depth
       , std::uint8_t(p.lfo.key_sync), p.lfo.wave, p.pitch_mod_sens
       , p.transpose})
         d.push_back(x);
      d.insert(d.end(), p.name, p.name + 10);
      d.push_back(0);                         // checksum, not checked
      d.push_back(0xF7);
      return d;
   }

   void check_same(q::dx_patch const& a, q::dx_patch const& b)
   {
      for (int i = 0; i != 6; ++i)
      {
         INFO("OP" << i + 1);
         auto const& x = a.op[i];
         auto const& y = b.op[i];
         CHECK(x.output_level == y.output_level);
         CHECK(x.fixed == y.fixed);
         CHECK(x.coarse == y.coarse);
         CHECK(x.fine == y.fine);
         CHECK(x.detune == y.detune);
         for (int s = 0; s != 4; ++s)
         {
            CHECK(x.env.rate[s] == y.env.rate[s]);
            CHECK(x.env.level[s] == y.env.level[s]);
         }
         CHECK(x.break_point == y.break_point);
         CHECK(x.left_depth == y.left_depth);
         CHECK(x.right_depth == y.right_depth);
         CHECK(x.left_curve == y.left_curve);
         CHECK(x.right_curve == y.right_curve);
         CHECK(x.rate_scaling == y.rate_scaling);
         CHECK(x.amp_mod_sens == y.amp_mod_sens);
         CHECK(x.velocity_sens == y.velocity_sens);
      }
      for (int s = 0; s != 4; ++s)
      {
         CHECK(a.pitch_env.rate[s] == b.pitch_env.rate[s]);
         CHECK(a.pitch_env.level[s] == b.pitch_env.level[s]);
      }
      CHECK(a.algorithm.algorithm == b.algorithm.algorithm);
      CHECK(a.algorithm.feedback == b.algorithm.feedback);
      CHECK(a.osc_key_sync == b.osc_key_sync);
      CHECK(a.lfo.speed == b.lfo.speed);
      CHECK(a.lfo.delay == b.lfo.delay);
      CHECK(a.lfo.key_sync == b.lfo.key_sync);
      CHECK(a.lfo.wave == b.lfo.wave);
      CHECK(a.pitch_mod_depth == b.pitch_mod_depth);
      CHECK(a.amp_mod_depth == b.amp_mod_depth);
      CHECK(a.pitch_mod_sens == b.pitch_mod_sens);
      CHECK(a.transpose == b.transpose);
      CHECK(std::string(a.name) == std::string(b.name));
   }
}

TEST_CASE("A cartridge reads back the 32 voices packed into it")
{
   auto const d = cartridge(1);
   REQUIRE(d.size() == 4104);
   auto const bank = q::dx_cartridge(d);
   REQUIRE(bank.has_value());
   for (int i = 0; i != 32; ++i)
   {
      INFO("voice " << i + 1);
      check_same((*bank)[i], make_patch(1 + i));
   }
}

TEST_CASE("A single voice reads back")
{
   auto const p = make_patch(7);
   auto const d = single(p);
   REQUIRE(d.size() == 163);
   auto const v = q::dx_voice(d);
   REQUIRE(v.has_value());
   check_same(*v, p);
}

TEST_CASE("Any device number is read; another header or length is not")
{
   auto d = cartridge(1);
   d[2] = 0x05;                          // device 6
   CHECK(q::dx_cartridge(d).has_value());

   auto other = d;
   other[1] = 0x41;                      // not Yamaha
   CHECK(!q::dx_cartridge(other).has_value());

   other = d;
   other[3] = 0x00;                      // a single voice's format
   CHECK(!q::dx_cartridge(other).has_value());

   other = d;
   other.pop_back();                     // short
   CHECK(!q::dx_cartridge(other).has_value());

   CHECK(!q::dx_voice(d).has_value());   // a cartridge is not a voice
   CHECK(!q::dx_cartridge(single(make_patch(1))).has_value());
}

TEST_CASE("Fields out of range are clamped, and a bad checksum is read")
{
   auto d = cartridge(1);
   auto* v = d.data() + 6;               // the first voice
   v[0] = 0x7F;                          // OP6 R1
   v[14] = 0x7F;                         // OP6 output level
   v[12] = std::uint8_t(0x7F);           // OP6 detune 15, rate scaling 7
   v[102 + 15] = 0x7F;                   // transpose
   v[102 + 14] = std::uint8_t(7 << 1);   // LFO wave 7
   v[118] = 0x7F;                        // a name character
   d[d.size() - 2] ^= 0x55;              // the checksum, wrong

   auto const bank = q::dx_cartridge(d);
   REQUIRE(bank.has_value());
   auto const& p = (*bank)[0];
   CHECK(p.op[5].env.rate[0] == 99);
   CHECK(p.op[5].output_level == 99);
   CHECK(p.op[5].detune == 14);
   CHECK(p.op[5].rate_scaling == 7);
   CHECK(p.transpose == 48);
   CHECK(p.lfo.wave == 5);
   CHECK(p.name[0] == ' ');
}
