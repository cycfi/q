/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DELAY_PROCESSOR_SEPTEMBER_7_2026)
#define Q_PLUG_DELAY_PROCESSOR_SEPTEMBER_7_2026

#include <q_plug/processor.hpp>
#include <q/fx/delay.hpp>
#include <q/fx/lowpass.hpp>
#include "delay_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
class delay_processor : public q_plug::processor
{
public:

                        delay_processor(delay_controller& ctl);

   channel_config       channels() const override { return {1, 1}; }

   void                 activate() override;
   void                 reset() override;

   void                 process(
                           in_channels const& in
                         , out_channels const& out) override;

private:

   float                delay_samples() const;

   delay_controller&    _ctl;

   // Both are sized and set from the sample rate in activate.
   q::delay             _delay{std::size_t(1)};
   q::one_pole_lowpass  _delay_lp{0.0f};
};

#endif
