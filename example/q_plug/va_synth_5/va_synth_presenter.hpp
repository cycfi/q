/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_5_PRESENTER_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_5_PRESENTER_SEPTEMBER_11_2026

#include <qplug/presenter.hpp>
#include "va_synth_controller.hpp"
#include "adsr_control.hpp"
#include <memory>
#include <string>

namespace qplug = cycfi::qplug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class va_synth_presenter : public qplug::presenter
{
public:
                        va_synth_presenter(va_synth_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   va_synth_controller& _ctl;
};

#endif
