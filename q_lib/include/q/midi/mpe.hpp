/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_MPE_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_MPE_HPP_SEPTEMBER_9_2026

#include <q/midi/processor.hpp>
#include <array>
#include <cstdint>
#include <utility>

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // The per note messages.
   //
   // MIDI 1.0 has no message that addresses one note of a chord. MPE gets
   // there by agreement instead: a note is given a channel of its own, and
   // that channel's bend, pressure and timbre are the note's own. These are
   // what comes out the other side, each naming the note it belongs to.
   ////////////////////////////////////////////////////////////////////////////
   struct note_expression : message_base
   {
      constexpr note_expression(
         std::uint8_t channel, std::uint8_t key, float value)
       : _channel(channel), _key(key), _value(value)
      {}

      constexpr std::uint8_t     channel() const   { return _channel; }
      constexpr std::uint8_t     key() const       { return _key; }

   protected:

      std::uint8_t   _channel;
      std::uint8_t   _key;
      float          _value;
   };

   // How far this note is bent, in semitones, the zone's own bend added to
   // the note's. Semitones rather than a fraction, because that is what a
   // synth needs to shift a phase iterator.
   struct note_pitch : note_expression
   {
      using note_expression::note_expression;

      constexpr float            semitones() const { return _value; }
   };

   // How hard the key is being held, now, rather than how hard it was
   // struck. 0 to 1.
   struct note_pressure : note_expression
   {
      using note_expression::note_expression;

      constexpr float            value() const     { return _value; }
   };

   // The third dimension, controller 74, which keyboards map to sideways
   // movement along a key. 0 to 1, and it starts centred at 0.5 by
   // convention rather than at zero.
   struct note_timbre : note_expression
   {
      using note_expression::note_expression;

      constexpr float            value() const     { return _value; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // mpe_reader: a processor that wraps a processor, reading a zone's
   // channel messages as messages about single notes.
   //
   //    auto chain = midi::mpe_reader{my_synth};
   //    midi::dispatch(msg, time, chain);
   //
   // Notes pass through untouched: a synth still gets its note on and note
   // off. What changes is expression, which arrives as note_pitch,
   // note_pressure and note_timbre instead of channel messages the synth
   // would have to attribute itself.
   //
   // A zone is declared by registered parameter 6 on its master channel,
   // whose value is how many member channels follow it: upward from channel
   // 1 for the lower zone, downward from channel 16 for the upper one. A
   // count of zero takes the zone away again. Registered parameter 0 sets
   // the bend range, on the master channel for the zone's own bend and on a
   // member channel for the notes'.
   //
   // Until a zone is declared, nothing is per note and every message passes
   // through as it always did. Channels outside a zone keep doing so.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class mpe_reader
   {
   public:

      // What a device sends if it never says otherwise: the specification's
      // defaults, 48 semitones for a note and 2 for the zone.
      static constexpr float  default_member_range = 48.0f;
      static constexpr float  default_master_range = 2.0f;

      explicit                mpe_reader(P next)
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
      void                    operator()(pitch_bend msg, std::size_t time);
      void                    operator()(
                                 channel_aftertouch msg, std::size_t time);
      void                    operator()(
                                 control_change msg, std::size_t time);

      // How many member channels the declared zones hold between them.
      std::uint8_t            zone_members() const;

   private:

      static constexpr std::uint8_t lower_master = 0;
      static constexpr std::uint8_t upper_master = 15;
      static constexpr std::uint8_t timbre_cc = 74;

      struct zone
      {
         std::uint8_t   _members = 0;
         float          _master_range = default_master_range;
         float          _member_range = default_member_range;
         float          _master_bend = 0.0f;     // -1 to 1
      };

      struct voice
      {
         bool           _sounding = false;
         std::uint8_t   _key = 0;
         float          _bend = 0.0f;            // -1 to 1
      };

      zone*                   zone_of(std::uint8_t channel);
      bool                    is_master(std::uint8_t channel) const;
      void                    note_ended(std::uint8_t channel);

      void                    send_pitch(
                                 std::uint8_t channel, std::size_t time);
      void                    send_zone_pitch(
                                 zone const& z, std::size_t time);

                              // The two registered parameters MPE uses. The
                              // rest are none of this stage's business.
      void                    parameter(
                                 std::uint8_t channel, std::uint16_t number
                               , std::uint8_t value);

      P                       _next;
      zone                    _lower;
      zone                    _upper;
      std::array<voice, 16>   _voices;

      // Registered parameter selection, per channel, tracked only far
      // enough to recognise the two that matter.
      std::array<std::uint16_t, 16>  _selected = {};
      std::array<bool, 16>           _has_selection = {};
   };

   ////////////////////////////////////////////////////////////////////////////
   // An lvalue is referred to and a temporary is owned, so a chain can be
   // built in one expression and kept:
   //
   //    auto chain = midi::mpe_reader{my_synth};
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   mpe_reader(P&&) -> mpe_reader<P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   inline bool mpe_reader<P>::is_master(std::uint8_t channel) const
   {
      return (channel == lower_master && _lower._members != 0)
         || (channel == upper_master && _upper._members != 0);
   }

   template <typename P>
   inline auto mpe_reader<P>::zone_of(std::uint8_t channel) -> zone*
   {
      // A lower zone runs up from channel 1, an upper zone down from
      // channel 15. A channel claimed by neither is not MPE's business.
      if (_lower._members != 0
         && channel >= lower_master+1
         && channel <= lower_master + _lower._members)
         return &_lower;

      if (_upper._members != 0
         && channel <= upper_master-1
         && channel >= upper_master - _upper._members)
         return &_upper;

      return nullptr;
   }

   template <typename P>
   inline std::uint8_t mpe_reader<P>::zone_members() const
   {
      return _lower._members + _upper._members;
   }

   template <typename P>
   inline void mpe_reader<P>::note_ended(std::uint8_t channel)
   {
      _voices[channel]._sounding = false;
      _voices[channel]._bend = 0.0f;
   }

   template <typename P>
   inline void mpe_reader<P>::send_pitch(
      std::uint8_t channel, std::size_t time)
   {
      auto const* z = zone_of(channel);
      auto const& v = _voices[channel];
      if (!z || !v._sounding)
         return;

      auto const semitones =
         (v._bend * z->_member_range) + (z->_master_bend * z->_master_range);
      _next(note_pitch{channel, v._key, semitones}, time);
   }

   template <typename P>
   inline void mpe_reader<P>::send_zone_pitch(zone const& z, std::size_t time)
   {
      // A master bend moves every note the zone is holding.
      for (std::uint8_t ch = 0; ch != 16; ++ch)
      {
         if (zone_of(ch) == &z && _voices[ch]._sounding)
            send_pitch(ch, time);
      }
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(note_on msg, std::size_t time)
   {
      auto const channel = msg.channel();
      if (zone_of(channel))
      {
         // Velocity zero is the older way of ending a note, and controllers
         // still send it.
         if (msg.velocity() == 0)
            note_ended(channel);
         else
         {
            _voices[channel]._sounding = true;
            _voices[channel]._key = msg.key();
         }
      }
      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(note_off msg, std::size_t time)
   {
      if (zone_of(msg.channel()))
         note_ended(msg.channel());
      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(pitch_bend msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const bend = (float(msg.value()) - 8192.0f) / 8192.0f;

      if (is_master(channel))
      {
         auto& z = (channel == lower_master)? _lower : _upper;
         z._master_bend = bend;
         send_zone_pitch(z, time);
         return;
      }

      if (zone_of(channel))
      {
         _voices[channel]._bend = bend;
         send_pitch(channel, time);
         return;
      }

      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(
      channel_aftertouch msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const value = float(msg.pressure()) / 127.0f;

      if (is_master(channel))
      {
         auto const& z = (channel == lower_master)? _lower : _upper;
         for (std::uint8_t ch = 0; ch != 16; ++ch)
         {
            if (zone_of(ch) == &z && _voices[ch]._sounding)
               _next(note_pressure{ch, _voices[ch]._key, value}, time);
         }
         return;
      }

      if (zone_of(channel) && _voices[channel]._sounding)
      {
         _next(
            note_pressure{channel, _voices[channel]._key, value}, time);
         return;
      }

      if (zone_of(channel))
         return;                 // in a zone, but no note to attribute it to

      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::parameter(
      std::uint8_t channel, std::uint16_t number, std::uint8_t value)
   {
      if (number == 6)
      {
         // The zone itself. Declaring one clears whatever it was holding.
         if (channel == lower_master)
            _lower = zone{value};
         else if (channel == upper_master)
            _upper = zone{value};

         for (auto& v : _voices)
            v = voice{};
         return;
      }

      if (number == 0)
      {
         // The bend range: the zone's own on a master channel, the notes'
         // on a member channel.
         if (channel == lower_master)
            _lower._master_range = float(value);
         else if (channel == upper_master)
            _upper._master_range = float(value);
         else if (auto* z = zone_of(channel))
            z->_member_range = float(value);
      }
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(
      control_change msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const value = msg.value();

      switch (msg.controller())
      {
         case cc::rpn_msb:
            _selected[channel] =
               (_selected[channel] & 0x7F) | (std::uint16_t(value) << 7);
            _has_selection[channel] = true;
            return;

         case cc::rpn_lsb:
            _selected[channel] =
               (_selected[channel] & 0x3F80) | (value & 0x7F);
            _has_selection[channel] = true;
            return;

         case cc::data_entry:
            if (_has_selection[channel])
            {
               parameter(channel, _selected[channel], value);
               return;
            }
            break;

         case timbre_cc:
            if (is_master(channel))
            {
               auto const& z = (channel == lower_master)? _lower : _upper;
               auto const timbre = float(value) / 127.0f;
               for (std::uint8_t ch = 0; ch != 16; ++ch)
               {
                  if (zone_of(ch) == &z && _voices[ch]._sounding)
                     _next(
                        note_timbre{ch, _voices[ch]._key, timbre}, time);
               }
               return;
            }
            if (zone_of(channel))
            {
               if (_voices[channel]._sounding)
               {
                  _next(
                     note_timbre{
                        channel, _voices[channel]._key
                      , float(value) / 127.0f}
                   , time);
               }
               return;
            }
            break;

         default:
            break;
      }

      _next(msg, time);
   }
}

#endif
