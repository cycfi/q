/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_BENDER_PROCESSOR_NOVEMBER_5_2026)
#define Q_PLUG_BENDER_PROCESSOR_NOVEMBER_5_2026

#include <q_plug/midi_processor.hpp>
#include <q/fx/bacf_pitch_shifter.hpp>
#include <q/fx/formant_corrector.hpp>
#include <q/fx/lowpass.hpp>
#include <optional>
#include "bender_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;
namespace midi = q::midi_2_0;

///////////////////////////////////////////////////////////////////////////////
// The bar is the host's MIDI pitch bend. Each sample: the bend in
// semitones, through the bend range and smoothed, as a ratio, drives the
// shifter, then the formant corrector puts the instrument's formants back.
// At rest both pass the input through, with no latency.
///////////////////////////////////////////////////////////////////////////////
class bender_processor
 : public q_plug::midi_processor<bender_processor>
{
public:

   using midi_processor::operator();

                        bender_processor(bender_controller& ctl);

   channel_config       channels() const override { return {1, 1}; }

   void                 activate() override;
   void                 deactivate() override;
   void                 reset() override;

   void                 process(
                           in_channels const& in
                         , out_channels const& out) override;

   void                 operator()(midi::pitch_bend msg, std::size_t time);

private:

   using table_type = q::formant_table;

   void                 formants();
   void                 to_scope(float semitones);

   bender_controller&   _ctl;

   // Made for the sample rate in activate.
   std::optional<q::bacf_pitch_shifter> _shift;
   q::formant_corrector _correct;
   q::one_pole_lowpass  _smooth{0.0f};

   std::uint32_t        _bend = midi::pitch_bend::center;
   table_type const*    _table = nullptr;
   float                _amount = 1.0f;
   std::size_t          _scope_period = 0;
   std::size_t          _scope_count = 0;
};

#endif
