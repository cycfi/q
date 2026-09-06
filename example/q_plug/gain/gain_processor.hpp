/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_PROCESSOR_SEPTEMBER_6_2026)
#define QPLUG_GAIN_PROCESSOR_SEPTEMBER_6_2026

#include <qplug/processor.hpp>
#include "gain_controller.hpp"

namespace qplug = cycfi::qplug;

///////////////////////////////////////////////////////////////////////////////
class gain_processor : public qplug::processor
{
public:
                        gain_processor(gain_controller& ctl);

   qplug::channel_config
                        channels() const override { return {1, 1}; }
   void                 process(in_channels const& in
                         , out_channels const& out) override;

private:

   gain_controller&     _ctl;
};

#endif
