/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_GAIN_PROCESSOR_SEPTEMBER_6_2026)
#define Q_PLUG_GAIN_PROCESSOR_SEPTEMBER_6_2026

#include <q_plug/processor.hpp>
#include <q/fx/lowpass.hpp>
#include "gain_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
class gain_processor : public q_plug::processor
{
public:

                        gain_processor(gain_controller& ctl);

   channel_config       channels() const override { return {1, 1}; }

   void                 activate() override;
   void                 reset() override;

   void                 process(
                           in_channels const& in
                         , out_channels const& out) override;

private:

   float                gain() const;

   gain_controller&     _ctl;
   q::one_pole_lowpass  _gain_lp{0.0f};
};

#endif
