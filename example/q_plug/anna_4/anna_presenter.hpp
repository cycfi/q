/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_ANNA_4_PRESENTER_SEPTEMBER_11_2026)
#define Q_PLUG_ANNA_4_PRESENTER_SEPTEMBER_11_2026

#include <q_plug/presenter.hpp>
#include "anna_controller.hpp"
#include "adsr_control.hpp"
#include <memory>

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class anna_presenter : public q_plug::presenter
{
public:
                        anna_presenter(anna_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   anna_controller& _ctl;
};

#endif
