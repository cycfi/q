/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_PROCESSOR_SEPTEMBER_6_2026)
#define QPLUG_GAIN_PROCESSOR_SEPTEMBER_6_2026

#include <qplug/processor.hpp>
#include <q/fx/lowpass.hpp>
#include "gain_controller.hpp"

namespace qplug = cycfi::qplug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
class gain_processor : public qplug::processor
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
