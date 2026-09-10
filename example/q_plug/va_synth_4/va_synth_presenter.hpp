/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_4_PRESENTER_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_4_PRESENTER_SEPTEMBER_11_2026

#include <qplug/presenter.hpp>
#include "va_synth_controller.hpp"
#include "adsr_control.hpp"
#include <memory>

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

   // An envelope control edits four parameters at once, so it is wired by
   // hand rather than bound: each of its four values is watched on the
   // controller's model and sent back as an edit. This is what bind does
   // for a one-value control, written out.
   void                 wire(
                           std::shared_ptr<adsr_control> control
                         , int attack, int decay, int sustain, int release);

   va_synth_controller& _ctl;
};

#endif
