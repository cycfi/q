/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_LOOPER_PROCESSOR_OCTOBER_12_2026)
#define Q_PLUG_LOOPER_PROCESSOR_OCTOBER_12_2026

#include <q_plug/processor.hpp>
#include <q/fx/lowpass.hpp>
#include "looper_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
class looper_processor : public q_plug::processor
{
public:

                        looper_processor(looper_controller& ctl);

   channel_config       channels() const override { return {0, 2}; }

   void                 activate() override;
   void                 reset() override;

   void                 process(
                           in_channels const& in
                         , out_channels const& out) override;

private:

   float                gain() const;
   void                 end();

   looper_controller&   _ctl;
   sample_buffer const* _last = nullptr;     // compared, never read
   double               _pos = 0.0;          // in the sample's frames
   bool                 _ended = false;      // until Play is off
   q::one_pole_lowpass  _gain_lp{1.0f};
};

#endif
