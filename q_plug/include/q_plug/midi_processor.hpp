/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_MIDI_PROCESSOR_HPP_SEPTEMBER_11_2026)
#define QPLUG_MIDI_PROCESSOR_HPP_SEPTEMBER_11_2026

#include <qplug/processor.hpp>
#include <q/midi/ump_processor.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/translate.hpp>

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
   // Which dialect the host sent is not visible here, and that is the whole
   // point: a processor writes MIDI 1.0 overloads and hears every dialect.
   // Bytes reach the MIDI 1.0 dispatch; packets reach the packet reader and
   // then Q's to_midi1, so a MIDI 2.0 note on arrives as a MIDI 1.0 note on
   // with its velocity scaled down. What has no MIDI 1.0 form, per-note
   // pitch bend among it, is dropped there.
   //
   // A processor that wants the MIDI 2.0 messages whole, at their full
   // resolution, overrides midi(packet const&, std::size_t) itself and
   // dispatches as it pleases.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Derived>
   class midi_processor : public processor, public q::midi_2_0::processor
   {
   public:

      using q::midi_2_0::processor::operator();

   protected:

      bool  has_midi_input() const override { return true; }

      void  midi(q::midi_1_0::raw_message msg, std::size_t time) override
      {
         q::midi_1_0::dispatch(msg, time, static_cast<Derived&>(*this));
      }

      void  midi(q::midi_2_0::packet const& p, std::size_t time) override
      {
         _reader(p, time, _translate);
      }

   private:

      using translator = q::midi_2_0::to_midi1<Derived&>;

      q::midi_2_0::packet_reader<>  _reader;
      translator                    _translate{
                                       static_cast<Derived&>(*this)};
   };
}

#endif
