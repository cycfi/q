/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_3_PROCESSOR_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_3_PROCESSOR_SEPTEMBER_11_2026

#include <qplug/midi_processor.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/sin_cos_gen.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q/fx/svf.hpp>
#include <q/fx/chorus.hpp>
#include <q/fx/clip.hpp>
#include "va_synth_controller.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace qplug = cycfi::qplug;
namespace q = cycfi::q;
namespace midi = q::midi_1_0;

///////////////////////////////////////////////////////////////////////////////
// The envelope this synth asks Q for. It carries no sustain rate, and that
// is what makes the sustain hold its level until the key comes up: Q reads
// the shape of the config it is given. Q's own adsr_envelope_gen::config
// carries one, and gets a sustain that runs down instead.
///////////////////////////////////////////////////////////////////////////////
struct va_synth_envelope_config
{
   q::duration    attack_rate;
   q::duration    decay_rate;
   q::decibel     sustain_level;
   q::duration    release_rate;
};

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
                           va_synth_envelope_config const& amp
                         , va_synth_envelope_config const& filter
                         , float sps);

      void              on(
                           q::frequency freq, float velocity
                         , float filter_velocity);
      void              off();
      bool              active() const;

      // One sample, at the pitch the note was struck at times a factor
      // the synth works out once per sample for every voice: the bend
      // and the wheel, which move every note together.
      float             operator()(float pitch_factor);

      // The filter settings, pushed in from the panel each block. The
      // cutoff the envelope sweeps up from, how far up it sweeps, and
      // how resonant the filter is.
      void              filter(
                           q::frequency cutoff, double depth
                         , double resonance);

      q::phase_iterator _phase;
      q::adsr_envelope_gen
                        _env;          // how loud the note is
      q::adsr_envelope_gen
                        _filter_env;   // and how bright
      // Two two-pole sections in series: 24 dB per octave, the slope a
      // classic virtual analog filter has. One pair alone is 12 dB, and
      // a sweep through it is much the milder for it. The resonance sits
      // on the first section only; on both, the peak would double.
      q::svf            _filter;
      q::svf            _filter2;
      q::frequency      _freq{440.0};      // as struck, before any bend
      float             _sps;
      float             _cutoff = 1000.0f;
      float             _depth = 4.0f;
      float             _velocity = 0.0f;
      float             _filter_velocity = 1.0f;   // scales the contour
      std::uint8_t      _key = 0;      // the MIDI key this voice is playing
      std::uint64_t     _order = 0;    // allocation order, for stealing
      bool              _held = false; // its key is up, the pedal is down
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
   void                 operator()(midi::control_change msg
                         , std::size_t time);
   void                 operator()(midi::pitch_bend msg, std::size_t time);

private:

   void                 note_on(std::uint8_t key, float velocity);
   float                sensed(float velocity, double sensitivity) const;
   void                 note_off(std::uint8_t key);
   void                 sustain(bool down);

   // Pitch bend reaches two semitones each way, the convention every
   // keyboard ships with. The wheel adds vibrato from a small sine at a
   // fixed rate, up to half a semitone each way: the classic assignment.
   // The sine is Q's sin_cos_gen, a recursive generator made for low
   // frequencies, a few multiplies a sample.
   static constexpr float  bend_range = 2.0f;          // semitones
   static constexpr float  vibrato_depth = 0.5f;       // semitones
   static constexpr q::frequency vibrato_rate{5.5};
   voice&               allocate(std::uint8_t key);

   va_synth_envelope_config
                        envelope_config() const;
   va_synth_envelope_config
                        filter_envelope_config() const;
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
      double            release = -1.0;

      bool              operator==(settings const&) const = default;
   };

   static void          push(
                           q::adsr_envelope_gen& env
                         , settings const& now, settings const& then
                         , float sustain, float sps);

   void                 update_filter();
   void                 update_chorus();

   va_synth_controller& _ctl;
   std::vector<voice>   _voices;
   settings             _pushed;
   settings             _filter_pushed;
   q::cubic_clip        _clip;
   std::optional<q::chorus>
                        _chorus;       // built once the rate is known
   std::uint64_t        _order = 0;
   bool                 _sustain = false;
   float                _bend = 0.0f;       // semitones, from the wheel
   float                _wheel = 0.0f;      // 0 to 1
   q::sin_cos_gen       _lfo{vibrato_rate, 44100.0f};   // retuned on activate
};

#endif
