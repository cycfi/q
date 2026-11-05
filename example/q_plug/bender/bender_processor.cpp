/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "bender_processor.hpp"
#include <cmath>
#include <cstring>

using namespace cycfi::q::literals;

namespace
{
   // A bend jumps from one message to the next; this turns the steps into
   // a glide.
   constexpr auto smoothing = 100_Hz;
}

bender_processor::bender_processor(bender_controller& ctl)
 : _ctl(ctl)
{}

void bender_processor::activate()
{
   auto const sps = float(this->sps());
   _shift.emplace(sps);
   _smooth.cutoff(smoothing, sps);
   _scope_period = std::size_t(bender_controller::scope_period * sps);
   _ctl.worker.start(sps);
}

void bender_processor::deactivate()
{
   _ctl.worker.stop();
}

void bender_processor::reset()
{
   if (_shift)
      _shift->reset();
   _correct.reset();
   _smooth = 0.0f;
   _scope_count = 0;
}

void bender_processor::operator()(midi::pitch_bend msg, std::size_t)
{
   _bend = msg.value();
}

// The latest table, once a block, and the amount, given to the corrector
// only when they change: each sets it up afresh.
void bender_processor::formants()
{
   auto const* latest = _ctl.worker.table();
   auto const* table = _ctl.formants() ? latest : nullptr;
   if (table != _table)
   {
      _table = table;
      _correct.table(table);
   }

   auto const amount = _ctl.amount();
   if (amount != _amount)
   {
      _amount = amount;
      _correct.amount(amount);
   }

   _ctl.worker.freeze(_ctl.frozen());
}

// One value per scope period. With the ring full, the editor is away or
// behind, and the value is dropped.
void bender_processor::to_scope(float semitones)
{
   if (++_scope_count < _scope_period)
      return;
   _scope_count = 0;
   if (auto p = _ctl.scope.allocate(sizeof(float)))
   {
      std::memcpy(p, &semitones, sizeof(float));
      _ctl.scope.publish();
   }
}

void bender_processor::process(
   in_channels const& in, out_channels const& out)
{
   formants();

   auto const range = _ctl.range();
   auto const target = range.semitones(midi::pitch_bend{0, 0, _bend});
   auto const frames = in.frames.size();
   auto i = in[0].begin();
   auto o = out[0].begin();

   if (frames != 0)
      _ctl.worker.feed({&i[0], frames});

   for (std::size_t f = 0; f != frames; ++f)
   {
      auto const semitones = _smooth(target);
      auto const ratio = std::exp2(semitones / 12.0f);
      o[f] = _correct((*_shift)(i[f], ratio), ratio);
      to_scope(semitones);
   }
}
