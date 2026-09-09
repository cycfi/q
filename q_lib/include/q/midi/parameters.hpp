/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PARAMETERS_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_PARAMETERS_HPP_SEPTEMBER_9_2026

#include <q/midi/processor.hpp>
#include <array>
#include <cstdint>
#include <utility>

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // rpn and nrpn: a parameter addressed by a 14 bit number, holding a 14 bit
   // value.
   //
   // Neither is a message on the wire. Six controllers spell them out: four
   // naming the parameter, two setting it. rpn_reader assembles them, so a
   // processor reads a whole parameter instead of the halves.
   //
   // A registered parameter (rpn) has a meaning the specification assigns,
   // pitch bend sensitivity being number 0. An unregistered one (nrpn) means
   // whatever the instrument says it means.
   ////////////////////////////////////////////////////////////////////////////
   struct parameter_message : message_base
   {
      constexpr parameter_message(
         std::uint8_t channel, std::uint16_t number, std::uint16_t value)
       : _channel(channel), _number(number), _value(value)
      {}

      constexpr std::uint8_t     channel() const   { return _channel; }
      constexpr std::uint16_t    number() const    { return _number; }
      constexpr std::uint16_t    value() const     { return _value; }

   private:

      std::uint8_t   _channel;
      std::uint16_t  _number;
      std::uint16_t  _value;
   };

   struct rpn : parameter_message
   {
      using parameter_message::parameter_message;
   };

   struct nrpn : parameter_message
   {
      using parameter_message::parameter_message;
   };

   ////////////////////////////////////////////////////////////////////////////
   // rpn_reader: a processor that wraps a processor. It takes the six
   // controllers that make up a parameter, hands the assembled parameter to
   // the processor it wraps, and passes everything else along untouched.
   //
   //    auto chain = midi::rpn_reader{my_synth};
   //    midi::dispatch(msg, time, chain);
   //
   // Stages nest, so this one can sit in front of another. It belongs
   // outside cc14_reader, because the data entry controllers, 6 and 38, are
   // also a coarse and fine pair, and here they mean this parameter's value.
   //
   // The selection is per channel and survives until it is replaced or
   // ended: a controller names a parameter once, then sends data entries for
   // as long as it sweeps it.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class rpn_reader
   {
   public:

      static constexpr std::uint16_t null_number = 0x3FFF;
      static constexpr std::uint16_t max_value = 0x3FFF;

      explicit                rpn_reader(P next)
                               : _next(std::forward<P>(next))
                              {}

                              // Anything that is not a control change is not
                              // ours to read.
                              template <typename Message>
      void                    operator()(Message msg, std::size_t time)
                              {
                                 _next(msg, time);
                              }

      void                    operator()(
                                 control_change msg, std::size_t time);

   private:

      enum class kind : std::uint8_t { none, registered, assignable };

      struct state
      {
         kind           _kind = kind::none;
         std::uint16_t  _number = 0;
         std::uint16_t  _value = 0;
      };

      void                    select(
                                 state& st, kind k, bool msb
                               , std::uint8_t half);

      void                    emit(
                                 state const& st, std::uint8_t channel
                               , std::size_t time);

      P                       _next;
      std::array<state, 16>   _channels;
   };

   ////////////////////////////////////////////////////////////////////////////
   // An lvalue is referred to and a temporary is owned, so a chain can be
   // built in one expression and kept:
   //
   //    auto chain = midi::rpn_reader{my_synth};
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   rpn_reader(P&&) -> rpn_reader<P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   inline void rpn_reader<P>::select(
      state& st, kind k, bool msb, std::uint8_t half)
   {
      // A number arrives in halves, and the two need not both be sent: a
      // device selecting parameter 0 often sends only the half that changed.
      if (st._kind != k)
      {
         st._kind = k;
         st._number = 0;
         st._value = 0;
      }

      if (msb)
         st._number = (st._number & 0x7F) | (std::uint16_t(half & 0x7F) << 7);
      else
         st._number = (st._number & 0x3F80) | (half & 0x7F);

      // 127/127 is the specification's way of saying "no parameter", so a
      // data entry that follows a finished gesture lands nowhere.
      if (st._number == null_number)
         st._kind = kind::none;
   }

   template <typename P>
   inline void rpn_reader<P>::emit(
      state const& st, std::uint8_t channel, std::size_t time)
   {
      if (st._kind == kind::registered)
         _next(rpn{channel, st._number, st._value}, time);
      else if (st._kind == kind::assignable)
         _next(nrpn{channel, st._number, st._value}, time);
   }

   template <typename P>
   inline void rpn_reader<P>::operator()(
      control_change msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const value = msg.value();
      auto& st = _channels[channel];

      switch (msg.controller())
      {
         case cc::rpn_msb:
            select(st, kind::registered, true, value);
            return;

         case cc::rpn_lsb:
            select(st, kind::registered, false, value);
            return;

         case cc::nonrpn_msb:
            select(st, kind::assignable, true, value);
            return;

         case cc::nonrpn_lsb:
            select(st, kind::assignable, false, value);
            return;

         case cc::data_entry:
            // The coarse half also clears the fine one: a device that sends
            // only this half means the value it names, not that value plus
            // whatever was left behind.
            if (st._kind != kind::none)
            {
               st._value = std::uint16_t(value & 0x7F) << 7;
               emit(st, channel, time);
            }
            return;

         case cc::data_entry_lsb:
            if (st._kind != kind::none)
            {
               st._value = (st._value & 0x3F80) | (value & 0x7F);
               emit(st, channel, time);
            }
            return;

         case cc::data_inc:
            if (st._kind != kind::none)
            {
               if (st._value < max_value)
                  ++st._value;
               emit(st, channel, time);
            }
            return;

         case cc::data_dec:
            if (st._kind != kind::none)
            {
               if (st._value > 0)
                  --st._value;
               emit(st, channel, time);
            }
            return;

         default:
            _next(msg, time);
            return;
      }
   }
}

#endif
