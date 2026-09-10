/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_MIDI_PROCESSOR_HPP_SEPTEMBER_11_2026)
#define QPLUG_MIDI_PROCESSOR_HPP_SEPTEMBER_11_2026

#include <qplug/processor.hpp>
#include <q/midi/ump_processor.hpp>
#include <q/midi/packet_reader.hpp>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // midi_processor: a processor that hears the host's notes.
   //
   // Derive from it with your own type, pull in its catch-all, and write an
   // overload per message you answer, exactly as a Q processor does:
   //
   //    struct my_synth : qplug::midi_processor<my_synth>
   //    {
   //       using midi_processor::operator();
   //       void operator()(q::midi_1_0::note_on msg, std::size_t time);
   //    };
   //
   // Which dialect the host sent is not visible here. Bytes reach the MIDI
   // 1.0 dispatch and packets the packet reader, and a MIDI 1.0 voice
   // message inside a packet reaches the same overloads as one that
   // arrived as bytes.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Derived>
   class midi_processor : public processor, public q::midi_2_0::processor
   {
   public:

      using q::midi_2_0::processor::operator();

   private:

      bool  has_midi_input() const override { return true; }

      void  midi(q::midi_1_0::raw_message msg, std::size_t time) override
      {
         q::midi_1_0::dispatch(msg, time, static_cast<Derived&>(*this));
      }

      void  midi(q::midi_2_0::packet const& p, std::size_t time) override
      {
         _reader(p, time, static_cast<Derived&>(*this));
      }

      q::midi_2_0::packet_reader<>  _reader;
   };
}

#endif
