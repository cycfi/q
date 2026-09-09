/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PER_NOTE_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_PER_NOTE_HPP_SEPTEMBER_9_2026

#include <q/midi/ump_processor.hpp>
#include <q/midi/mpe.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

namespace cycfi::q::midi_2_0
{
   // The per-note vocabulary is shared with MPE, so a synth written for one
   // hears the other through the same overloads.
   using midi_1_0::note_pitch;
   using midi_1_0::note_pressure;
   using midi_1_0::note_timbre;

   ////////////////////////////////////////////////////////////////////////////
   // per_note_reader: a processor that wraps a processor, reading MIDI
   // 2.0's per-note messages as note_pitch, note_pressure and note_timbre.
   //
   //    auto chain = midi2::per_note_reader{my_synth};
   //    midi2::dispatch(packet, time, chain);
   //
   // Where MPE spends a channel per note, MIDI 2.0 addresses the note by
   // number: per-note pitch bend, section 4.2.12, poly pressure, 4.2.3,
   // and registered per-note controller 74, brightness, Appendix A, which
   // is the same controller MPE carries timbre on. Channel bend, channel
   // pressure and channel controller 74 still apply to every sounding note
   // on the channel, and combine with the note's own, as in MPE.
   //
   // The bend range is registered controller bank 0 index 0, section
   // 4.2.7, the MIDI 2.0 form of pitch bend sensitivity, and applies to
   // both channel and per-note bend. It defaults to 2 semitones, since the
   // specification names no other default; a MIDI-CI profile may change
   // it, and a keyboard that wants a wide per-note range must say so.
   //
   // Notes pass through untouched, and a note on reports the state its
   // number already holds, since per-note messages may precede it and
   // apply to future notes, section 4.2.5. The state is kept for every
   // note number on every channel, which is plain arrays rather than a
   // pool, at the cost of a few kilobytes per reader.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class per_note_reader
   {
   public:

      static constexpr float  default_range = 2.0f;
      static constexpr float  centre_timbre = 0.5f;
      static constexpr std::uint8_t timbre_controller = 74;

      explicit                per_note_reader(P next)
                               : _next(std::forward<P>(next))
                              {}

                              // Anything not named below is not ours.
                              template <typename Message>
      void                    operator()(Message msg, std::size_t time)
                              {
                                 _next(msg, time);
                              }

      void                    operator()(note_on msg, std::size_t time);
      void                    operator()(note_off msg, std::size_t time);
      void                    operator()(
                                 per_note_pitch_bend msg, std::size_t time);
      void                    operator()(poly_pressure msg, std::size_t time);
      void                    operator()(
                                 registered_per_note_controller msg
                               , std::size_t time);
      void                    operator()(pitch_bend msg, std::size_t time);
      void                    operator()(
                                 channel_pressure msg, std::size_t time);
      void                    operator()(control_change msg, std::size_t time);
      void                    operator()(
                                 registered_controller msg, std::size_t time);

   private:

      struct channel_state
      {
         float          _bend = 0.0f;         // -1 to 1
         float          _pressure = 0.0f;     // 0 to 1
         float          _timbre = centre_timbre;
         float          _range = default_range;
      };

      struct note_state
      {
         bool           _sounding = false;
         float          _bend = 0.0f;
         float          _pressure = 0.0f;
         float          _timbre = centre_timbre;
      };

      static constexpr float  bipolar(std::uint32_t v)
                              {
                                 return (float(v) - 2147483648.0f)
                                    / 2147483648.0f;
                              }
      static constexpr float  unipolar(std::uint32_t v)
                              { return float(v) / 4294967296.0f; }

      void                    send_pitch(
                                 std::uint8_t ch, std::uint8_t key
                               , std::size_t time);
      void                    send_pressure(
                                 std::uint8_t ch, std::uint8_t key
                               , std::size_t time);
      void                    send_timbre(
                                 std::uint8_t ch, std::uint8_t key
                               , std::size_t time);

                              template <typename F>
      void                    for_each_sounding(std::uint8_t ch, F f);

      P                       _next;
      std::array<channel_state, 16>  _channels;
      std::array<std::array<note_state, 128>, 16> _notes;
   };

   template <typename P>
   per_note_reader(P&&) -> per_note_reader<P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   template <typename F>
   inline void per_note_reader<P>::for_each_sounding(std::uint8_t ch, F f)
   {
      for (std::uint8_t key = 0; key != 128; ++key)
         if (_notes[ch][key]._sounding)
            f(key);
   }

   template <typename P>
   inline void per_note_reader<P>::send_pitch(
      std::uint8_t ch, std::uint8_t key, std::size_t time)
   {
      auto const& c = _channels[ch];
      auto const& n = _notes[ch][key];
      _next(note_pitch{ch, key, (n._bend + c._bend) * c._range}, time);
   }

   template <typename P>
   inline void per_note_reader<P>::send_pressure(
      std::uint8_t ch, std::uint8_t key, std::size_t time)
   {
      auto const& c = _channels[ch];
      auto const& n = _notes[ch][key];
      _next(note_pressure{
         ch, key, std::min(1.0f, n._pressure + c._pressure)}, time);
   }

   template <typename P>
   inline void per_note_reader<P>::send_timbre(
      std::uint8_t ch, std::uint8_t key, std::size_t time)
   {
      // The channel's value is an offset from centre, as in MPE, so a
      // channel that says nothing changes nothing.
      auto const& c = _channels[ch];
      auto const& n = _notes[ch][key];
      _next(note_timbre{
         ch, key
       , std::clamp(n._timbre + (c._timbre - centre_timbre), 0.0f, 1.0f)}
       , time);
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(note_on msg, std::size_t time)
   {
      auto const ch = msg.channel();
      auto const key = msg.key();
      _notes[ch][key]._sounding = true;
      _next(msg, time);

      // 4.2.5: what its number already holds is the note's starting state.
      send_pitch(ch, key, time);
      send_pressure(ch, key, time);
      send_timbre(ch, key, time);
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(note_off msg, std::size_t time)
   {
      _notes[msg.channel()][msg.key()]._sounding = false;
      _next(msg, time);
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(
      per_note_pitch_bend msg, std::size_t time)
   {
      auto& n = _notes[msg.channel()][msg.key()];
      n._bend = bipolar(msg.value());
      if (n._sounding)
         send_pitch(msg.channel(), msg.key(), time);
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(
      poly_pressure msg, std::size_t time)
   {
      auto& n = _notes[msg.channel()][msg.key()];
      n._pressure = unipolar(msg.value());
      if (n._sounding)
         send_pressure(msg.channel(), msg.key(), time);
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(
      registered_per_note_controller msg, std::size_t time)
   {
      if (msg.index() != timbre_controller)
      {
         _next(msg, time);
         return;
      }
      auto& n = _notes[msg.channel()][msg.key()];
      n._timbre = unipolar(msg.value());
      if (n._sounding)
         send_timbre(msg.channel(), msg.key(), time);
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(pitch_bend msg, std::size_t time)
   {
      auto const ch = msg.channel();
      _channels[ch]._bend = bipolar(msg.value());
      for_each_sounding(ch,
         [&](std::uint8_t key) { send_pitch(ch, key, time); });
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(
      channel_pressure msg, std::size_t time)
   {
      auto const ch = msg.channel();
      _channels[ch]._pressure = unipolar(msg.value());
      for_each_sounding(ch,
         [&](std::uint8_t key) { send_pressure(ch, key, time); });
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(
      control_change msg, std::size_t time)
   {
      if (msg.controller() != timbre_controller)
      {
         _next(msg, time);
         return;
      }
      auto const ch = msg.channel();
      _channels[ch]._timbre = unipolar(msg.value());
      for_each_sounding(ch,
         [&](std::uint8_t key) { send_timbre(ch, key, time); });
   }

   template <typename P>
   inline void per_note_reader<P>::operator()(
      registered_controller msg, std::size_t time)
   {
      if (msg.bank() != 0 || msg.index() != 0)
      {
         _next(msg, time);
         return;
      }

      // Zero extended from MIDI 1.0's 14 bits, M2-115 4.1: whole semitones
      // in the top seven bits, the fraction of one in the seven below.
      auto const v = msg.value();
      _channels[msg.channel()]._range =
         float(v >> 25) + float((v >> 18) & 0x7F) / 128.0f;
   }
}

#endif
