/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DX_SYSEX_HPP_OCTOBER_1_2026)
#define CYCFI_Q_DX_SYSEX_HPP_OCTOBER_1_2026

#include <q/synth/fm/dx_patcher.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // DX7 patches from the DX7's own system exclusive dumps, as .syx files
   // carry them: dx_cartridge reads a bulk dump of 32 voices, packed, and
   // dx_voice a dump of one voice. Either is nothing when the bytes are not
   // that dump: another header, or another length. A field out of its range
   // is clamped, since old cartridges carry stray bits. The checksum is not
   // checked: many cartridges in circulation fail it and play correctly.
   // The layouts are in the notes after the implementation.
   ////////////////////////////////////////////////////////////////////////////
   using dx_bytes = std::span<std::uint8_t const>;
   using dx_bank = std::array<dx_patch, 32>;

   std::optional<dx_bank>  dx_cartridge(dx_bytes bytes);
   std::optional<dx_patch> dx_voice(dx_bytes bytes);

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      constexpr std::size_t dx_bulk_size = 6 + 4096 + 2;      // note 1
      constexpr std::size_t dx_single_size = 6 + 155 + 2;     // note 2

      constexpr std::uint8_t dx_clamp(std::uint8_t v, std::uint8_t max)
      {
         return std::min(v, max);
      }

      // F0 43 0n ..: Yamaha, any device number
      inline bool dx_header(
         dx_bytes b, std::size_t size, std::uint8_t format
       , std::uint8_t count_hi, std::uint8_t count_lo)
      {
         return b.size() == size && b[0] == 0xF0 && b[1] == 0x43
            && (b[2] & 0xF0) == 0 && b[3] == format
            && b[4] == count_hi && b[5] == count_lo && b[size - 1] == 0xF7;
      }

      // A voice name is ten ASCII characters; anything else is a space.
      inline void dx_name(std::uint8_t const* src, char (&name)[11])
      {
         for (std::size_t i = 0; i != 10; ++i)
         {
            auto c = src[i];
            name[i] = (c >= 32 && c < 127)? char(c) : ' ';
         }
         name[10] = 0;
      }

      // The global fields, in the order both layouts share (note 2)
      inline void dx_globals(std::uint8_t const* g, dx_patch& p)
      {
         for (std::size_t s = 0; s != 4; ++s)
         {
            p.pitch_env.rate[s] = dx_clamp(g[s], 99);
            p.pitch_env.level[s] = dx_clamp(g[4 + s], 99);
         }
         p.algorithm.algorithm = (g[8] & 31) + 1;
         p.algorithm.feedback = g[9] & 7;
         p.osc_key_sync = g[10] & 1;
         p.lfo.speed = dx_clamp(g[11], 99);
         p.lfo.delay = dx_clamp(g[12], 99);
         p.pitch_mod_depth = dx_clamp(g[13], 99);
         p.amp_mod_depth = dx_clamp(g[14], 99);
         p.lfo.key_sync = g[15] & 1;
         p.lfo.wave = dx_clamp(g[16], 5);
         p.pitch_mod_sens = g[17] & 7;
         p.transpose = dx_clamp(g[18], 48);
      }

      // The fields an operator has in both layouts
      inline void dx_operator(std::uint8_t const* o, dx_op_config& op)
      {
         for (std::size_t s = 0; s != 4; ++s)
         {
            op.env.rate[s] = dx_clamp(o[s], 99);
            op.env.level[s] = dx_clamp(o[4 + s], 99);
         }
         op.break_point = dx_clamp(o[8], 99);
         op.left_depth = dx_clamp(o[9], 99);
         op.right_depth = dx_clamp(o[10], 99);
      }

      // One voice of a cartridge: 128 bytes, packed (note 1)
      inline dx_patch dx_unpack(std::uint8_t const* v)
      {
         dx_patch p;
         for (std::size_t k = 0; k != 6; ++k)
         {
            auto const* o = v + 17 * k;
            auto& op = p.op[5 - k];                      // OP6 first
            dx_operator(o, op);
            op.left_curve = o[11] & 3;
            op.right_curve = (o[11] >> 2) & 3;
            op.rate_scaling = o[12] & 7;
            op.detune = dx_clamp((o[12] >> 3) & 15, 14);
            op.amp_mod_sens = o[13] & 3;
            op.velocity_sens = (o[13] >> 2) & 7;
            op.output_level = dx_clamp(o[14], 99);
            op.fixed = o[15] & 1;
            op.coarse = (o[15] >> 1) & 31;
            op.fine = dx_clamp(o[16], 99);
         }

         // The globals, unpacked into the shared order
         auto const* g = v + 102;
         std::uint8_t const globals[] =
         {
            g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7]
          , g[8], std::uint8_t(g[9] & 7), std::uint8_t((g[9] >> 3) & 1)
          , g[10], g[11], g[12], g[13]
          , std::uint8_t(g[14] & 1), std::uint8_t((g[14] >> 1) & 7)
          , std::uint8_t((g[14] >> 4) & 7), g[15]
         };
         dx_globals(globals, p);
         dx_name(v + 118, p.name);
         return p;
      }

      // One voice, unpacked: 155 bytes (note 2)
      inline dx_patch dx_unpacked(std::uint8_t const* v)
      {
         dx_patch p;
         for (std::size_t k = 0; k != 6; ++k)
         {
            auto const* o = v + 21 * k;
            auto& op = p.op[5 - k];                      // OP6 first
            dx_operator(o, op);
            op.left_curve = o[11] & 3;
            op.right_curve = o[12] & 3;
            op.rate_scaling = o[13] & 7;
            op.amp_mod_sens = o[14] & 3;
            op.velocity_sens = o[15] & 7;
            op.output_level = dx_clamp(o[16], 99);
            op.fixed = o[17] & 1;
            op.coarse = o[18] & 31;
            op.fine = dx_clamp(o[19], 99);
            op.detune = dx_clamp(o[20], 14);
         }
         dx_globals(v + 126, p);
         dx_name(v + 145, p.name);
         return p;
      }
   }

   inline std::optional<dx_bank> dx_cartridge(dx_bytes bytes)
   {
      if (!detail::dx_header(bytes, detail::dx_bulk_size, 9, 0x20, 0x00))
         return std::nullopt;

      dx_bank bank;
      for (std::size_t i = 0; i != 32; ++i)
         bank[i] = detail::dx_unpack(bytes.data() + 6 + 128 * i);
      return bank;
   }

   inline std::optional<dx_patch> dx_voice(dx_bytes bytes)
   {
      if (!detail::dx_header(bytes, detail::dx_single_size, 0, 0x01, 0x1B))
         return std::nullopt;
      return detail::dx_unpacked(bytes.data() + 6);
   }

   // Notes:
   //
   // 1. A bulk dump: F0 43 0n 09 20 00, 4096 bytes of 32 voices of 128,
   //    a checksum, F7. A voice holds its operators first, OP6 to OP1, 17
   //    bytes each: R1-R4, L1-L4, break point, left depth, right depth,
   //    the right curve over the left (2 bits each), detune over rate
   //    scaling (4 over 3), velocity over amplitude sensitivity (3 over
   //    2), output level, coarse over the fixed bit (5 over 1), fine.
   //    Then from byte 102: the pitch envelope's R1-R4 and L1-L4, the
   //    algorithm (0 to 31), oscillator sync over feedback (1 over 3),
   //    LFO speed, delay, pitch and amplitude depth, pitch sensitivity
   //    over wave over LFO sync (3 over 3 over 1), transpose, and the
   //    name, ten characters.
   // 2. A single voice: F0 43 0n 00 01 1B, 155 bytes, a checksum, F7.
   //    The same fields one byte each, operators of 21 (OP6 first): R1-R4,
   //    L1-L4, break point, depths, curves, rate scaling, amplitude and
   //    velocity sensitivity, output level, the fixed bit, coarse, fine,
   //    detune. Then from byte 126 the globals in the order dx_globals
   //    reads, and the name.
}

#endif
