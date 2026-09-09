/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_CONTROLLERS_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_CONTROLLERS_HPP_SEPTEMBER_9_2026

#include <q/midi/processor.hpp>
#include <array>
#include <cstdint>
#include <utility>

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // control_change_14: a controller carrying a 14 bit value.
   //
   // Controllers 0 to 31 have a partner 32 higher holding the fine half of
   // the same value. The pair is one control, and this is what it reads as:
   // the coarse controller's number, and the value both halves make.
   ////////////////////////////////////////////////////////////////////////////
   struct control_change_14 : message_base
   {
      constexpr control_change_14(
         std::uint8_t channel, cc::controller ctrl, std::uint16_t value)
       : _channel(channel), _controller(ctrl), _value(value)
      {}

      constexpr std::uint8_t     channel() const      { return _channel; }
      constexpr cc::controller   controller() const   { return _controller; }
      constexpr std::uint16_t    value() const        { return _value; }

   private:

      std::uint8_t      _channel;
      cc::controller    _controller;
      std::uint16_t     _value;
   };

   ////////////////////////////////////////////////////////////////////////////
   // cc14_reader: a processor that wraps a processor, joining the two
   // halves of the controllers that have them.
   //
   //    auto chain = midi::rpn_reader{midi::cc14_reader{my_synth}};
   //    midi::dispatch(msg, time, chain);
   //
   // Controllers 0 to 63 reach the wrapped processor as control_change_14
   // and no longer as control_change; everything else, 64 and up included,
   // is untouched. Using this stage is a choice, so a processor that wants
   // plain seven bit controllers simply leaves it out.
   //
   // It belongs inside rpn_reader, which takes the data entry controllers
   // first: those are a coarse and fine pair too, but they mean a
   // parameter's value rather than a control of their own.
   //
   // Each half reports as it arrives, rather than the coarse half waiting
   // for a partner that in most cases never comes. A controller sending both
   // therefore reports twice, once coarse and then refined. That is the same
   // shape a moved fader gives, and it needs no clock, which a library that
   // may run inside an audio callback cannot have.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class cc14_reader
   {
   public:

      static constexpr std::uint8_t pairs = 32;

      explicit                cc14_reader(P next)
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

      P                       _next;

      // The coarse half last seen, per controller, per channel.
      std::array<std::array<std::uint8_t, pairs>, 16> _coarse = {};
   };

   ////////////////////////////////////////////////////////////////////////////
   // An lvalue is referred to and a temporary is owned, so a chain can be
   // built in one expression and kept:
   //
   //    auto chain = midi::rpn_reader{midi::cc14_reader{my_synth}};
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   cc14_reader(P&&) -> cc14_reader<P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   inline void cc14_reader<P>::operator()(
      control_change msg, std::size_t time)
   {
      auto const number = std::uint8_t(msg.controller());
      if (number >= pairs*2)
      {
         _next(msg, time);
         return;
      }

      auto const channel = msg.channel();
      auto const value = msg.value() & 0x7F;
      auto& coarse = _coarse[channel];

      std::uint16_t wide;
      std::uint8_t ctrl;
      if (number < pairs)
      {
         // A coarse half replaces the value outright: the fine half it was
         // paired with described the old position, not this one.
         ctrl = number;
         coarse[ctrl] = value;
         wide = std::uint16_t(value) << 7;
      }
      else
      {
         ctrl = number - pairs;
         wide = (std::uint16_t(coarse[ctrl]) << 7) | value;
      }

      _next(control_change_14{channel, cc::controller(ctrl), wide}, time);
   }
}

#endif
