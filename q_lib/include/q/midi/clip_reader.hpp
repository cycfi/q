/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_CLIP_READER_HPP_NOVEMBER_8_2026)
#define CYCFI_Q_MIDI_CLIP_READER_HPP_NOVEMBER_8_2026

#include <q/midi/packet_reader.hpp>
#include <q/support/base.hpp>
#include <cstdint>
#include <cstring>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // clip_reader: a MIDI Clip File (M2-116), seen in place.
   //
   // A clip file is the eight bytes "SMF2CLIP" followed by Universal MIDI
   // Packets, big endian. Time is in Delta Clockstamps (M2-104 section
   // 7.2.3.2), in ticks of the Delta Clockstamp Ticks Per Quarter Note
   // (7.2.3.1), at the tempo of the Flex Data Set Tempo messages, 120 BPM
   // until the first. The packets before Start of Clip are the clip's
   // configuration header; the sequence runs from Start of Clip to End of
   // Clip, both included.
   //
   // As file_reader does for a standard MIDI file, the bytes stay the
   // caller's. Calling the reader plays every packet of the sequence, the
   // clock ones included, into a Processor through a packet_reader, each
   // timed in samples at the sample rate given; tempo changes apply from
   // where they stand.
   //
   // A clip with no magic, no ticks per quarter, no Start or End of Clip,
   // or a packet cut short leaves the reader false, and playing it does
   // nothing.
   ////////////////////////////////////////////////////////////////////////////
   class clip_reader
   {
   public:

                              clip_reader(byte_span bytes);

      explicit                operator bool() const   { return _ok; }

      std::uint16_t           ticks_per_quarter() const { return _tpq; }
      double                  duration() const        { return _duration; }

                              template <typename P>
                              requires concepts::midi::Processor<P>
      void                    operator()(float sps, P&& proc) const;

   private:

      // What a walk through the clip finds.
      struct summary
      {
         bool                 whole = false;
         std::uint16_t        tpq = 0;
         double               duration = 0.0;
      };

      std::uint32_t           word(std::size_t i) const;

                              template <typename F>
      summary                 walk(F&& f) const;

      byte_span               _bytes;
      std::size_t             _words = 0;
      bool                    _ok = false;
      std::uint16_t           _tpq = 0;
      double                  _duration = 0.0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline clip_reader::clip_reader(byte_span bytes)
    : _bytes{bytes}
   {
      if (bytes.size() < 8 || (bytes.size() - 8) % 4 != 0
         || std::memcmp(bytes.data(), "SMF2CLIP", 8) != 0)
         return;
      _words = (bytes.size() - 8) / 4;
      auto const s = walk([](packet const&, double) {});
      _ok = s.whole;
      _tpq = s.tpq;
      _duration = s.duration;
   }

   inline std::uint32_t clip_reader::word(std::size_t i) const
   {
      auto const* b = _bytes.data() + 8 + 4 * i;
      return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16)
         | (std::uint32_t(b[2]) << 8) | b[3];
   }

   // Every packet in order, with the time it falls at in seconds; f is
   // called for those of the sequence.
   template <typename F>
   inline clip_reader::summary clip_reader::walk(F&& f) const
   {
      summary s;
      double seconds = 0.0;
      double quarter = 0.5;                  // 120 BPM until a Set Tempo
      bool started = false, ended = false;
      std::size_t i = 0;
      while (i < _words && !ended)
      {
         auto const w0 = word(i);
         auto const n = packet_words(std::uint8_t(w0 >> 28));
         if (i + n > _words)
            return s;                        // a packet cut short
         std::uint32_t w[4] = {w0, 0, 0, 0};
         for (std::size_t k = 1; k != n; ++k)
            w[k] = word(i + k);
         i += n;

         auto const type = w0 >> 28;
         if (type == 0x0)                    // utility: the clock
         {
            auto const status = (w0 >> 20) & 0xF;
            if (status == 0x3)
               s.tpq = std::uint16_t(w0 & 0xFFFF);
            else if (status == 0x4 && s.tpq)
               seconds += (w0 & 0xFFFFF) * quarter / s.tpq;
         }
         else if (type == 0xD && ((w0 >> 8) & 0xFF) == 0 && (w0 & 0xFF) == 0)
         {
            quarter = w[1] * 1e-8;           // Set Tempo, 10 ns a quarter
         }
         else if (type == 0xF)
         {
            auto const status = (w0 >> 16) & 0x3FF;
            started = started || status == 0x20;
            ended = started && status == 0x21;
         }
         if (started)
            f(packet{w[0], w[1], w[2], w[3]}, seconds);
      }
      s.duration = seconds;
      s.whole = s.tpq != 0 && started && ended;
      return s;
   }

   template <typename P>
   requires concepts::midi::Processor<P>
   inline void clip_reader::operator()(float sps, P&& proc) const
   {
      if (!_ok)
         return;
      packet_reader<> reader;
      walk([&](packet const& p, double seconds)
      {
         reader(p, std::size_t(seconds * sps + 0.5), proc);
      });
   }
}

#endif
