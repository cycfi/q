/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_MODES_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_MODES_HPP_SEPTEMBER_9_2026

#include <q/midi/processor.hpp>
#include <cstdint>
#include <utility>

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // The channel mode messages.
   //
   // Controllers 120 and up are not controls at all. They tell an instrument
   // how to behave: stop sounding, forget every controller position, answer
   // on one channel or all of them, play one note at a time or many. They
   // are controllers only because MIDI had no room left for new status
   // bytes.
   //
   // Each is its own type here, so a synth answers the ones it cares about
   // and ignores the rest, rather than switching on a controller number.
   ////////////////////////////////////////////////////////////////////////////
   struct channel_mode : message_base
   {
      constexpr channel_mode(std::uint8_t channel)
       : _channel(channel)
      {}

      constexpr std::uint8_t     channel() const   { return _channel; }

   private:

      std::uint8_t   _channel;
   };

   // Silence every voice at once, envelopes and all.
   struct all_sounds_off : channel_mode
   {
      using channel_mode::channel_mode;
   };

   // Every controller back to its default: wheels centred, pedals up.
   struct reset_all_controllers : channel_mode
   {
      using channel_mode::channel_mode;
   };

   // Whether the instrument's own keyboard still plays its own voices, or
   // only sends. 0 is off and 127 is on; nothing is defined between.
   struct local_control : channel_mode
   {
      constexpr local_control(std::uint8_t channel, bool on)
       : channel_mode(channel), _on(on)
      {}

      constexpr bool             on() const        { return _on; }

   private:

      bool  _on;
   };

   // Release every sounding note, as a note off would. Voices still in their
   // release stage keep ringing, which is what separates this from
   // all_sounds_off.
   struct all_notes_off : channel_mode
   {
      using channel_mode::channel_mode;
   };

   struct omni_off : channel_mode
   {
      using channel_mode::channel_mode;
   };

   struct omni_on : channel_mode
   {
      using channel_mode::channel_mode;
   };

   // One note at a time, over this many channels. Zero means every channel
   // the instrument has. MPE configures a zone with exactly this message.
   struct mono_mode : channel_mode
   {
      constexpr mono_mode(std::uint8_t channel, std::uint8_t channels)
       : channel_mode(channel), _channels(channels)
      {}

      constexpr std::uint8_t     channels() const  { return _channels; }

   private:

      std::uint8_t   _channels;
   };

   struct poly_mode : channel_mode
   {
      using channel_mode::channel_mode;
   };

   ////////////////////////////////////////////////////////////////////////////
   // mode_reader: a processor that wraps a processor, turning the channel
   // mode controllers into the messages above.
   //
   //    auto chain = midi::mode_reader{my_synth};
   //    midi::dispatch(msg, time, chain);
   //
   // These controllers are 120 and up, so this stage never contends with
   // cc14_reader, whose range ends at 63, or with rpn_reader. Nest them in
   // any order.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   class mode_reader
   {
   public:

      explicit                mode_reader(P next)
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
   };

   ////////////////////////////////////////////////////////////////////////////
   // An lvalue is referred to and a temporary is owned, so a chain can be
   // built in one expression and kept:
   //
   //    auto chain = midi::mode_reader{midi::cc14_reader{my_synth}};
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   mode_reader(P&&) -> mode_reader<P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   inline void mode_reader<P>::operator()(
      control_change msg, std::size_t time)
   {
      auto const channel = msg.channel();
      auto const value = msg.value();

      switch (msg.controller())
      {
         case cc::all_sounds_off:
            _next(all_sounds_off{channel}, time);
            return;

         case cc::reset:
            _next(reset_all_controllers{channel}, time);
            return;

         case cc::local:
            _next(local_control{channel, value >= 64}, time);
            return;

         case cc::all_notes_off:
            _next(all_notes_off{channel}, time);
            return;

         case cc::omni_off:
            _next(omni_off{channel}, time);
            return;

         case cc::omni_on:
            _next(omni_on{channel}, time);
            return;

         case cc::mono:
            _next(mono_mode{channel, value}, time);
            return;

         case cc::poly:
            _next(poly_mode{channel}, time);
            return;

         default:
            _next(msg, time);
            return;
      }
   }
}

#endif
