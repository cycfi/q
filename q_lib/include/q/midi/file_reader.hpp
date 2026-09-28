/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_FILE_READER_HPP_SEPTEMBER_28_2026)
#define CYCFI_Q_MIDI_FILE_READER_HPP_SEPTEMBER_28_2026

#include <q/midi/byte_reader.hpp>
#include <q/midi/processor.hpp>
#include <cstdint>
#include <vector>

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // file_reader: a standard MIDI file, seen in place.
   //
   // The file's bytes stay the caller's, as they do for byte_reader, and the
   // reader holds a view into them. Calling it plays the file into a
   // Processor, the same one a live input would feed, with each message
   // timed in samples at the sample rate given. The file's tempo, including
   // any changes in it, is applied on the way, and tracks are merged so the
   // messages arrive in time order whatever track they came from.
   //
   // Formats 0 and 1 are read. Format 2 holds independent sequences rather
   // than one piece, so its tracks are played in order, one after another.
   //
   // Anything malformed leaves the reader false, and playing it does
   // nothing.
   ////////////////////////////////////////////////////////////////////////////
   struct file_reader
   {
                              file_reader(byte_span bytes);

      explicit                operator bool() const   { return _ok; }

      int                     format() const          { return _format; }
      std::size_t             tracks() const          { return _tracks.size(); }
      float                   tempo() const           { return _tempo; }
      double                  duration() const        { return _duration; }

                              template <typename P>
                              requires concepts::midi::Processor<P>
      void                    operator()(float sps, P&& proc) const;

   private:

      struct track
      {
         std::size_t          first;                  // into the file
         std::size_t          last;
      };

      // One event, as walk hands it over.
      struct event
      {
         double               time;                   // seconds from the start
         std::uint8_t         status;
         std::uint8_t         meta;                   // the type, if meta
         byte_span            data;                   // what follows it
      };

      // Every event in time order, tempo applied on the way.
                              template <typename F>
      void                    walk(F&& f) const;

      byte_span               _bytes;
      std::vector<track>      _tracks;
      bool                    _ok = false;
      int                     _format = 0;
      int                     _division = 96;         // ticks per quarter
      int                     _frames = 0;            // SMPTE, 0 if unused
      float                   _tempo = 120.0f;        // the file's first
      double                  _duration = 0.0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      // A position in the file that reads forward and stops at the end.
      // Everything a malformed file can do is caught here: a length that
      // overruns, a quantity that never terminates, a chunk that claims
      // more than the file holds.
      struct smf_cursor
      {
         constexpr smf_cursor(byte_span bytes, std::size_t pos = 0)
          : _bytes{bytes}, _pos{pos}
         {}

         constexpr bool       done(std::size_t last) const
                              {
                                 return _pos >= last || _pos >= _bytes.size();
                              }

         constexpr std::size_t pos() const   { return _pos; }
         constexpr void       pos(std::size_t p)   { _pos = p; }

         constexpr std::uint8_t byte()
                              {
                                 if (_pos >= _bytes.size())
                                 {
                                    _bad = true;
                                    return 0;
                                 }
                                 return _bytes[_pos++];
                              }

         constexpr std::uint8_t peek() const
                              {
                                 return _pos < _bytes.size()? _bytes[_pos] : 0;
                              }

         constexpr std::uint32_t big(int n)
                              {
                                 std::uint32_t r = 0;
                                 for (int i = 0; i != n; ++i)
                                    r = (r << 8) | byte();
                                 return r;
                              }

         // A variable length quantity: seven bits a byte, the high bit
         // saying more follow. Four bytes is all one may take.
         constexpr std::uint32_t variable()
                              {
                                 std::uint32_t r = 0;
                                 for (int i = 0; i != 4; ++i)
                                 {
                                    auto b = byte();
                                    r = (r << 7) | (b & 0x7F);
                                    if (!(b & 0x80))
                                       return r;
                                 }
                                 _bad = true;
                                 return r;
                              }

         constexpr void       skip(std::size_t n)
                              {
                                 if (n > _bytes.size() - _pos)
                                 {
                                    _pos = _bytes.size();
                                    _bad = true;
                                 }
                                 else
                                 {
                                    _pos += n;
                                 }
                              }

         constexpr bool       bad() const    { return _bad; }

      private:

         byte_span            _bytes;
         std::size_t          _pos = 0;
         bool                 _bad = false;
      };

      constexpr bool smf_tag(byte_span bytes, std::size_t pos, char const* tag)
      {
         if (pos + 4 > bytes.size())
            return false;
         for (int i = 0; i != 4; ++i)
            if (bytes[pos + i] != std::uint8_t(tag[i]))
               return false;
         return true;
      }
   }

   inline file_reader::file_reader(byte_span bytes)
    : _bytes{bytes}
   {
      if (!detail::smf_tag(bytes, 0, "MThd"))
         return;

      detail::smf_cursor c{bytes, 4};
      auto header_size = c.big(4);
      if (header_size < 6)
         return;

      _format = int(c.big(2));
      auto count = c.big(2);
      auto division = int(c.big(2));
      c.skip(header_size - 6);               // a header may be longer

      if (division & 0x8000)
      {
         // SMPTE: frames a second in the high byte, as a negative number,
         // and ticks a frame in the low one.
         _frames = 256 - ((division >> 8) & 0xFF);
         _division = division & 0xFF;
         if (_frames == 0 || _division == 0)
            return;
      }
      else
      {
         _division = division;
         if (_division == 0)
            return;
      }

      // The track chunks, in the order they appear. Anything else is
      // skipped, which is what the specification asks of a reader.
      while (!c.done(bytes.size()) && _tracks.size() < count)
      {
         auto tag_at = c.pos();
         c.skip(4);
         auto size = c.big(4);
         auto first = c.pos();
         if (c.bad() || size > bytes.size() - first)
            return;

         if (detail::smf_tag(bytes, tag_at, "MTrk"))
            _tracks.push_back({first, first + size});
         c.skip(size);
      }

      if (c.bad() || _tracks.empty())
         return;

      _ok = true;

      // Where the file ends, and the tempo it starts at.
      auto tempo_set = false;
      walk([&](event const& ev)
      {
         _duration = ev.time;
         if (!tempo_set && ev.status == 0xFF && ev.meta == 0x51
            && ev.data.size() == 3 && ev.time == 0.0)
         {
            auto us = (ev.data[0] << 16) | (ev.data[1] << 8) | ev.data[2];
            if (us > 0)
            {
               _tempo = float(6e7 / us);
               tempo_set = true;
            }
         }
      });
   }

   template <typename F>
   inline void file_reader::walk(F&& f) const
   {
      if (!_ok)
         return;

      // One cursor per track, each holding the tick its next event falls
      // on and the running status it is in. The earliest tick goes first,
      // and ties go to the earlier track, which is the order a sequencer
      // writing the file had them in.
      struct position
      {
         detail::smf_cursor   cursor;
         std::uint64_t        tick = 0;
         std::uint8_t         status = 0;
         bool                 live = true;
      };

      std::vector<position> pos;
      pos.reserve(_tracks.size());
      for (auto const& t : _tracks)
         pos.push_back({detail::smf_cursor{_bytes, t.first}});

      // Format 2 is a set of independent sequences, so only one track is
      // ever live at a time.
      auto sequential = _format == 2;
      auto current = std::size_t{0};

      auto delta = [&](position& p)
      {
         p.tick += p.cursor.variable();
         if (p.cursor.bad())
            p.live = false;
      };

      for (std::size_t i = 0; i != pos.size(); ++i)
      {
         if (pos[i].cursor.done(_tracks[i].last))
            pos[i].live = false;
         else
            delta(pos[i]);
         if (sequential && i != 0)
            pos[i].live = false;
      }

      // Microseconds a quarter note, which a tempo meta event changes. The
      // seconds a tick takes follows from it, or from the frame rate when
      // the file is timed in SMPTE, where tempo has no say.
      auto us_per_quarter = 500000.0;        // 120 bpm, the default
      auto seconds = 0.0;
      std::uint64_t at = 0;                  // the tick `seconds` is for

      auto tick_seconds = [&]()
      {
         return _frames?
            1.0 / (_frames * _division) :
            (us_per_quarter * 1e-6) / _division
            ;
      };

      for (;;)
      {
         // The next event, out of every track that still has one.
         auto next = pos.size();
         for (std::size_t i = 0; i != pos.size(); ++i)
         {
            if (!pos[i].live)
               continue;
            if (next == pos.size() || pos[i].tick < pos[next].tick)
               next = i;
         }

         if (next == pos.size())
         {
            if (!sequential || ++current == pos.size())
               return;

            // On to the next sequence, from where this one ended.
            auto& p = pos[current];
            p.live = true;
            p.tick = 0;
            seconds = 0.0;
            at = 0;
            if (p.cursor.done(_tracks[current].last))
               p.live = false;
            else
               delta(p);
            continue;
         }

         auto& p = pos[next];
         seconds += (p.tick - at) * tick_seconds();
         at = p.tick;

         auto status = p.cursor.peek();
         if (status & 0x80)
         {
            p.cursor.byte();
            if (status < 0xF0)
               p.status = status;            // a run may follow
         }
         else
         {
            status = p.status;               // running status
            if (!(status & 0x80))
            {
               p.live = false;
               continue;
            }
         }

         if (status == 0xFF)                 // meta
         {
            auto type = p.cursor.byte();
            auto size = p.cursor.variable();
            auto first = p.cursor.pos();
            p.cursor.skip(size);

            if (!p.cursor.bad())
            {
               if (type == 0x51 && size == 3)
               {
                  auto us = double(
                     (_bytes[first] << 16) | (_bytes[first+1] << 8)
                     | _bytes[first+2]
                  );
                  if (us > 0.0)
                     us_per_quarter = us;
               }
               f(event{seconds, status, type,
                  byte_span{_bytes.data() + first, size}});
            }

            if (type == 0x2F)                // end of track
               p.live = false;
         }
         else if (status == 0xF0 || status == 0xF7)
         {
            auto size = p.cursor.variable();
            auto first = p.cursor.pos();
            p.cursor.skip(size);
            if (!p.cursor.bad())
               f(event{seconds, status, 0,
                  byte_span{_bytes.data() + first, size}});
         }
         else
         {
            std::uint8_t data[2] = {};
            auto n = detail::data_length(status);
            for (std::size_t i = 0; i != n; ++i)
               data[i] = p.cursor.byte();
            if (!p.cursor.bad())
               f(event{seconds, status, 0, byte_span{data, n}});
         }

         if (p.cursor.bad() || p.cursor.done(_tracks[next].last))
            p.live = false;
         else if (p.live)
            delta(p);
      }
   }

   template <typename P>
   requires concepts::midi::Processor<P>
   inline void file_reader::operator()(float sps, P&& proc) const
   {
      walk([&](event const& ev)
      {
         if (ev.status == 0xFF)              // meta: nothing to play
            return;

         auto at = std::size_t(ev.time * sps + 0.5);
         if (ev.status == 0xF0 || ev.status == 0xF7)
         {
            proc(sysex_view{ev.data}, at);
         }
         else
         {
            std::uint32_t raw = ev.status;
            for (std::size_t i = 0; i != ev.data.size(); ++i)
               raw |= std::uint32_t(ev.data[i]) << (8 * (i + 1));
            dispatch(raw_message{raw}, at, proc);
         }
      });
   }
}

namespace cycfi::q::midi
{
   using midi_1_0::file_reader;
}

#endif
