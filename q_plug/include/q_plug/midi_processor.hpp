/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_MIDI_PROCESSOR_HPP_SEPTEMBER_11_2026)
#define QPLUG_MIDI_PROCESSOR_HPP_SEPTEMBER_11_2026

#include <qplug/processor.hpp>
#include <q/midi/processor.hpp>
#include <q/midi/ump_processor.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/translate.hpp>
#include <type_traits>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // midi_processor: a processor that receives the host's MIDI.
   //
   // Derive from it with your own type, pull in its catch-all, and write an
   // overload per message you handle, exactly as a Q processor does:
   //
   //    struct my_processor : qplug::midi_processor<my_processor>
   //    {
   //       using midi_processor::operator();
   //       void operator()(q::midi_2_0::note_on msg, std::size_t time);
   //    };
   //
   // Base names the protocol the overloads are written for, and is the Q
   // processor they derive from. Whatever the host sends, MIDI 1.0 bytes,
   // MIDI 2.0 packets or CLAP notes, arrives in that protocol; which one
   // the host chose is not visible here.
   //
   // The default, q::midi_2_0::processor, loses nothing: MIDI 2.0 messages
   // arrive as they are and MIDI 1.0 ones are widened by Q's to_midi2.
   // q::midi_1_0::processor is best effort: MIDI 1.0 messages arrive as
   // they are, MIDI 2.0 ones are narrowed by Q's to_midi1, and what has no
   // MIDI 1.0 form, per-note pitch bend among it, is dropped.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Derived, typename Base = q::midi_2_0::processor>
   class midi_processor : public processor, public Base
   {
      static_assert(
         std::is_same_v<Base, q::midi_2_0::processor>
      || std::is_same_v<Base, q::midi_1_0::processor>
       , "Base must be q::midi_2_0::processor or q::midi_1_0::processor");

   public:

      using Base::operator();

   protected:

      bool  has_midi_input() const override { return true; }

      void  midi(q::midi_1_0::raw_message msg, std::size_t time) override
      {
         if constexpr (std::is_same_v<Base, q::midi_2_0::processor>)
            q::midi_1_0::dispatch(msg, time, _translate);
         else
            q::midi_1_0::dispatch(msg, time, derived());
      }

      void  midi(q::midi_2_0::packet const& p, std::size_t time) override
      {
         _reader(p, time, _translate);
      }

   private:

      Derived& derived() { return static_cast<Derived&>(*this); }

      using translator = std::conditional_t<
         std::is_same_v<Base, q::midi_2_0::processor>
       , q::midi_2_0::to_midi2<Derived&>
       , q::midi_2_0::to_midi1<Derived&>
      >;

      q::midi_2_0::packet_reader<>  _reader;
      translator                    _translate{derived()};
   };
}

#endif
