/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_PROCESSOR_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_PROCESSOR_SEPTEMBER_11_2026

#include <qplug/midi_processor.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q/fx/clip.hpp>
#include "va_synth_controller.hpp"

#include <cstdint>
#include <vector>

namespace qplug = cycfi::qplug;
namespace q = cycfi::q;
namespace midi = q::midi_1_0;

///////////////////////////////////////////////////////////////////////////////
// The synth: a pool of voices, and the notes that drive them.
//
// This is Q's poly_synth example, with a plugin around it. The voice and the
// pool are the same code: what the plugin adds is parameters that move while
// notes sound, a panel that shows them, and notes that arrive from a host
// rather than from a MIDI port.
//
// Deriving from midi_processor is what makes the host's notes arrive as Q
// messages. Which dialect the host speaks is not visible here.
///////////////////////////////////////////////////////////////////////////////
class va_synth_processor
 : public qplug::midi_processor<va_synth_processor>
{
public:

   using midi_processor::operator();

   static constexpr std::size_t num_voices = 16;

   ////////////////////////////////////////////////////////////////////////////
   // One voice: a bandwidth limited sawtooth through an envelope.
   //
   // The envelope is what makes a note a note, and it is also what tells the
   // pool the voice is finished: a voice is active until its envelope
   // returns to idle, which is after the release has run, not when the key
   // came up.
   ////////////////////////////////////////////////////////////////////////////
   struct voice
   {
                        voice(
                           q::adsr_envelope_gen::config const& cfg
                         , float sps);

      void              on(q::frequency freq, float velocity);
      void              off();
      bool              active() const;
      float             operator()();

      q::phase_iterator _phase;
      q::adsr_envelope_gen
                        _env;
      float             _sps;
      float             _velocity = 0.0f;
      std::uint8_t      _key = 0;      // the MIDI key this voice is playing
      std::uint64_t     _order = 0;    // allocation order, for stealing
   };

                        va_synth_processor(va_synth_controller& ctl);

   // An instrument: no audio in, stereo out.
   channel_config       channels() const override { return {0, 2}; }

   void                 activate() override;
   void                 reset() override;

   void                 process(
                           in_channels const& in
                         , out_channels const& out) override;

   // The notes, as Q delivers them. Everything else a keyboard sends
   // reaches the do-nothing default this pulls in above.
   void                 operator()(midi::note_on msg, std::size_t time);
   void                 operator()(midi::note_off msg, std::size_t time);

private:

   void                 note_on(std::uint8_t key, float velocity);
   void                 note_off(std::uint8_t key);
   voice&               allocate(std::uint8_t key);

   q::adsr_envelope_gen::config
                        envelope_config() const;
   void                 update_envelopes();

   // The envelope settings as they were last pushed into the voices.
   // Pushing a rate into a segment that is running restarts that segment,
   // so a release would never finish if every block pushed the same value
   // again. Only a setting that actually moved is pushed.
   struct settings
   {
      double            attack = -1.0;
      double            decay = -1.0;
      double            sustain_level = -1.0;
      double            sustain_rate = -1.0;
      double            release = -1.0;
   };

   va_synth_controller& _ctl;
   std::vector<voice>   _voices;
   settings             _pushed;
   q::cubic_clip        _clip;
   std::uint64_t        _order = 0;
};

#endif
