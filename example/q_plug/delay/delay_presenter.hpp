/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DELAY_PRESENTER_SEPTEMBER_7_2026)
#define Q_PLUG_DELAY_PRESENTER_SEPTEMBER_7_2026

#include <q_plug/presenter.hpp>
#include "delay_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class delay_presenter : public q_plug::presenter
{
public:
                        delay_presenter(delay_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   delay_controller&    _ctl;
};

#endif
