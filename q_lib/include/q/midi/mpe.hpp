/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_MPE_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_MPE_HPP_SEPTEMBER_9_2026

#include <q/midi/processor.hpp>
#include <algorithm>
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
   // the note's, each scaled by its own range.
   struct note_pitch : note_expression
   {
      using note_expression::note_expression;

      constexpr float            semitones() const { return _value; }
   };

   // How hard the key is being held now, rather than how hard it was
   // struck. 0 to 1.
   struct note_pressure : note_expression
   {
      using note_expression::note_expression;

      constexpr float            value() const     { return _value; }
   };

   // The third dimension, controller 74. 0 to 1, starting centred.
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
   // Notes pass through untouched, so a synth still gets its note on and
   // note off. Expression arrives as note_pitch, note_pressure and
   // note_timbre instead of channel messages the synth would have to
   // attribute itself, and a note is given its channel's current values the
   // moment it starts.
   //
   // Follows MIDI Polyphonic Expression 1.0 (MMA/AMEI RP-053). Until a zone
   // is declared nothing is per note, and channels outside a zone always
   // pass through, so a plain keyboard plays as it always did.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class mpe_reader
   {
   public:

      // 2.4 and 2.5: what a configuration message sets the ranges to.
      static constexpr float  default_member_range = 48.0f;
      static constexpr float  default_master_range = 2.0f;

      // 3.3.5: controller 74 starts at 0x40 so movement can go either way.
      static constexpr float  centre_timbre = 64.0f/127.0f;

      // 2.2.1: a channel holds more than one note once the zone runs out of
      // channels, and expression then reaches all of them.
      static constexpr std::size_t max_notes_per_channel = 8;

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
                                 poly_aftertouch msg, std::size_t time);
      void                    operator()(
                                 program_change msg, std::size_t time);
      void                    operator()(
                                 control_change msg, std::size_t time);

      std::uint8_t            lower_members() const { return _lower._members; }
      std::uint8_t            upper_members() const { return _upper._members; }
      std::uint8_t            zone_members() const;

   private:

      static constexpr std::uint8_t lower_master = 0;
      static constexpr std::uint8_t upper_master = 15;
      static constexpr std::uint8_t timbre_cc = 74;
      static constexpr std::uint8_t max_members = 15;

      struct zone
      {
         std::uint8_t   _members = 0;
         float          _master_range = default_master_range;
         float          _member_range = default_member_range;
         float          _master_bend = 0.0f;        // -1 to 1
         float          _master_pressure = 0.0f;    // 0 to 1
         float          _master_timbre = centre_timbre;
      };

      // 3.3: a channel's values are kept even with nothing sounding, since
      // they are the initial state of the next note to start on it.
      struct channel_state
      {
         float          _bend = 0.0f;
         float          _pressure = 0.0f;
         float          _timbre = centre_timbre;
         std::array<std::uint8_t, max_notes_per_channel> _keys = {};
         std::uint8_t   _count = 0;
      };

      zone const*             zone_of(std::uint8_t channel) const;
      zone*                   zone_of(std::uint8_t channel);
      bool                    is_master(std::uint8_t channel) const;
      zone&                   master_zone(std::uint8_t channel);

      void                    configure(std::uint8_t channel, std::uint8_t n);
      void                    resolve_overlap(bool lower_is_newer);
      void                    stop_all(std::size_t time);

      void                    send_pitch(
                                 std::uint8_t channel, std::uint8_t key
                               , std::size_t time);
      void                    send_pressure(
                                 std::uint8_t channel, std::uint8_t key
                               , std::size_t time);
      void                    send_timbre(
                                 std::uint8_t channel, std::uint8_t key
                               , std::size_t time);

                              // Apply one of the three to every note a zone
                              // is holding, which is what a master channel
                              // message means.
                              template <typename F>
      void                    for_each_note(zone const& z, F f);

      void                    parameter(
                                 std::uint8_t channel, std::uint16_t number
                               , std::uint8_t value);

      P                       _next;
      zone                    _lower;
      zone                    _upper;
      std::array<channel_state, 16>  _channels;

      // Registered parameter selection, tracked far enough to recognise the
      // two MPE uses: 0 for the bend range, 6 for the zone itself.
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
   inline auto mpe_reader<P>::master_zone(std::uint8_t channel) -> zone&
   {
      return (channel == lower_master)? _lower : _upper;
   }

   template <typename P>
   inline auto mpe_reader<P>::zone_of(std::uint8_t channel) const -> zone const*
   {
      // 2.1.1: the lower zone runs up from channel 2, the upper zone down
      // from channel 15, and either may take the other's master channel
      // when that zone is unused.
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
   inline auto mpe_reader<P>::zone_of(std::uint8_t channel) -> zone*
   {
      auto const* z = const_cast<mpe_reader const*>(this)->zone_of(channel);
      return const_cast<zone*>(z);
   }

   template <typename P>
   inline std::uint8_t mpe_reader<P>::zone_members() const
   {
      return _lower._members + _upper._members;
   }

   template <typename P>
   template <typename F>
   inline void mpe_reader<P>::for_each_note(zone const& z, F f)
   {
      for (std::uint8_t ch = 0; ch != 16; ++ch)
      {
         if (zone_of(ch) != &z)
            continue;
         auto const& state = _channels[ch];
         for (std::uint8_t i = 0; i != state._count; ++i)
            f(ch, state._keys[i]);
      }
   }

   template <typename P>
   inline void mpe_reader<P>::send_pitch(
      std::uint8_t channel, std::uint8_t key, std::size_t time)
   {
      auto const* z = zone_of(channel);
      if (!z)
         return;

      // 2.4: the note's own bend and the zone's, each by its own range.
      auto const semitones =
         (_channels[channel]._bend * z->_member_range)
       + (z->_master_bend * z->_master_range);
      _next(note_pitch{channel, key, semitones}, time);
   }

   template <typename P>
   inline void mpe_reader<P>::send_pressure(
      std::uint8_t channel, std::uint8_t key, std::size_t time)
   {
      auto const* z = zone_of(channel);
      if (!z)
         return;

      auto const value = std::min(
         1.0f, _channels[channel]._pressure + z->_master_pressure);
      _next(note_pressure{channel, key, value}, time);
   }

   template <typename P>
   inline void mpe_reader<P>::send_timbre(
      std::uint8_t channel, std::uint8_t key, std::size_t time)
   {
      auto const* z = zone_of(channel);
      if (!z)
         return;

      // Timbre rests at centre, so the zone's value is an offset from it
      // rather than a level to add.
      auto const value = std::clamp(
         _channels[channel]._timbre + (z->_master_timbre - centre_timbre)
       , 0.0f, 1.0f);
      _next(note_timbre{channel, key, value}, time);
   }

   template <typename P>
   inline void mpe_reader<P>::stop_all(std::size_t time)
   {
      // 2.1.4: a receiver stops every ongoing note when a zone changes, so
      // a reconfiguration cannot leave notes hanging.
      for (std::uint8_t ch = 0; ch != 16; ++ch)
      {
         auto& state = _channels[ch];
         for (std::uint8_t i = 0; i != state._count; ++i)
            _next(note_off{ch, state._keys[i], 0}, time);
         state = channel_state{};
      }
   }

   template <typename P>
   inline void mpe_reader<P>::resolve_overlap(bool lower_is_newer)
   {
      // 2.1.1: a channel cannot belong to two zones, and the newer message
      // takes the channels it asked for, even if that leaves the other zone
      // with none.
      if (lower_is_newer)
      {
         if (_upper._members == 0)
            return;
         auto const room =
            (_lower._members >= max_members)? 0 : 14 - _lower._members;
         if (room <= 0)
            _upper._members = 0;
         else
            _upper._members = std::min<int>(_upper._members, room);
      }
      else
      {
         if (_lower._members == 0)
            return;
         auto const room =
            (_upper._members >= max_members)? 0 : 14 - _upper._members;
         if (room <= 0)
            _lower._members = 0;
         else
            _lower._members = std::min<int>(_lower._members, room);
      }
   }

   template <typename P>
   inline void mpe_reader<P>::configure(
      std::uint8_t channel, std::uint8_t members)
   {
      // 2.1.1: only the two master channels may carry a configuration
      // message, and a count above fifteen is not a count.
      if (channel != lower_master && channel != upper_master)
         return;
      if (members > max_members)
         return;

      auto const lower = (channel == lower_master);
      auto& z = lower? _lower : _upper;

      // 2.4, 2.5: the ranges go back to their defaults with every
      // configuration message.
      z = zone{members};
      resolve_overlap(lower);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(note_on msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto* z = zone_of(channel);
      if (!z)
      {
         _next(msg, time);
         return;
      }

      auto& state = _channels[channel];
      if (msg.velocity() == 0)
      {
         // The older way of ending a note, which controllers still send.
         for (std::uint8_t i = 0; i != state._count; ++i)
         {
            if (state._keys[i] == msg.key())
            {
               state._keys[i] = state._keys[state._count-1];
               --state._count;
               break;
            }
         }
         _next(msg, time);
         return;
      }

      if (state._count < max_notes_per_channel)
         state._keys[state._count++] = msg.key();

      _next(msg, time);

      // 3.3: the note starts from whatever its channel currently holds.
      send_pitch(channel, msg.key(), time);
      send_pressure(channel, msg.key(), time);
      send_timbre(channel, msg.key(), time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(note_off msg, std::size_t time)
   {
      auto const channel = msg.channel();
      if (zone_of(channel))
      {
         auto& state = _channels[channel];
         for (std::uint8_t i = 0; i != state._count; ++i)
         {
            if (state._keys[i] == msg.key())
            {
               state._keys[i] = state._keys[state._count-1];
               --state._count;
               break;
            }
         }
      }
      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(pitch_bend msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const bend = (float(msg.value()) - 8192.0f) / 8192.0f;

      if (is_master(channel))
      {
         auto& z = master_zone(channel);
         z._master_bend = bend;
         for_each_note(z,
            [&](std::uint8_t ch, std::uint8_t key)
            { send_pitch(ch, key, time); });
         return;
      }

      if (zone_of(channel))
      {
         auto& state = _channels[channel];
         state._bend = bend;
         for (std::uint8_t i = 0; i != state._count; ++i)
            send_pitch(channel, state._keys[i], time);
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
         auto& z = master_zone(channel);
         z._master_pressure = value;
         for_each_note(z,
            [&](std::uint8_t ch, std::uint8_t key)
            { send_pressure(ch, key, time); });
         return;
      }

      if (zone_of(channel))
      {
         auto& state = _channels[channel];
         state._pressure = value;
         for (std::uint8_t i = 0; i != state._count; ++i)
            send_pressure(channel, state._keys[i], time);
         return;
      }

      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(
      poly_aftertouch msg, std::size_t time)
   {
      // 2.5: polyphonic key pressure must not be sent on a member channel,
      // and is reserved. On a master channel it is allowed, and passes
      // through as it stands.
      if (zone_of(msg.channel()) && !is_master(msg.channel()))
         return;
      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::operator()(
      program_change msg, std::size_t time)
   {
      // 2.3.3: in mode 3, which is MPE's usual mode, a program change on a
      // member channel is ignored.
      if (zone_of(msg.channel()) && !is_master(msg.channel()))
         return;
      _next(msg, time);
   }

   template <typename P>
   inline void mpe_reader<P>::parameter(
      std::uint8_t channel, std::uint16_t number, std::uint8_t value)
   {
      if (number == 6)
      {
         configure(channel, value);
         return;
      }

      if (number == 0)
      {
         // 2.4: the zone's own range on a master channel, the notes' range
         // on any member channel, which then applies to all of them.
         if (is_master(channel))
            master_zone(channel)._master_range = float(value);
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
      auto const controller = msg.controller();

      switch (controller)
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
               auto const number = _selected[channel];
               auto const was_zone = (number == 6);
               if (was_zone)
                  stop_all(time);
               parameter(channel, number, value);
               return;
            }
            break;

         case timbre_cc:
            if (is_master(channel))
            {
               auto& z = master_zone(channel);
               z._master_timbre = float(value) / 127.0f;
               for_each_note(z,
                  [&](std::uint8_t ch, std::uint8_t key)
                  { send_timbre(ch, key, time); });
               return;
            }
            if (zone_of(channel))
            {
               auto& state = _channels[channel];
               state._timbre = float(value) / 127.0f;
               for (std::uint8_t i = 0; i != state._count; ++i)
                  send_timbre(channel, state._keys[i], time);
               return;
            }
            break;

         default:
            // 2.3.1: the rest are zone messages. On a member channel they
            // are ignored; on a master channel, or outside any zone, they
            // pass through.
            if (zone_of(channel) && !is_master(channel))
               return;
            break;
      }

      _next(msg, time);
   }
}

#endif
