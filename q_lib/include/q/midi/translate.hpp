/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_TRANSLATE_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_TRANSLATE_HPP_SEPTEMBER_9_2026

#include <q/midi/ump_processor.hpp>
#include <q/midi/scaling.hpp>
#include <array>
#include <cstdint>
#include <utility>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // Translation between the two protocols. M2-104-UM Appendix D, the
   // Default Translation Mode, with values scaled per M2-115-U.
   //
   // Both stages are processor proxies, like the readers:
   // one takes MIDI 2.0 messages and hands the processor behind it MIDI 1.0
   // ones, the other the reverse. A processor written for either protocol
   // can therefore be fed by either kind of stream.
   //
   //    auto chain = midi2::to_midi1{my_midi1_synth};
   //    midi2::dispatch(packet, time, chain);
   //
   //    auto chain = midi2::to_midi2{my_midi2_synth};
   //    midi::dispatch(raw, time, chain);
   //
   // Translated packets are addressed to group 0: a MIDI 1.0 stream has no
   // groups to carry over.
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // to_midi1: MIDI 2.0 in, MIDI 1.0 out. Appendix D.2.
   //
   // What has no MIDI 1.0 form is dropped, per D.2.8: the relative
   // controllers, the per-note controllers, per-note management and
   // per-note pitch bend. Anything else, a system message or a MIDI 1.0
   // voice message the stream carried as it was, passes through.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class to_midi1
   {
   public:

      explicit                to_midi1(P next)
                               : _next(std::forward<P>(next))
                              {}

                              template <typename Message>
      void                    operator()(Message msg, std::size_t time)
                              {
                                 _next(msg, time);
                              }

      void                    operator()(note_on msg, std::size_t time);
      void                    operator()(note_off msg, std::size_t time);
      void                    operator()(poly_pressure msg, std::size_t time);
      void                    operator()(control_change msg, std::size_t time);
      void                    operator()(
                                 registered_controller msg, std::size_t time);
      void                    operator()(
                                 assignable_controller msg, std::size_t time);
      void                    operator()(program_change msg, std::size_t time);
      void                    operator()(
                                 channel_pressure msg, std::size_t time);
      void                    operator()(pitch_bend msg, std::size_t time);

                              // D.2.8: no equivalent, so nothing is sent.
      void                    operator()(
                                 relative_registered_controller, std::size_t) {}
      void                    operator()(
                                 relative_assignable_controller, std::size_t) {}
      void                    operator()(
                                 registered_per_note_controller, std::size_t) {}
      void                    operator()(
                                 assignable_per_note_controller, std::size_t) {}
      void                    operator()(per_note_management, std::size_t) {}
      void                    operator()(per_note_pitch_bend, std::size_t) {}

   private:

      void                    cc(std::uint8_t channel, std::uint8_t ctrl
                               , std::uint8_t value, std::size_t time);

      P                       _next;
   };

   template <typename P>
   to_midi1(P&&) -> to_midi1<P>;

   ////////////////////////////////////////////////////////////////////////////
   // to_midi2: MIDI 1.0 in, MIDI 2.0 out. Appendix D.3.
   //
   // Two things span more than one message and are held until complete.
   // A registered or assignable parameter is four controllers, and D.3.3
   // sends it when the fine data entry arrives, when a second coarse one
   // does, or when a new parameter is selected. Bank select is two
   // controllers that mean nothing alone and ride along with the next
   // program change, D.3.4.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class to_midi2
   {
   public:

      // A MIDI 1.0 stream has no groups, so everything translated out of
      // one goes to group 0.
      static constexpr std::uint8_t group = 0;

      explicit                to_midi2(P next)
                               : _next(std::forward<P>(next))
                              {}

                              template <typename Message>
      void                    operator()(Message msg, std::size_t time)
                              {
                                 _next(msg, time);
                              }

      void                    operator()(
                                 midi_1_0::note_on msg, std::size_t time);
      void                    operator()(
                                 midi_1_0::note_off msg, std::size_t time);
      void                    operator()(
                                 midi_1_0::poly_pressure msg
                               , std::size_t time);
      void                    operator()(
                                 midi_1_0::control_change msg
                               , std::size_t time);
      void                    operator()(
                                 midi_1_0::program_change msg
                               , std::size_t time);
      void                    operator()(
                                 midi_1_0::channel_pressure msg
                               , std::size_t time);
      void                    operator()(
                                 midi_1_0::pitch_bend msg, std::size_t time);

   private:

      void                    send_parameter(
                                 std::uint8_t channel, std::uint8_t lsb
                               , std::size_t time);

      struct state
      {
         bool           _registered = false;
         bool           _selected = false;
         std::uint8_t   _bank = 0;
         std::uint8_t   _index = 0;
         std::uint8_t   _data_msb = 0;
         bool           _data_held = false;

         bool           _bank_known = false;
         std::uint8_t   _bank_msb = 0;
         std::uint8_t   _bank_lsb = 0;
      };

      P                       _next;
      std::array<state, 16>   _channels;
   };

   template <typename P>
   to_midi2(P&&) -> to_midi2<P>;

   ////////////////////////////////////////////////////////////////////////////
   // to_midi1 implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   inline void to_midi1<P>::cc(
      std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value
    , std::size_t time)
   {
      _next(midi_1_0::control_change{
         channel, midi_1_0::cc::controller(ctrl), value}, time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(note_on msg, std::size_t time)
   {
      // D.2.1: a velocity that scales to zero would read as a note off, so
      // it becomes one.
      auto velocity = std::uint8_t(scale_down(msg.velocity(), 16, 7));
      if (velocity == 0)
         velocity = 1;
      _next(midi_1_0::note_on{msg.channel(), msg.key(), velocity}, time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(note_off msg, std::size_t time)
   {
      auto const velocity = std::uint8_t(scale_down(msg.velocity(), 16, 7));
      _next(midi_1_0::note_off{msg.channel(), msg.key(), velocity}, time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(poly_pressure msg, std::size_t time)
   {
      auto const value = std::uint8_t(scale_down(msg.value(), 32, 7));
      _next(midi_1_0::poly_pressure{msg.channel(), msg.key(), value}, time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(control_change msg, std::size_t time)
   {
      cc(msg.channel(), msg.controller()
       , std::uint8_t(scale_down(msg.value(), 32, 7)), time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(
      registered_controller msg, std::size_t time)
   {
      // D.2.3: one message becomes four. The value narrows to 14 bits, by
      // zero extension for the indexes that are counts, M2-115 4.1.
      auto const value = registered_uses_zero_extension(msg.index())
         ? zero_extend_down(msg.value(), 32, 14)
         : scale_down(msg.value(), 32, 14);

      cc(msg.channel(), 101, msg.bank(), time);
      cc(msg.channel(), 100, msg.index(), time);
      cc(msg.channel(), 6, std::uint8_t(value >> 7), time);
      cc(msg.channel(), 38, std::uint8_t(value & 0x7F), time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(
      assignable_controller msg, std::size_t time)
   {
      auto const value = scale_down(msg.value(), 32, 14);

      cc(msg.channel(), 99, msg.bank(), time);
      cc(msg.channel(), 98, msg.index(), time);
      cc(msg.channel(), 6, std::uint8_t(value >> 7), time);
      cc(msg.channel(), 38, std::uint8_t(value & 0x7F), time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(program_change msg, std::size_t time)
   {
      // D.2.4: bank first, if there is one, in the order MIDI 1.0 expects.
      if (msg.bank_valid())
      {
         cc(msg.channel(), 0, msg.bank_msb(), time);
         cc(msg.channel(), 32, msg.bank_lsb(), time);
      }
      _next(midi_1_0::program_change{msg.channel(), msg.program()}, time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(channel_pressure msg, std::size_t time)
   {
      auto const value = std::uint8_t(scale_down(msg.value(), 32, 7));
      _next(midi_1_0::channel_pressure{msg.channel(), value}, time);
   }

   template <typename P>
   inline void to_midi1<P>::operator()(pitch_bend msg, std::size_t time)
   {
      auto const value = std::uint16_t(scale_down(msg.value(), 32, 14));
      _next(midi_1_0::pitch_bend{msg.channel(), value}, time);
   }

   ////////////////////////////////////////////////////////////////////////////
   // to_midi2 implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::note_on msg, std::size_t time)
   {
      // D.3.1: velocity zero is a note off, and says so in MIDI 2.0. The
      // attribute is zero, absent a profile that says otherwise.
      if (msg.velocity() == 0)
      {
         _next(note_off{group, msg.channel(), msg.key(), 0}, time);
         return;
      }

      auto const velocity = scale_up(msg.velocity(), 7, 16);
      _next(note_on{
         group, msg.channel(), msg.key(), std::uint16_t(velocity)}, time);
   }

   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::note_off msg, std::size_t time)
   {
      auto const velocity = scale_up(msg.velocity(), 7, 16);
      _next(note_off{
         group, msg.channel(), msg.key(), std::uint16_t(velocity)}, time);
   }

   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::poly_pressure msg, std::size_t time)
   {
      _next(poly_pressure{group, msg.channel(), msg.key()
                        , scale_up(msg.pressure(), 7, 32)}, time);
   }

   // D.3.3: send the parameter a channel is holding, if it is holding one,
   // with lsb for its fine half. Nothing is sent when no parameter is
   // selected, when no coarse data entry has arrived, or when the
   // selection is the null function.
   template <typename P>
   inline void to_midi2<P>::send_parameter(
      std::uint8_t channel, std::uint8_t lsb, std::size_t time)
   {
      auto& st = _channels[channel];
      if (!st._selected || !st._data_held)
         return;
      st._data_held = false;

      auto const v14 = (std::uint32_t(st._data_msb) << 7) | lsb;
      if (st._registered)
      {
         auto const v32 = registered_uses_zero_extension(st._index)
            ? zero_extend_up(v14, 14, 32)
            : scale_up(v14, 14, 32);
         _next(registered_controller{
            group, channel, st._bank, st._index, v32}, time);
      }
      else
      {
         _next(assignable_controller{
            group, channel, st._bank, st._index
          , scale_up(v14, 14, 32)}, time);
      }
   }

   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::control_change msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const value = msg.value();
      auto& st = _channels[channel];

      switch (msg.controller())
      {
         // D.3.3: the halves of a parameter are held, and one of three
         // things sends it. A new selection is the first: it says the
         // parameter before it has ended, so that one goes now, with a
         // zero for the fine half that never came.
         case midi_1_0::cc::rpn_msb:
         case midi_1_0::cc::rpn_lsb:
         case midi_1_0::cc::nrpn_msb:
         case midi_1_0::cc::nrpn_lsb:
         {
            send_parameter(channel, 0, time);
            auto const registered =
               msg.controller() == midi_1_0::cc::rpn_msb ||
               msg.controller() == midi_1_0::cc::rpn_lsb;
            auto const is_msb =
               msg.controller() == midi_1_0::cc::rpn_msb ||
               msg.controller() == midi_1_0::cc::nrpn_msb;
            st._registered = registered;
            st._selected = true;
            (is_msb? st._bank : st._index) = value;

            // The null function, both halves 0x7F, selects nothing and is
            // not translated.
            if (st._bank == 0x7F && st._index == 0x7F)
               st._selected = false;
            return;
         }

         // A second coarse data entry is the next: the one before it goes
         // first, then this one is held in its place.
         case midi_1_0::cc::data_entry:
            send_parameter(channel, 0, time);
            st._data_msb = value;
            st._data_held = true;
            return;

         // And the fine half is the last, which completes the value.
         case midi_1_0::cc::data_entry_lsb:
            if (st._data_held)
               send_parameter(channel, value, time);
            return;

         // D.3.3, D.3.4: bank select waits for its program change.
         case midi_1_0::cc::bank_select:
            st._bank_msb = value; st._bank_known = true;
            return;
         case midi_1_0::cc::bank_select_lsb:
            st._bank_lsb = value; st._bank_known = true;
            return;

         default:
            break;
      }

      // Everything else, increment and decrement included, is a control
      // change of its own number.
      _next(control_change{group, channel, std::uint8_t(msg.controller())
                         , scale_up(value, 7, 32)}, time);
   }

   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::program_change msg, std::size_t time)
   {
      auto const& st = _channels[msg.channel()];
      if (st._bank_known)
      {
         _next(program_change{group, msg.channel(), msg.preset()
                            , st._bank_msb, st._bank_lsb}, time);
      }
      else
      {
         _next(program_change{group, msg.channel(), msg.preset()}, time);
      }
   }

   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::channel_pressure msg, std::size_t time)
   {
      _next(channel_pressure{
         group, msg.channel(), scale_up(msg.pressure(), 7, 32)}, time);
   }

   template <typename P>
   inline void to_midi2<P>::operator()(
      midi_1_0::pitch_bend msg, std::size_t time)
   {
      _next(pitch_bend{
         group, msg.channel(), scale_up(msg.value(), 14, 32)}, time);
   }
}

#endif
