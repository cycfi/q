/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_VA_SYNTH_4_PRESENTER_SEPTEMBER_11_2026)
#define Q_PLUG_VA_SYNTH_4_PRESENTER_SEPTEMBER_11_2026

#include <q_plug/presenter.hpp>
#include "va_synth_controller.hpp"
#include "adsr_control.hpp"
#include <memory>

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class va_synth_presenter : public q_plug::presenter
{
public:
                        va_synth_presenter(va_synth_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   va_synth_controller& _ctl;
};

#endif
