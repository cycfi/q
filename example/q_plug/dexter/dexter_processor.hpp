/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DEXTER_PROCESSOR_OCTOBER_1_2026)
#define Q_PLUG_DEXTER_PROCESSOR_OCTOBER_1_2026

#include <q_plug/midi_processor.hpp>
#include <q/synth/fm/dx_patcher.hpp>
#include <q/synth/fm/fm_voice.hpp>
#include <q/fx/dc_block.hpp>
#include <q/fx/lowpass.hpp>
#include "dexter_controller.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;
namespace midi = q::midi_2_0;
namespace cc = midi::cc;

///////////////////////////////////////////////////////////////////////////////
// The synth: Q's FM synth example, played from a host. A dx_patcher
// compiles the parameters into what the voices take, again whenever one of
// the voice's parameters moves, and the notes sounding take it where they
// are, as a DX7's do: an edit is heard on a held note, and a note held by
// its release level fades when the new patch's is lower.
//
// On top of the example: poly or mono, a glide between notes, pitch bend
// and the sustain pedal.
///////////////////////////////////////////////////////////////////////////////
class dexter_processor
 : public q_plug::midi_processor<dexter_processor>
{
public:

   using midi_processor::operator();

   static constexpr std::size_t num_voices = 16;

   // One voice: an fm_voice and the pitch it plays at. The pitch is a
   // step per sample that glides to where the key puts it.
   struct voice
   {
      void              glide_to(double step, double samples);
      q::phase_iterator next(float bend);

      q::fm_voice       fm;
      q::phase_iterator master;
      double            step = 0.0;          // as the key puts it
      double            now = 0.0;           // where the glide is
      double            ratio = 1.0;         // per sample, while gliding
      std::uint32_t     glide = 0;           // samples left
      std::uint8_t      key = 0;
      float             velocity = 0.0f;     // 0 to 1, as it began
      std::uint64_t     order = 0;           // for stealing the oldest
      bool              held = false;        // key up, pedal down
   };

                        dexter_processor(dexter_controller& ctl);

   // An instrument: no audio in, stereo out.
   channel_config       channels() const override { return {0, 2}; }

   void                 activate() override;
   void                 reset() override;

   void                 parameters_changed() override;
   void                 process(
                           in_channels const& in
                         , out_channels const& out) override;

   void                 operator()(midi::note_on msg, std::size_t time);
   void                 operator()(midi::note_off msg, std::size_t time);
   void                 operator()(midi::control_change msg
                         , std::size_t time);
   void                 operator()(midi::pitch_bend msg, std::size_t time);

private:

   // Pitch bend reaches two semitones each way.
   static constexpr float bend_range = 2.0f;

   void                 note_on(std::uint8_t key, float velocity);
   void                 note_off(std::uint8_t key);
   void                 sustain(bool down);
   voice&               allocate();
   double               step_of(std::uint8_t key) const;
   double               glide_samples() const;

   dexter_controller&       _ctl;
   std::optional<q::dx_patcher>
                        _patcher;
   std::vector<voice>   _voices;
   q::one_pole_lowpass  _volume{1.0f};
   std::optional<q::dc_block>
                        _dc;              // made once the rate is known
   std::vector<std::uint8_t>
                        _keys;            // down, in order, for mono
   std::uint64_t        _order = 0;
   double               _last_step = 0.0; // the last note's, for a glide
   bool                 _sustain = false;
   float                _bend = 1.0f;     // a factor on every pitch
   float                _wheel = 0.0f;    // 0 to 1
};

#endif
